from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import json

from .elf import ElfImage, Section, Segment, SHF_ALLOC, SHF_WRITE, hx, quote_ld_path, sanitize


@dataclass(frozen=True)
class Item:
    kind: str
    name: str
    addr: int
    size: int
    offset: int
    flags: str
    align: int
    phdrs: tuple[str, ...]
    source_section: str | None = None

    @property
    def end(self) -> int:
        return self.addr + self.size

    @property
    def is_noload(self) -> bool:
        return self.kind in {"nobits", "noload_gap"}


def contains(start: int, end: int, inner_start: int, inner_end: int) -> bool:
    return start <= inner_start and inner_end <= end


def overlaps(a0: int, a1: int, b0: int, b1: int) -> bool:
    return a0 < b1 and b0 < a1


def section_in_load(section: Section, segment: Segment) -> bool:
    if not segment.is_load or segment.memsz == 0:
        return False
    if section.is_nobits:
        return section.addr >= segment.file_end_vma and contains(
            segment.vaddr, segment.mem_end_vma, section.addr, section.end
        )
    return contains(segment.vaddr, segment.file_end_vma, section.addr, section.end)


def item_phdrs(section: Section, segments: list[Segment]) -> tuple[str, ...]:
    names = []
    for seg in segments:
        if seg.is_load and section_in_load(section, seg):
            names.append(seg.name)
        elif not seg.is_load and seg.memsz and contains(seg.vaddr, seg.mem_end_vma, section.addr, section.end):
            names.append(seg.name)
    return tuple(names)


def gap_phdrs(addr: int, size: int, segments: list[Segment]) -> tuple[str, ...]:
    end = addr + size
    return tuple(
        seg.name
        for seg in segments
        if seg.is_load and seg.filesz and contains(seg.vaddr, seg.file_end_vma, addr, end)
    )


def first_filehdr_load(segments: list[Segment]) -> int | None:
    for seg in segments:
        if seg.is_load and seg.offset == 0 and seg.filesz > 0:
            return seg.index
    return None


def add_gap_items(
    out: list[Item],
    addr_start: int,
    addr_end: int,
    file_backed_end: int,
    load_file_offset: int,
    load_vaddr: int,
    blockers: list[Section],
    segments: list[Segment],
) -> None:
    cur = addr_start
    for blocker in sorted([s for s in blockers if overlaps(addr_start, addr_end, s.addr, s.end)], key=lambda s: s.addr):
        if cur < blocker.addr:
            add_gap_range(out, cur, min(blocker.addr, addr_end), file_backed_end, load_file_offset, load_vaddr, segments)
        cur = max(cur, blocker.end)
    if cur < addr_end:
        add_gap_range(out, cur, addr_end, file_backed_end, load_file_offset, load_vaddr, segments)


def add_gap_range(
    out: list[Item],
    start: int,
    end: int,
    file_backed_end: int,
    load_file_offset: int,
    load_vaddr: int,
    segments: list[Segment],
) -> None:
    if start < file_backed_end:
        backed_end = min(end, file_backed_end)
        if backed_end > start:
            out.append(
                Item(
                    kind="gap",
                    name=f".gap_{start:x}",
                    addr=start,
                    size=backed_end - start,
                    offset=load_file_offset + (start - load_vaddr),
                    flags="a",
                    align=1,
                    phdrs=gap_phdrs(start, backed_end - start, segments),
                )
            )
        start = backed_end

    if end > start:
        out.append(
            Item(
                kind="noload_gap",
                name=f".noload_gap_{start:x}",
                addr=start,
                size=end - start,
                offset=0,
                flags="aw",
                align=1,
                phdrs=gap_phdrs(start, end - start, segments),
            )
        )


