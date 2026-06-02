#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import struct
import sys
from typing import Iterable

from elftools.elf.elffile import ELFFile


ROOT = Path(__file__).resolve().parents[1]

# Section flags
SHF_WRITE = 0x1
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHF_TLS = 0x400

# Program header types
PT_NULL = 0
PT_LOAD = 1
PT_DYNAMIC = 2
PT_INTERP = 3
PT_NOTE = 4
PT_SHLIB = 5
PT_PHDR = 6
PT_TLS = 7

PT_NAMES = {
    PT_NULL: "PT_NULL",
    PT_LOAD: "PT_LOAD",
    PT_DYNAMIC: "PT_DYNAMIC",
    PT_INTERP: "PT_INTERP",
    PT_NOTE: "PT_NOTE",
    PT_SHLIB: "PT_SHLIB",
    PT_PHDR: "PT_PHDR",
    PT_TLS: "PT_TLS",
}


@dataclass(frozen=True)
class Phdr:
    index: int
    p_type: int
    flags: int
    offset: int
    vaddr: int
    paddr: int
    filesz: int
    memsz: int
    align: int

    @property
    def name(self) -> str:
        return f"ph{self.index}"

    @property
    def ld_type(self) -> str:
        # GNU ld PHDRS accepts numeric p_type values for OS-specific headers.
        return PT_NAMES.get(self.p_type, f"0x{self.p_type:x}")

    @property
    def is_load(self) -> bool:
        return self.p_type == PT_LOAD

    @property
    def is_tls(self) -> bool:
        return self.p_type == PT_TLS

    @property
    def mem_start(self) -> int:
        return self.vaddr

    @property
    def mem_end(self) -> int:
        return self.vaddr + self.memsz

    @property
    def file_mem_end(self) -> int:
        return self.vaddr + self.filesz


@dataclass(frozen=True)
class Section:
    name: str
    safe: str
    sh_type: str
    flags: int
    addr: int
    offset: int
    size: int
    align: int

    @property
    def is_alloc(self) -> bool:
        return bool(self.flags & SHF_ALLOC)

    @property
    def is_nobits(self) -> bool:
        return self.sh_type == "SHT_NOBITS"

    @property
    def is_progbits_like(self) -> bool:
        return not self.is_nobits

    @property
    def end(self) -> int:
        return self.addr + self.size


@dataclass
class Item:
    kind: str
    name: str
    asm_section: str
    addr: int
    size: int
    offset: int
    flags: int
    align: int
    source_section: Section | None
    explicit_phdrs: list[str]

    @property
    def end(self) -> int:
        return self.addr + self.size

    @property
    def is_noload(self) -> bool:
        return self.kind in {"nobits", "noload_gap"}


def sanitize(name: str) -> str:
    s = name.strip(".")
    s = re.sub(r"[^A-Za-z0-9_]+", "_", s)
    if not s:
        s = "unnamed"
    if s[0].isdigit():
        s = "_" + s
    return s


def asm_flags(sh_flags: int) -> str:
    flags = "a"
    if sh_flags & SHF_EXECINSTR:
        flags += "x"
    if sh_flags & SHF_WRITE:
        flags += "w"
    if sh_flags & SHF_TLS:
        flags += "T"
    return flags


def ranges_overlap(a0: int, a1: int, b0: int, b1: int) -> bool:
    return a0 < b1 and b0 < a1


def range_contains(outer0: int, outer1: int, inner0: int, inner1: int) -> bool:
    return outer0 <= inner0 and inner1 <= outer1


