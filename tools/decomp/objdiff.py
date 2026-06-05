from __future__ import annotations

from pathlib import Path
from dataclasses import dataclass
import json

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
from elftools.elf.sections import SymbolTableSection

from .elf import ElfImage, Symbol, quote_ld_path
from .replacements import run_nm

R_PPC64_REL24 = 10
R_PPC64_ADDR32 = 1
R_PPC64_TOC16 = 47
DW_OP_ADDR = 0x03
NOP = 0x60000000
TOC_RESTORE_R2_40 = 0xE8410028
TOC_SAVE_R2_40 = 0xF8410028


@dataclass(frozen=True)
class ObjdiffBudget:
    symbols: list[str]
    code: int
    data: int


def source_text_symbols(obj: Path, nm: Path) -> dict[str, str]:
    symbols: dict[str, str] = {}
    for line in run_nm(nm, obj):
        parts = line.split()
        if len(parts) < 3:
            continue
        name, kind = parts[0], parts[1]
        if kind.lower() not in {"t", "w"} or name == ".text":
            continue
        if name.startswith(".text."):
            continue
        symbols[name] = section_for_symbol(name)
    return symbols


def section_for_symbol(symbol: str) -> str:
    if symbol.startswith("."):
        return ".text" + symbol
    return ".text." + symbol


def source_section_sizes(obj: Path) -> dict[str, int]:
    with obj.open("rb") as fh:
        elf = ELFFile(fh)
        return {sec.name: int(sec["sh_size"]) for sec in elf.iter_sections()}


def source_section_alignments(obj: Path) -> dict[str, int]:
    with obj.open("rb") as fh:
        elf = ELFFile(fh)
        return {sec.name: int(sec["sh_addralign"]) or 1 for sec in elf.iter_sections()}


def source_section_data(obj: Path) -> dict[str, bytes]:
    with obj.open("rb") as fh:
        elf = ELFFile(fh)
        return {sec.name: sec.data() for sec in elf.iter_sections() if sec["sh_type"] != "SHT_NOBITS"}


def source_bss_symbols(obj: Path) -> list[tuple[str, int, int, str]]:
    with obj.open("rb") as fh:
        elf = ELFFile(fh)
        sections = list(elf.iter_sections())
        out: list[tuple[str, int, int, str]] = []
        for sec in sections:
            if not isinstance(sec, SymbolTableSection):
                continue
            for sym in sec.iter_symbols():
                shndx = sym["st_shndx"]
                if not isinstance(shndx, int) or shndx >= len(sections):
                    continue
                if sections[shndx].name != ".bss":
                    continue
                if sym["st_info"]["type"] != "STT_OBJECT" or not sym.name:
                    continue
                out.append((sym.name, int(sym["st_value"]), int(sym["st_size"]), sym["st_info"]["bind"]))
        return sorted(out, key=lambda item: (item[1], item[0]))


def source_path_for_obj(root: Path, obj: Path) -> str:
    rel = obj.relative_to(root / "build" / "src").with_suffix(".cpp")
    return rel.as_posix().lower()


def dwarf_string(attr) -> str | None:
    if attr is None:
        return None
    value = attr.value
    if isinstance(value, bytes):
        return value.decode("utf-8", "replace")
    return str(value)


def die_ref(cu, attr) -> int:
    if attr.form == "DW_FORM_ref_addr":
        return int(attr.value)
    return cu.cu_offset + int(attr.value)


def die_name(die, dies: dict[int, object], cu) -> str | None:
    name = dwarf_string(die.attributes.get("DW_AT_name"))
    if name:
        return name
    spec = die.attributes.get("DW_AT_specification")
    if spec is None:
        return None
    spec_die = dies.get(die_ref(cu, spec))
    if spec_die is None:
        return None
    return dwarf_string(spec_die.attributes.get("DW_AT_name"))


def die_linkage_name(die, dies: dict[int, object], cu) -> str | None:
    linkage = dwarf_string(die.attributes.get("DW_AT_MIPS_linkage_name"))
    if linkage:
        return linkage
    spec = die.attributes.get("DW_AT_specification")
    if spec is None:
        return None
    spec_die = dies.get(die_ref(cu, spec))
    if spec_die is None:
        return None
    return dwarf_string(spec_die.attributes.get("DW_AT_MIPS_linkage_name"))