def collect_items(image: ElfImage) -> list[Item]:
    sections = image.alloc_sections()
    segments = image.segments
    blockers = [s for s in sections if s.is_nobits]
    emitted: set[str] = set()
    items: list[Item] = []
    header_load = first_filehdr_load(segments)

    for seg in sorted([s for s in segments if s.is_load and s.memsz], key=lambda s: (s.vaddr, s.offset, s.index)):
        cur = seg.vaddr
        load_sections = sorted([s for s in sections if section_in_load(s, seg)], key=lambda s: (s.addr, s.offset, s.name))

        for section in load_sections:
            if section.addr > cur and not (seg.index == header_load and cur == seg.vaddr):
                add_gap_items(items, cur, section.addr, seg.file_end_vma, seg.offset, seg.vaddr, blockers, segments)

            if section.name not in emitted:
                items.append(
                    Item(
                        kind="nobits" if section.is_nobits else "section",
                        name=section.name,
                        addr=section.addr,
                        size=section.size,
                        offset=section.offset,
                        flags=section.asm_flags,
                        align=section.align,
                        phdrs=item_phdrs(section, segments),
                        source_section=section.name,
                    )
                )
                emitted.add(section.name)
            cur = max(cur, section.end)

        if cur < seg.mem_end_vma:
            add_gap_items(items, cur, seg.mem_end_vma, seg.file_end_vma, seg.offset, seg.vaddr, blockers, segments)

    for section in sections:
        if section.name in emitted:
            continue
        items.append(
            Item(
                kind="nobits" if section.is_nobits else "section",
                name=section.name,
                addr=section.addr,
                size=section.size,
                offset=section.offset,
                flags=section.asm_flags,
                align=section.align,
                phdrs=item_phdrs(section, segments) or ("NONE",),
                source_section=section.name,
            )
        )

    return sorted(items, key=lambda item: (item.addr, item.kind, item.name))


def text_items(image: ElfImage) -> list[dict[str, object]]:
    text = image.section(".text")
    funcs = image.text_functions()
    out: list[dict[str, object]] = []
    cur = text.addr

    for i, func in enumerate(funcs):
        if func.addr < cur:
            continue
        if func.addr > cur:
            out.append({"kind": "gap", "addr": cur, "size": func.addr - cur, "symbol": ""})
        next_addr = funcs[i + 1].addr if i + 1 < len(funcs) else text.end
        size = func.size if func.size > 0 and func.addr + func.size <= text.end else next_addr - func.addr
        out.append({"kind": "func", "addr": func.addr, "size": size, "symbol": func.name})
        cur = max(cur, func.addr + size)

    if cur < text.end:
        out.append({"kind": "gap", "addr": cur, "size": text.end - cur, "symbol": ""})

    for item in out:
        item["offset"] = text.offset + (int(item["addr"]) - text.addr)
        addr = int(item["addr"])
        label = sanitize(str(item["symbol"]), f"gap_{addr:08x}")
        item["section"] = f".text.func_{addr:08x}_{label}"
    return out


def emit_words(out, data: bytes) -> None:
    whole = len(data) & ~3
    for i in range(0, whole, 4):
        out.write(f"    .4byte 0x{int.from_bytes(data[i:i + 4], 'big'):08x}\n")
    for byte in data[whole:]:
        out.write(f"    .byte 0x{byte:02x}\n")


def write_manifest(image: ElfImage, build_dir: Path) -> None:
    build_dir.mkdir(parents=True, exist_ok=True)
    data = {
        "entry": hx(image.entry),
        "segments": [
            {
                "name": seg.name,
                "type": seg.ld_type,
                "offset": hx(seg.offset),
                "vaddr": hx(seg.vaddr),
                "filesz": hx(seg.filesz),
                "memsz": hx(seg.memsz),
                "flags": hx(seg.flags),
                "align": hx(seg.align),
            }
            for seg in image.segments
        ],
        "sections": [
            {
                "name": sec.name,
                "type": sec.sh_type,
                "addr": hx(sec.addr),
                "offset": hx(sec.offset),
                "size": hx(sec.size),
                "flags": hx(sec.flags),
                "align": sec.align,
                "nobits": sec.is_nobits,
            }
            for sec in image.alloc_sections()
        ],
        "relocations": [
            {
                "section": rel.section,
                "offset": hx(rel.offset),
                "type": str(rel.r_type),
                "symbol": rel.sym_name,
                "addend": None if rel.addend is None else hx(rel.addend),
            }
            for rel in image.relocations
        ],
    }
    (build_dir / "manifest.json").write_text(json.dumps(data, indent=2) + "\n")

    with (build_dir / "sections.tsv").open("w") as out:
        out.write("name\ttype\taddr\toffset\tsize\tflags\talign\tnobits\n")
        for sec in image.alloc_sections():
            out.write(f"{sec.name}\t{sec.sh_type}\t{hx(sec.addr)}\t{hx(sec.offset)}\t{hx(sec.size)}\t{hx(sec.flags)}\t{sec.align}\t{int(sec.is_nobits)}\n")

    with (build_dir / "relocations.tsv").open("w") as out:
        out.write("section\toffset\ttype\tsymbol\taddend\n")
        for rel in image.relocations:
            addend = "" if rel.addend is None else hx(rel.addend)
            out.write(f"{rel.section}\t{hx(rel.offset)}\t{rel.r_type}\t{rel.sym_name}\t{addend}\n")