def read_raw_phdrs(elf_path: Path, elf: ELFFile) -> list[Phdr]:
    """
    pyelftools sometimes gives named p_type strings. For linker reproduction we
    want the raw numeric p_type and p_flags exactly as stored.
    """
    header = elf.header
    phoff = int(header["e_phoff"])
    phentsize = int(header["e_phentsize"])
    phnum = int(header["e_phnum"])

    if elf.elfclass != 64:
        raise RuntimeError("expected ELF64")

    endian = ">" if not elf.little_endian else "<"
    fmt = endian + "IIQQQQQQ"
    expected_size = struct.calcsize(fmt)

    if phentsize < expected_size:
        raise RuntimeError(f"unexpected program header size: {phentsize}")

    phdrs: list[Phdr] = []

    with elf_path.open("rb") as f:
        f.seek(phoff)

        for i in range(phnum):
            raw = f.read(phentsize)
            if len(raw) != phentsize:
                raise RuntimeError("short read while reading program headers")

            p_type, p_flags, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align = struct.unpack(
                fmt, raw[:expected_size]
            )

            phdrs.append(
                Phdr(
                    index=i,
                    p_type=p_type,
                    flags=p_flags,
                    offset=p_offset,
                    vaddr=p_vaddr,
                    paddr=p_paddr,
                    filesz=p_filesz,
                    memsz=p_memsz,
                    align=p_align,
                )
            )

    return phdrs


def read_alloc_sections(elf: ELFFile) -> list[Section]:
    out: list[Section] = []

    for sec in elf.iter_sections():
        flags = int(sec["sh_flags"])
        if not (flags & SHF_ALLOC):
            continue

        size = int(sec["sh_size"])
        if size == 0:
            continue

        name = sec.name
        out.append(
            Section(
                name=name,
                safe=sanitize(name),
                sh_type=sec["sh_type"],
                flags=flags,
                addr=int(sec["sh_addr"]),
                offset=int(sec["sh_offset"]),
                size=size,
                align=int(sec["sh_addralign"]) or 1,
            )
        )

    out.sort(key=lambda s: (s.addr, s.offset, s.name))
    return out


def section_in_load_phdr(sec: Section, ph: Phdr) -> bool:
    """
    Recreate normal LOAD membership.

    PROGBITS-like sections belong to a LOAD if their VMA range is inside the
    file-backed portion of that LOAD.

    NOBITS sections belong to a LOAD only if they live in the memory-only tail.
    This avoids incorrectly putting .tbss into the RW LOAD just because its
    VMA numerically falls inside the load range.
    """
    if not ph.is_load or ph.memsz == 0:
        return False

    if sec.is_nobits:
        return (
            sec.addr >= ph.file_mem_end
            and range_contains(ph.mem_start, ph.mem_end, sec.addr, sec.end)
        )

    return range_contains(ph.mem_start, ph.file_mem_end, sec.addr, sec.end)


def section_in_nonload_phdr(sec: Section, ph: Phdr) -> bool:
    if ph.is_load or ph.memsz == 0:
        return False

    return range_contains(ph.mem_start, ph.mem_end, sec.addr, sec.end)


def item_phdrs_for_section(sec: Section, phdrs: list[Phdr]) -> list[str]:
    result: list[str] = []

    for ph in phdrs:
        if ph.is_load:
            if section_in_load_phdr(sec, ph):
                result.append(ph.name)
        else:
            if section_in_nonload_phdr(sec, ph):
                result.append(ph.name)

    return result


def phdrs_for_gap(addr: int, size: int, phdrs: list[Phdr]) -> list[str]:
    """
    Gap blobs are only there to preserve file bytes in PT_LOAD segments.
    They should not accidentally become part of TLS/LOOS metadata.
    """
    result: list[str] = []
    end = addr + size

    for ph in phdrs:
        if not ph.is_load or ph.filesz == 0:
            continue

        if range_contains(ph.mem_start, ph.file_mem_end, addr, end):
            result.append(ph.name)

    return result


def find_first_filehdr_load(phdrs: list[Phdr]) -> int | None:
    """
    Original PS3 ELF has LOAD 0 with p_offset=0 and p_vaddr=0x10000.
    That means ELF header + PHDR table are inside the first LOAD.
    """
    for ph in phdrs:
        if ph.is_load and ph.filesz > 0 and ph.offset == 0:
            return ph.index
    return None