def die_addr(die, elf: ELFFile, addr_size: int) -> int | None:
    loc = die.attributes.get("DW_AT_location")
    if loc is None or loc.form not in {"DW_FORM_block1", "DW_FORM_block2", "DW_FORM_block4", "DW_FORM_block"}:
        return None
    data = bytes(loc.value)
    if not data or data[0] != DW_OP_ADDR:
        return None
    if len(data) < 1 + addr_size:
        return None
    return int.from_bytes(data[1:1 + addr_size], "little" if elf.little_endian else "big")


def die_byte_size(die, dies: dict[int, object], cu, seen: set[int] | None = None) -> int | None:
    seen = seen or set()
    if die.offset in seen:
        return None
    seen.add(die.offset)

    size = die.attributes.get("DW_AT_byte_size")
    if size is not None:
        return int(size.value)

    spec_attr = die.attributes.get("DW_AT_specification")
    if spec_attr is not None:
        spec_die = dies.get(die_ref(cu, spec_attr))
        if spec_die is not None:
            spec_size = die_byte_size(spec_die, dies, cu, seen)
            if spec_size is not None:
                return spec_size

    type_attr = die.attributes.get("DW_AT_type")
    if type_attr is not None and die.tag != "DW_TAG_array_type":
        type_die = dies.get(die_ref(cu, type_attr))
        if type_die is not None:
            type_size = die_byte_size(type_die, dies, cu, seen)
            if type_size is not None:
                return type_size

    if die.tag == "DW_TAG_array_type" and type_attr is not None:
        type_die = dies.get(die_ref(cu, type_attr))
        element_size = die_byte_size(type_die, dies, cu, seen) if type_die is not None else None
        if element_size is None:
            return None
        count = 1
        for child in die.iter_children():
            if child.tag != "DW_TAG_subrange_type":
                continue
            upper = child.attributes.get("DW_AT_upper_bound")
            count_attr = child.attributes.get("DW_AT_count")
            if count_attr is not None:
                count *= int(count_attr.value)
            elif upper is not None:
                count *= int(upper.value) + 1
        return element_size * count

    return None


def dwarf_bss_symbols(image: ElfImage, source_rel: str) -> list[tuple[str, int, int]]:
    bss = image.section(".bss")
    cu = dwarf_compile_unit(image, source_rel)
    if cu is None:
        return []

    out: list[tuple[str, int, int]] = []
    dies = {die.offset: die for die in cu.iter_DIEs()}
    addr_size = int(cu["address_size"])
    for die in dies.values():
        if die.tag != "DW_TAG_variable":
            continue
        addr = die_addr(die, image.elf, addr_size)
        if addr is None or not (bss.addr <= addr < bss.end):
            continue
        name = die_linkage_name(die, dies, cu) or die_name(die, dies, cu)
        if not name:
            continue
        size = die_byte_size(die, dies, cu) or 0
        out.append((name, addr, size))
    return sorted(out, key=lambda item: (item[1], item[0]))


def dwarf_bss_addresses(image: ElfImage, source_rel: str) -> dict[str, int]:
    out: dict[str, int] = {}
    cu = dwarf_compile_unit(image, source_rel)
    if cu is not None:
        dies = {die.offset: die for die in cu.iter_DIEs()}
        addr_size = int(cu["address_size"])
        for die in dies.values():
            if die.tag != "DW_TAG_variable":
                continue
            addr = die_addr(die, image.elf, addr_size)
            if addr is None:
                continue
            name = die_name(die, dies, cu)
            if name:
                out[name] = addr
            linkage = die_linkage_name(die, dies, cu)
            if linkage:
                out[linkage] = addr
    return out


def unmangle_last_component(name: str) -> str | None:
    if not name.startswith("_Z"):
        return None
    if name.startswith("_ZZ"):
        last = None
        for start in range(3, len(name)):
            if name[start - 1] != "E" or not name[start].isdigit():
                continue
            i = start
            while i < len(name) and name[i].isdigit():
                i += 1
            length = int(name[start:i])
            component = name[i:i + length]
            if len(component) == length:
                last = component
        if last is not None:
            return last
    i = 2
    if i < len(name) and name[i] == "N":
        i += 1
    last = None
    while i < len(name) and name[i].isdigit():
        start = i
        while i < len(name) and name[i].isdigit():
            i += 1
        length = int(name[start:i])
        component = name[i:i + length]
        if len(component) != length:
            return None
        last = component
        i += length
    return last


