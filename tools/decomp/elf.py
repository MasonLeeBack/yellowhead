from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import struct

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
from elftools.elf.sections import SymbolTableSection

SHF_WRITE = 0x1
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHF_TLS = 0x400

PT_NULL = 0
PT_LOAD = 1
PT_TLS = 7

PT_NAMES = {
    PT_NULL: "PT_NULL",
    PT_LOAD: "PT_LOAD",
    PT_TLS: "PT_TLS",
}


def hx(value: int) -> str:
    return f"0x{value:x}"


def sanitize(name: str, fallback: str = "unnamed") -> str:
    name = name.strip(".") or fallback
    name = re.sub(r"[^A-Za-z0-9_.$+-]+", "_", name)
    name = name.strip("._") or fallback
    if name[0].isdigit():
        name = "_" + name
    return name[:140]


def quote_ld_path(path: str) -> str:
    return '"' + path.replace("\\", "/").replace('"', '\\"') + '"'


@dataclass(frozen=True)
class Segment:
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
        return PT_NAMES.get(self.p_type, f"0x{self.p_type:x}")

    @property
    def is_load(self) -> bool:
        return self.p_type == PT_LOAD

    @property
    def file_end_vma(self) -> int:
        return self.vaddr + self.filesz

    @property
    def mem_end_vma(self) -> int:
        return self.vaddr + self.memsz


@dataclass(frozen=True)
class Section:
    name: str
    sh_type: str
    flags: int
    addr: int
    offset: int
    size: int
    align: int

    @property
    def end(self) -> int:
        return self.addr + self.size

    @property
    def is_alloc(self) -> bool:
        return bool(self.flags & SHF_ALLOC)

    @property
    def is_exec(self) -> bool:
        return bool(self.flags & SHF_EXECINSTR)

    @property
    def is_write(self) -> bool:
        return bool(self.flags & SHF_WRITE)

    @property
    def is_tls(self) -> bool:
        return bool(self.flags & SHF_TLS)

    @property
    def is_nobits(self) -> bool:
        return self.sh_type == "SHT_NOBITS"

    @property
    def asm_flags(self) -> str:
        flags = "a"
        if self.is_exec:
            flags += "x"
        if self.is_write:
            flags += "w"
        if self.is_tls:
            flags += "T"
        return flags


@dataclass(frozen=True)
class Symbol:
    name: str
    addr: int
    size: int
    kind: str
    bind: str
    section: str | None


@dataclass(frozen=True)
class Reloc:
    section: str
    offset: int
    r_type: int | str
    sym_name: str
    addend: int | None


class ElfImage:
    def __init__(self, path: Path):
        self.path = path
        self._fh = path.open("rb")
        self.elf = ELFFile(self._fh)
        self.endian = ">" if not self.elf.little_endian else "<"
        self.sections = self._read_sections()
        self.segments = self._read_segments()
        self.symbols = self._read_symbols()
        self.relocations = self._read_relocations()

    def close(self) -> None:
        self._fh.close()

    def __enter__(self) -> "ElfImage":
        return self

    def __exit__(self, _exc_type, _exc, _tb) -> None:
        self.close()

    @property
    def entry(self) -> int:
        return int(self.elf.header["e_entry"])

    def section(self, name: str) -> Section:
        for section in self.sections:
            if section.name == name:
                return section
        raise RuntimeError(f"missing section {name}")

    def alloc_sections(self) -> list[Section]:
        return [section for section in self.sections if section.is_alloc and section.size > 0]

    def text_functions(self) -> list[Symbol]:
        text = self.section(".text")
        funcs = [
            sym
            for sym in self.symbols
            if sym.kind == "STT_FUNC"
            and sym.name not in {".text", ".init", ".fini"}
            and text.addr <= sym.addr < text.end
        ]
        by_addr: dict[int, Symbol] = {}
        for sym in funcs:
            old = by_addr.get(sym.addr)
            if old is None or self._symbol_score(sym) > self._symbol_score(old):
                by_addr[sym.addr] = sym
        return [by_addr[addr] for addr in sorted(by_addr)]

    def read_at_offset(self, offset: int, size: int) -> bytes:
        self._fh.seek(offset)
        data = self._fh.read(size)
        if len(data) != size:
            raise RuntimeError(f"short read at file offset 0x{offset:x}")
        return data

    def _read_sections(self) -> list[Section]:
        out = []
        for sec in self.elf.iter_sections():
            out.append(
                Section(
                    name=sec.name,
                    sh_type=sec["sh_type"],
                    flags=int(sec["sh_flags"]),
                    addr=int(sec["sh_addr"]),
                    offset=int(sec["sh_offset"]),
                    size=int(sec["sh_size"]),
                    align=int(sec["sh_addralign"]) or 1,
                )
            )
        return out

    def _read_segments(self) -> list[Segment]:
        if self.elf.elfclass != 64:
            raise RuntimeError("expected ELF64")

        phoff = int(self.elf.header["e_phoff"])
        entsize = int(self.elf.header["e_phentsize"])
        count = int(self.elf.header["e_phnum"])
        fmt = self.endian + "IIQQQQQQ"
        size = struct.calcsize(fmt)
        out = []

        self._fh.seek(phoff)
        for index in range(count):
            raw = self._fh.read(entsize)
            if len(raw) != entsize:
                raise RuntimeError("short read while reading program headers")
            p_type, flags, offset, vaddr, paddr, filesz, memsz, align = struct.unpack(fmt, raw[:size])
            out.append(Segment(index, p_type, flags, offset, vaddr, paddr, filesz, memsz, align))
        return out

    def _read_symbols(self) -> list[Symbol]:
        sections = list(self.elf.iter_sections())
        out = []
        for sec in sections:
            if not isinstance(sec, SymbolTableSection):
                continue
            for sym in sec.iter_symbols():
                if not sym.name:
                    continue
                shndx = sym["st_shndx"]
                section_name = sections[shndx].name if isinstance(shndx, int) and shndx < len(sections) else None
                out.append(
                    Symbol(
                        name=sym.name,
                        addr=int(sym["st_value"]),
                        size=int(sym["st_size"]),
                        kind=sym["st_info"]["type"],
                        bind=sym["st_info"]["bind"],
                        section=section_name,
                    )
                )
        return sorted(out, key=lambda sym: (sym.addr, sym.name))

    def _read_relocations(self) -> list[Reloc]:
        out = []
        sections = list(self.elf.iter_sections())
        for sec in sections:
            if not isinstance(sec, RelocationSection):
                continue
            symtab = sections[sec["sh_link"]]
            for rel in sec.iter_relocations():
                sym_name = ""
                sym_index = rel["r_info_sym"]
                if isinstance(symtab, SymbolTableSection) and sym_index:
                    sym_name = symtab.get_symbol(sym_index).name
                addend = int(rel["r_addend"]) if rel.is_RELA() else None
                out.append(
                    Reloc(
                        section=sec.name,
                        offset=int(rel["r_offset"]),
                        r_type=rel["r_info_type"],
                        sym_name=sym_name,
                        addend=addend,
                    )
                )
        return out

    @staticmethod
    def _symbol_score(sym: Symbol) -> tuple[int, int, int, int]:
        bind_rank = {"STB_GLOBAL": 3, "STB_WEAK": 2}.get(sym.bind, 1)
        dot_rank = 1 if sym.name.startswith(".") else 0
        sized = 1 if sym.size else 0
        return bind_rank, dot_rank, sized, len(sym.name)