def collect_items(
    elf_path: Path,
    phdrs: list[Phdr],
    sections: list[Section],
) -> list[Item]:
    items: list[Item] = []
    emitted_sections: set[str] = set()
    first_filehdr_load = find_first_filehdr_load(phdrs)

    # NOBITS sections can overlap what looks like file padding by VMA.
    # Treat them as blockers so we do not generate explicit gap sections over
    # the same addresses.
    nobits_blockers = [s for s in sections if s.is_nobits]

    nonempty_loads = [ph for ph in phdrs if ph.is_load and ph.memsz > 0]
    nonempty_loads.sort(key=lambda p: (p.vaddr, p.offset, p.index))

    for ph in nonempty_loads:
        load_start = ph.mem_start
        load_file_end = ph.file_mem_end
        load_mem_end = ph.mem_end

        # Sections that actually belong to this LOAD by the rules above.
        load_sections = [s for s in sections if section_in_load_phdr(s, ph)]
        load_sections.sort(key=lambda s: (s.addr, s.offset, s.name))

        cur = load_start

        for sec in load_sections:
            # Add gaps before this section. Split around NOBITS blockers to
            # avoid overlapping .tbss/.bss/etc.
            if sec.addr > cur:
                add_gap_items(
                    items=items,
                    elf_path=elf_path,
                    phdrs=phdrs,
                    addr_start=cur,
                    addr_end=sec.addr,
                    file_backed_end=load_file_end,
                    load_file_offset=ph.offset,
                    load_vaddr=ph.vaddr,
                    blockers=nobits_blockers,
                    skip_filehdr_gap=(
                        ph.index == first_filehdr_load
                        and cur == load_start
                    ),
                )

            if sec.name not in emitted_sections:
                items.append(
                    Item(
                        kind="nobits" if sec.is_nobits else "section",
                        name=sec.name,
                        asm_section=sec.name,
                        addr=sec.addr,
                        size=sec.size,
                        offset=sec.offset,
                        flags=sec.flags,
                        align=sec.align,
                        source_section=sec,
                        explicit_phdrs=item_phdrs_for_section(sec, phdrs),
                    )
                )
                emitted_sections.add(sec.name)

            cur = max(cur, sec.end)

        if cur < load_mem_end:
            add_gap_items(
                items=items,
                elf_path=elf_path,
                phdrs=phdrs,
                addr_start=cur,
                addr_end=load_mem_end,
                file_backed_end=load_file_end,
                load_file_offset=ph.offset,
                load_vaddr=ph.vaddr,
                blockers=nobits_blockers,
                skip_filehdr_gap=False,
            )

    # Add non-LOAD-only sections, e.g. .tbss if it did not belong to a LOAD.
    for sec in sections:
        if sec.name in emitted_sections:
            continue

        phs = item_phdrs_for_section(sec, phdrs)
        if not phs:
            # Alloc section with no PHDR membership. This is unusual, but don't
            # silently drop it.
            phs = ["NONE"]

        items.append(
            Item(
                kind="nobits" if sec.is_nobits else "section",
                name=sec.name,
                asm_section=sec.name,
                addr=sec.addr,
                size=sec.size,
                offset=sec.offset,
                flags=sec.flags,
                align=sec.align,
                source_section=sec,
                explicit_phdrs=phs,
            )
        )
        emitted_sections.add(sec.name)

    items.sort(key=lambda i: (i.addr, i.kind, i.name))
    return items