def dwarf_address_for_source_symbol(addresses: dict[str, int], name: str) -> int | None:
    addr = addresses.get(name)
    if addr is not None:
        return addr
    last_component = unmangle_last_component(name)
    if last_component is not None:
        return addresses.get(last_component)
    return None


def dwarf_compile_unit(image: ElfImage, source_rel: str):
    cache = getattr(image, "_objdiff_dwarf_cu_by_source", None)
    if cache is None:
        cache = {}
        dwarf = image.elf.get_dwarf_info()
        for cu in dwarf.iter_CUs():
            top = cu.get_top_DIE()
            name = dwarf_string(top.attributes.get("DW_AT_name")) or ""
            normalized = name.replace("\\", "/").lower()
            if normalized:
                cache[normalized] = cu
        setattr(image, "_objdiff_dwarf_cu_by_source", cache)

    for name, cu in cache.items():
        if name.endswith(source_rel):
            return cu
    return None


def high_pc_addr(die) -> int | None:
    low = die.attributes.get("DW_AT_low_pc")
    high = die.attributes.get("DW_AT_high_pc")
    if low is None or high is None:
        return None
    if high.form == "DW_FORM_addr":
        return int(high.value)
    return int(low.value) + int(high.value)


def dwarf_subprogram_symbols(image: ElfImage, source_rel: str) -> dict[str, Symbol]:
    text = image.section(".text")
    cu = dwarf_compile_unit(image, source_rel)
    if cu is None:
        return {}

    out: dict[str, Symbol] = {}
    dies = {die.offset: die for die in cu.iter_DIEs()}
    for die in dies.values():
        if die.tag != "DW_TAG_subprogram":
            continue
        low = die.attributes.get("DW_AT_low_pc")
        if low is None:
            continue
        addr = int(low.value)
        high = high_pc_addr(die)
        if high is None or high <= addr or not (text.addr <= addr < text.end):
            continue
        size = high - addr

        keys: list[str] = []
        linkage = die_linkage_name(die, dies, cu)
        if linkage:
            keys.append(f".{linkage}")
        name = die_name(die, dies, cu)
        if name:
            if name.startswith("_GLOBAL__"):
                keys.append(f".{name}")
            elif name == "__static_initialization_and_destruction_0":
                keys.append("._Z41__static_initialization_and_destruction_0ii")
        for key in keys:
            out[key] = Symbol(key, addr, size, "STT_FUNC", "STB_LOCAL", ".text")
    return out


def original_bss_symbols_from_dwarf(
    image: ElfImage,
    root: Path,
    obj: Path,
    source_symbols: list[tuple[str, int, int, str]],
) -> tuple[int, list[tuple[str, int, int, str]]] | None:
    if not source_symbols:
        return None
    addresses = dwarf_bss_addresses(image, source_path_for_obj(root, obj))
    if not addresses:
        return None

    source_offsets = {name: offset for name, offset, _sym_size, _bind in source_symbols}

    mapped: list[tuple[str, int, int, str, int]] = []
    for name, _offset, sym_size, bind in source_symbols:
        addr = inferred_bss_address(addresses, source_offsets, name, _offset)
        if addr is None:
            return None
        mapped.append((name, addr, sym_size, bind, _offset))

    base = min(addr for _name, addr, _size, _bind, _offset in mapped)
    size = max(addr - base + sym_size for _name, addr, sym_size, _bind, _offset in mapped)
    symbols = [(name, addr - base, sym_size, bind) for name, addr, sym_size, bind, _offset in mapped]
    return size, sorted(symbols, key=lambda item: (item[1], item[0]))


def inferred_bss_address(addresses: dict[str, int], source_offsets: dict[str, int], name: str, offset: int) -> int | None:
    addr = dwarf_address_for_source_symbol(addresses, name)
    if addr is not None:
        return addr
    if name.startswith("_ZGVZ"):
        guarded_name = "_Z" + name[4:]
        guarded_addr = dwarf_address_for_source_symbol(addresses, guarded_name)
        guarded_offset = source_offsets.get(guarded_name)
        if guarded_addr is not None and guarded_offset is not None and guarded_offset > offset:
            return guarded_addr - (guarded_offset - offset)
    return None