def write_generated(image: ElfImage, root: Path, replacements: dict[str, dict[str, str]]) -> None:
    build_dir = root / "build"
    asm_dir = root / "asm"
    build_dir.mkdir(parents=True, exist_ok=True)
    (asm_dir / "text").mkdir(parents=True, exist_ok=True)

    txt_items = text_items(image)
    all_items = collect_items(image)

    with (build_dir / "text_layout.tsv").open("w") as out:
        out.write("kind\taddr\tsize\toffset\tsection\tsymbol\n")
        for item in txt_items:
            out.write(f"{item['kind']}\t{hx(int(item['addr']))}\t{hx(int(item['size']))}\t{hx(int(item['offset']))}\t{item['section']}\t{item['symbol']}\n")

    with (asm_dir / "text" / "text_blobs.s").open("w") as out:
        out.write("/* Auto-generated fallback text. */\n\n")
        for item in txt_items:
            if item["kind"] == "func" and item["symbol"] in replacements:
                continue
            out.write(f'.section {item["section"]}, "ax", @progbits\n')
            emit_words(out, image.read_at_offset(int(item["offset"]), int(item["size"])))
            out.write("\n")

    with (asm_dir / "section_blobs.s").open("w") as out:
        out.write("/* Auto-generated fallback data/rodata sections. */\n\n")
        out.write(".global __entry\n")
        out.write(f".set __entry, {hx(image.entry)}\n\n")
        for item in all_items:
            if item.source_section == ".text" or item.is_noload:
                continue
            out.write(f'.section {item.name}, "{item.flags}", @progbits\n')
            out.write(f'    .incbin "{image.path.as_posix()}", {item.offset}, {item.size}\n\n')

    write_text_ldinc(build_dir / "text_layout.ldinc", image, txt_items, replacements)
    write_linker(build_dir / "linker.ld", image, all_items)


def write_text_ldinc(path: Path, image: ElfImage, items: list[dict[str, object]], replacements: dict[str, dict[str, str]]) -> None:
    text = image.section(".text")
    with path.open("w") as out:
        out.write("/* Auto-generated text layout include. */\n")
        for item in items:
            start = int(item["addr"]) - text.addr
            end = start + int(item["size"])
            out.write(f"    . = {hx(start)};\n")
            if item["kind"] == "func" and item["symbol"] in replacements:
                repl = replacements[str(item["symbol"])]
                out.write(f"    KEEP({quote_ld_path(repl['object'])}({repl['section']}))\n")
            else:
                out.write(f"    KEEP(*( {item['section']} ))\n".replace("*( ", "*("))
            out.write(f"    . = {hx(end)};\n")


def write_linker(path: Path, image: ElfImage, items: list[Item]) -> None:
    header_load = first_filehdr_load(image.segments)
    with path.open("w") as out:
        out.write("/* Auto-generated linker script. */\n")
        out.write('OUTPUT_FORMAT("elf64-powerpc")\n')
        out.write("OUTPUT_ARCH(powerpc:common64)\n")
        out.write("ENTRY(__entry)\n\n")
        out.write("PHDRS\n{\n")
        for seg in image.segments:
            if seg.p_type == 0:
                continue
            extra = " FILEHDR PHDRS" if seg.index == header_load else ""
            out.write(f"  {seg.name} {seg.ld_type}{extra} FLAGS({hx(seg.flags)});\n")
        out.write("}\n\nSECTIONS\n{\n")
        out.write(f"  __entry = {hx(image.entry)};\n\n")
        for item in items:
            phdrs = " ".join(f":{p}" for p in item.phdrs if p != "NONE") or ":NONE"
            out.write(f"  . = {hx(item.addr)};\n")
            if item.is_noload:
                out.write(f"  {item.name} {hx(item.addr)} (NOLOAD) :\n  {{\n    . += {hx(item.size)};\n  }} {phdrs}\n\n")
            else:
                out.write(f"  {item.name} {hx(item.addr)} :\n  {{\n")
                if item.source_section == ".text":
                    out.write((path.parent / "text_layout.ldinc").read_text())
                else:
                    out.write(f'    KEEP("build/section_blobs.o"({item.name}))\n')
                out.write(f"  }} {phdrs}\n\n")
        out.write("  /DISCARD/ :\n")
        out.write("  {\n")
        out.write("    *(.opd) *(.opd.*)\n")
        out.write("    *(.toc) *(.toc.*) *(.got2)\n")
        out.write("    *(.text) *(.text.*)\n")
        out.write("    *(.data) *(.data.*) *(.bss) *(.bss.*)\n")
        out.write("    *(.comment) *(.note*) *(.eh_frame*) *(.gcc_except_table*)\n")
        out.write("  }\n")
        out.write("}\n")