def add_gap_items(
    items: list[Item],
    elf_path: Path,
    phdrs: list[Phdr],
    addr_start: int,
    addr_end: int,
    file_backed_end: int,
    load_file_offset: int,
    load_vaddr: int,
    blockers: list[Section],
    skip_filehdr_gap: bool,
) -> None:
    """
    Add gap blobs for file-backed padding and NOLOAD gaps for memory-only holes.
    Split around NOBITS blockers so output sections do not overlap.
    """
    if addr_end <= addr_start:
        return

    intervals: list[tuple[int, int, str]] = []

    cur = addr_start

    # If this is the ELF header/PHDR gap inside the first LOAD, do not emit it
    # as a section. FILEHDR PHDRS owns that area.
    if skip_filehdr_gap:
        cur = addr_end
        return

    relevant_blockers = [
        b for b in blockers
        if ranges_overlap(addr_start, addr_end, b.addr, b.end)
    ]
    relevant_blockers.sort(key=lambda b: b.addr)

    for b in relevant_blockers:
        if b.addr > cur:
            intervals.append((cur, min(b.addr, addr_end), "gap"))

        intervals.append((max(cur, b.addr), min(addr_end, b.end), "blocker"))
        cur = max(cur, b.end)

    if cur < addr_end:
        intervals.append((cur, addr_end, "gap"))

    for a0, a1, kind in intervals:
        if a1 <= a0:
            continue

        if kind == "blocker":
            continue

        if a0 < file_backed_end:
            file_part_end = min(a1, file_backed_end)
            if file_part_end > a0:
                file_offset = load_file_offset + (a0 - load_vaddr)
                size = file_part_end - a0
                name = f".gap_{a0:x}"

                items.append(
                    Item(
                        kind="gap",
                        name=name,
                        asm_section=name,
                        addr=a0,
                        size=size,
                        offset=file_offset,
                        flags=SHF_ALLOC,
                        align=1,
                        source_section=None,
                        explicit_phdrs=phdrs_for_gap(a0, size, phdrs),
                    )
                )

            if a1 > file_backed_end:
                a0 = file_backed_end

        if a1 > file_backed_end:
            size = a1 - max(a0, file_backed_end)
            start = max(a0, file_backed_end)
            if size > 0:
                name = f".noload_gap_{start:x}"
                items.append(
                    Item(
                        kind="noload_gap",
                        name=name,
                        asm_section=name,
                        addr=start,
                        size=size,
                        offset=0,
                        flags=SHF_ALLOC | SHF_WRITE,
                        align=1,
                        source_section=None,
                        explicit_phdrs=phdrs_for_gap(start, size, phdrs),
                    )
                )


def write_asm(asm_path: Path, elf_path: Path, entry: int, items: list[Item]) -> None:
    with asm_path.open("w") as out:
        out.write("/* Auto-generated by prepare_sections.py. Do not edit. */\n\n")
        out.write(".global __entry\n")
        out.write(f".set __entry, 0x{entry:x}\n\n")

        

        for item in items:
            if item.source_section is not None and item.source_section.name == ".text":
                continue

            if item.is_noload:
                continue

            flags = asm_flags(item.flags)
            secname = item.asm_section

            out.write(f'.section {secname}, "{flags}", @progbits\n')
            out.write(f".global __{sanitize(secname)}_start\n")
            out.write(f"__{sanitize(secname)}_start:\n")
            out.write(
                f'    .incbin "{elf_path.as_posix()}", '
                f"{item.offset}, {item.size}\n"
            )
            out.write(f".global __{sanitize(secname)}_end\n")
            out.write(f"__{sanitize(secname)}_end:\n\n")