def source_relocations(obj: Path) -> dict[str, dict[int, tuple[int, str, int]]]:
    with obj.open("rb") as fh:
        elf = ELFFile(fh)
        sections = list(elf.iter_sections())
        out: dict[str, dict[int, tuple[int, str, int]]] = {}
        for sec in sections:
            if not isinstance(sec, RelocationSection):
                continue
            target = sections[sec["sh_info"]].name
            symtab = sections[sec["sh_link"]]
            if not isinstance(symtab, SymbolTableSection):
                continue
            rels: dict[int, tuple[int, str, int]] = {}
            for rel in sec.iter_relocations():
                sym = symtab.get_symbol(rel["r_info_sym"])
                sym_name = sym.name
                shndx = sym["st_shndx"]
                if not sym_name and isinstance(shndx, int) and shndx < len(sections):
                    sym_name = sections[shndx].name
                addend = int(rel["r_addend"]) if rel.is_RELA() else 0
                rels[int(rel["r_offset"])] = (int(rel["r_info_type"]), sym_name, addend)
            out[target] = rels
        return out


def sign_extend(value: int, bits: int) -> int:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def branch_target(word: int, pc: int) -> int | None:
    if (word >> 26) != 18:
        return None
    disp = sign_extend(word & 0x03fffffc, 26)
    if word & 2:
        return disp
    return pc + disp


def inferred_branch_symbol(word: int, pc: int, branch_symbols: dict[int, str]) -> str | None:
    target = branch_target(word, pc)
    if target is None:
        return None
    return branch_symbols.get(target)


def emit_bytes(
    out,
    data: bytes,
    relocs: dict[int, tuple[int, str, int]] | None = None,
    *,
    addr: int = 0,
    branch_symbols: dict[int, str] | None = None,
) -> None:
    relocs = relocs or {}
    branch_symbols = branch_symbols or {}
    whole = len(data) & ~3
    for i in range(0, whole, 4):
        word = int.from_bytes(data[i:i + 4], "big")
        prev_branch_reloc = relocs.get(i - 4)
        branch_reloc = relocs.get(i)
        toc_reloc = relocs.get(i + 2)
        prev_inferred_branch = None
        if i >= 4:
            prev_word = int.from_bytes(data[i - 4:i], "big")
            prev_inferred_branch = inferred_branch_symbol(prev_word, addr + i - 4, branch_symbols)
        if (
            i >= 4
            and word == TOC_RESTORE_R2_40
            and (
                (prev_branch_reloc and prev_branch_reloc[0] == R_PPC64_REL24)
                or prev_inferred_branch is not None
            )
        ):
            out.write(f"    .4byte 0x{NOP:08x}\n")
            continue
        inferred_symbol = inferred_branch_symbol(word, addr + i, branch_symbols)
        if inferred_symbol is not None:
            mnemonic = "bl" if (word & 1) else "b"
            out.write(f"    {mnemonic} {inferred_symbol}\n")
            continue
        if branch_reloc and branch_reloc[0] == R_PPC64_REL24 and (word >> 26) == 18:
            _kind, sym, addend = branch_reloc
            suffix = f"+{addend}" if addend else ""
            mnemonic = "bl" if (word & 1) else "b"
            out.write(f"    {mnemonic} {sym}{suffix}\n")
            continue
        if toc_reloc and toc_reloc[0] == R_PPC64_TOC16:
            if emit_toc_instruction(out, word, toc_reloc):
                continue
        out.write(f"    .4byte 0x{word:08x}\n")
    for byte in data[whole:]:
        out.write(f"    .byte 0x{byte:02x}\n")


def emit_toc_instruction(out, word: int, reloc: tuple[int, str, int]) -> bool:
    _kind, sym, addend = reloc
    opcode = word >> 26
    rt = (word >> 21) & 31
    ra = (word >> 16) & 31
    target = sym + (f"+{addend}" if addend else "")

    if opcode == 32:
        out.write(f"    lwz {rt}, {target}@toc({ra})\n")
        return True
    if opcode == 48:
        out.write(f"    lfs {rt}, {target}@toc({ra})\n")
        return True
    if opcode == 58 and (word & 0x3) == 0:
        out.write(f"    ld {rt}, {target}@toc({ra})\n")
        return True
    return False


def emit_data_words(out, data: bytes, relocs: dict[int, tuple[int, str, int]] | None = None) -> None:
    relocs = relocs or {}
    whole = len(data) & ~3
    for i in range(0, whole, 4):
        reloc = relocs.get(i)
        if reloc and reloc[0] == R_PPC64_ADDR32:
            _kind, sym, addend = reloc
            suffix = f"+{addend}" if addend else ""
            out.write(f"    .4byte {sym}{suffix}\n")
        else:
            word = int.from_bytes(data[i:i + 4], "big")
            out.write(f"    .4byte 0x{word:08x}\n")
    for byte in data[whole:]:
        out.write(f"    .byte 0x{byte:02x}\n")


def should_emit_data_section(name: str, data: bytes) -> bool:
    if not data:
        return False
    if name == ".toc" or name.startswith(".rodata") or name.startswith(".data"):
        return True
    return False


def original_symbol_map(image: ElfImage) -> dict[str, Symbol]:
    return {sym.name: sym for sym in image.text_functions()}


def original_branch_symbol_map(image: ElfImage) -> dict[int, str]:
    text = image.section(".text")
    out = {sym.addr: sym.name for sym in image.text_functions()}
    for sym in image.symbols:
        if not sym.name or sym.name in {".text", ".init", ".fini"}:
            continue
        if text.addr <= sym.addr < text.end and sym.addr not in out:
            out[sym.addr] = sym.name
    return out


def original_stub_symbol_map(image: ElfImage, branch_symbols: dict[int, str]) -> dict[int, str]:
    text = image.section(".text")
    out: dict[int, str] = {}
    for addr in range(text.addr, text.end - 16, 4):
        offset = text.offset + (addr - text.addr)
        data = image.read_at_offset(offset, 16)
        words = [int.from_bytes(data[i:i + 4], "big") for i in range(0, 16, 4)]
        if words[0] != TOC_SAVE_R2_40:
            continue
        target = branch_target(words[3], addr + 12)
        if target is not None and target in branch_symbols:
            out[addr] = branch_symbols[target]
    return out


def emit_bss(out, size: int, align: int, symbols: list[tuple[str, int, int, str]]) -> None:
    out.write('.section .bss, "aw", @nobits\n')
    if align > 1:
        out.write(f"    .balign {align}\n")

    cur = 0
    for name, offset, sym_size, bind in symbols:
        if offset < cur or offset >= size:
            continue
        if offset > cur:
            out.write(f"    .space {offset - cur}\n")
        if bind == "STB_GLOBAL":
            out.write(f".global {name}\n")
        else:
            out.write(f".local {name}\n")
        out.write(f".type {name}, @object\n")
        out.write(f".size {name}, {sym_size}\n")
        out.write(f"{name}:\n")
        if sym_size:
            out.write(f"    .space {sym_size}\n")
        cur = offset + sym_size

    if cur < size:
        out.write(f"    .space {size - cur}\n")
    out.write("\n")