def write_linker_script(
    ld_path: Path,
    entry: int,
    phdrs: list[Phdr],
    items: list[Item],
) -> None:
    first_filehdr_load = find_first_filehdr_load(phdrs)

    with ld_path.open("w") as out:
        out.write("/* Auto-generated by prepare_sections.py. Do not edit. */\n")
        out.write('OUTPUT_FORMAT("elf64-powerpc")\n')
        out.write("OUTPUT_ARCH(powerpc:common64)\n")
        out.write("ENTRY(__entry)\n\n")

        out.write("PHDRS\n{\n")
        for ph in phdrs:
            if ph.p_type == PT_NULL:
                # Usually not useful to recreate null headers in ld output.
                # Your ELF does not appear to have PT_NULL, but keep this safe.
                continue

            extras = ""
            if ph.index == first_filehdr_load:
                extras = " FILEHDR PHDRS"

            out.write(
                f"  {ph.name} {ph.ld_type}{extras} "
                f"FLAGS(0x{ph.flags:x});\n"
            )
        out.write("}\n\n")

        out.write("SECTIONS\n{\n")
        out.write(f"  __entry = 0x{entry:x};\n\n")

        for item in items:
            phdr_part = " ".join(f":{p}" for p in item.explicit_phdrs if p != "NONE")
            if not phdr_part:
                phdr_part = ":NONE"

            out.write(f"  . = 0x{item.addr:x};\n")

            if item.is_noload:
                out.write(f"  {item.name} 0x{item.addr:x} (NOLOAD) :\n")
                out.write("  {\n")
                out.write(f"    . += 0x{item.size:x};\n")
                out.write(f"  }} {phdr_part}\n\n")
            else:
                out.write(f"  {item.name} 0x{item.addr:x} :\n")
                out.write("  {\n")

                if item.source_section is not None and item.source_section.name == ".text":
                    text_layout = ROOT / "build" / "text_layout.ldinc"

                    if not text_layout.exists():
                        raise RuntimeError(
                            "build/text_layout.ldinc does not exist. "
                            "Run tools/prepare_text_split.py before prepare_sections.py."
                        )

                    with text_layout.open("r") as inc:
                        for line in inc:
                            if line.strip().startswith("/*"):
                                continue
                            if line.strip():
                                out.write(line)
                else:
                    out.write(f'    KEEP("build/section_blobs.o"({item.asm_section}))\n')

                out.write(f"  }} {phdr_part}\n\n")

        out.write("  /DISCARD/ :\n")
        out.write("  {\n")
        out.write("    *(.opd)\n")
        out.write("    *(.opd.*)\n")
        out.write("    *(.toc*)\n")
        out.write("    *(.got2)\n")
        out.write("    *(.eh_frame)\n")
        out.write("    *(.eh_frame_hdr)\n")
        out.write("    *(.gcc_except_table*)\n")
        out.write("    *(.comment)\n")
        out.write("    *(.note*)\n")
        out.write("  }\n\n")

        out.write("}\n")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: prepare_sections.py orig/EBOOT.ELF")
        return 1

    elf_path = Path(sys.argv[1])
    asm_path = ROOT / "asm" / "section_blobs.s"
    ld_path = ROOT / "build" / "linker_sections.ld"

    asm_path.parent.mkdir(parents=True, exist_ok=True)
    ld_path.parent.mkdir(parents=True, exist_ok=True)

    with elf_path.open("rb") as f:
        elf = ELFFile(f)
        entry = int(elf.header["e_entry"])
        phdrs = read_raw_phdrs(elf_path, elf)
        sections = read_alloc_sections(elf)

    items = collect_items(elf_path, phdrs, sections)

    write_asm(asm_path, elf_path, entry, items)
    write_linker_script(ld_path, entry, phdrs, items)

    print(f"wrote {asm_path}")
    print(f"wrote {ld_path}")
    print(f"program headers: {len(phdrs)}")
    print(f"items: {len(items)}")

    for ph in phdrs:
        print(
            f"ph{ph.index}: type={ph.ld_type} "
            f"off=0x{ph.offset:x} vaddr=0x{ph.vaddr:x} "
            f"filesz=0x{ph.filesz:x} memsz=0x{ph.memsz:x} "
            f"flags=0x{ph.flags:x} align=0x{ph.align:x}"
        )

    for item in items:
        print(
            f"{item.kind:10} addr=0x{item.addr:x} size=0x{item.size:x} "
            f"phdrs={','.join(item.explicit_phdrs)} {item.name}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())