def write_original_object_asm(
    image: ElfImage,
    root: Path,
    source_obj: Path,
    asm_path: Path,
    nm: Path,
    original_symbols: dict[str, Symbol],
    original_branch_symbols: dict[int, str],
    original_stub_symbols: dict[int, str],
) -> ObjdiffBudget:
    text = image.section(".text")
    symbols = source_text_symbols(source_obj, nm)
    orig = dict(original_symbols)
    branch_symbols = dict(original_branch_symbols)
    dwarf_symbols = dwarf_subprogram_symbols(image, source_path_for_obj(root, source_obj))
    orig.update(dwarf_symbols)
    for symbol in dwarf_symbols:
        symbols.setdefault(symbol, section_for_symbol(symbol))
    branch_symbols.update({sym.addr: sym.name for sym in orig.values()})
    branch_symbols.update(original_stub_symbols)
    relocs = source_relocations(source_obj)
    sizes = source_section_sizes(source_obj)
    aligns = source_section_alignments(source_obj)
    section_data = source_section_data(source_obj)
    bss_symbols = source_bss_symbols(source_obj)
    dwarf_bss = original_bss_symbols_from_dwarf(image, root, source_obj, bss_symbols)
    emitted: list[str] = []
    code_size = 0
    data_size = 0

    asm_path.parent.mkdir(parents=True, exist_ok=True)
    with asm_path.open("w") as out:
        out.write("/* Auto-generated original-side objdiff object. */\n\n")
        bss_size = sizes.get(".bss", 0)
        if dwarf_bss is not None:
            bss_size, bss_symbols = dwarf_bss
        if bss_size:
            emit_bss(out, bss_size, aligns.get(".bss", 1), bss_symbols)
            data_size += bss_size
        for section_name, data in sorted(section_data.items()):
            if not should_emit_data_section(section_name, data):
                continue
            data_size += sizes.get(section_name, len(data))
            flags = '"aw"' if section_name in {".toc", ".data"} or section_name.startswith(".data.") else '"a"'
            out.write(f'.section {section_name}, {flags}, @progbits\n')
            if aligns.get(section_name, 1) > 1:
                out.write(f"    .balign {aligns[section_name]}\n")
            emit_data_words(out, data, relocs.get(section_name))
            out.write("\n")
        for symbol, section in sorted(symbols.items()):
            orig_sym = orig.get(symbol)
            if orig_sym is None or orig_sym.size <= 0:
                continue
            offset = text.offset + (orig_sym.addr - text.addr)
            data = image.read_at_offset(offset, orig_sym.size)
            out.write(f'.section {section}, "ax", @progbits\n')
            out.write(f".global {symbol}\n")
            out.write(f"{symbol}:\n")
            emit_bytes(out, data, relocs.get(section), addr=orig_sym.addr, branch_symbols=branch_symbols)
            out.write("\n")
            emitted.append(symbol)
            code_size += orig_sym.size
    return ObjdiffBudget(emitted, code_size, data_size)


def build_objdiff_config(root: Path, source_objs: list[Path]) -> dict[str, object]:
    units = []
    for obj in source_objs:
        obj = (root / obj).resolve() if not obj.is_absolute() else obj.resolve()
        rel = obj.relative_to(root / "build" / "src")
        orig_obj = root / "build" / "objdiff" / "orig" / rel
        units.append(
            {
                "name": rel.as_posix(),
                "target_path": orig_obj.relative_to(root).as_posix(),
                "base_path": obj.relative_to(root).as_posix(),
            }
        )

    return {
        "custom_make": "make",
        "build_base": True,
        "build_target": False,
        "watch_patterns": [
            "src/**/*.cpp",
            "src/**/*.c",
            "src/**/*.h",
            "include/**/*.h",
            "tools/decomp/**/*.py",
            "Makefile",
            "replacements.json",
        ],
        "ignore_patterns": [
            "build/**/*",
            "orig/**/*",
            "sdk/**/*",
            ".venv/**/*",
            "asm/**/*",
            "analysis/**/*",
        ],
        "units": units,
    }


def write_objdiff(root: Path, image: ElfImage, source_objs: list[Path]) -> None:
    nm = root / "tools" / "ppu-lv2-nm"
    targets = {}
    original_symbols = original_symbol_map(image)
    original_branch_symbols = original_branch_symbol_map(image)
    original_stub_symbols = original_stub_symbol_map(image, original_branch_symbols)
    for obj in source_objs:
        obj = (root / obj).resolve() if not obj.is_absolute() else obj.resolve()
        rel = obj.relative_to(root / "build" / "src")
        asm_path = root / "build" / "objdiff" / "orig" / rel.with_suffix(".s")
        budget = write_original_object_asm(
            image,
            root,
            obj,
            asm_path,
            nm,
            original_symbols,
            original_branch_symbols,
            original_stub_symbols,
        )
        targets[obj.relative_to(root).as_posix()] = {
            "asm": asm_path.relative_to(root).as_posix(),
            "object": (root / "build" / "objdiff" / "orig" / rel).relative_to(root).as_posix(),
            "symbols": budget.symbols,
        }

    out_dir = root / "build" / "objdiff"
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "targets.json").write_text(json.dumps(targets, indent=2, sort_keys=True) + "\n")
    (root / "objdiff.json").write_text(json.dumps(build_objdiff_config(root, source_objs), indent=2) + "\n")
