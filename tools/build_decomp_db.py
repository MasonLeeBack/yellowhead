#!/usr/bin/env python3
from pathlib import Path
import json
import re
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

ROOT = Path(__file__).resolve().parents[1]

def hx(x): return f"0x{x:x}"

def sanitize_path(path: str) -> str:
    path = path.replace("\\", "/")
    path = re.sub(r"^[A-Za-z]:/", "", path)
    path = path.lstrip("/")
    return path

def high_pc_value(die, low_pc: int) -> int | None:
    attr = die.attributes.get("DW_AT_high_pc")
    if not attr:
        return None

    # DWARF4: high_pc may be constant offset, older may be address.
    if attr.form.startswith("DW_FORM_addr"):
        return int(attr.value)

    return low_pc + int(attr.value)

def get_text_section(elf):
    sec = elf.get_section_by_name(".text")
    return {
        "addr": int(sec["sh_addr"]),
        "offset": int(sec["sh_offset"]),
        "size": int(sec["sh_size"]),
    }

def collect_symtab_functions(elf, text):
    text_start = text["addr"]
    text_end = text_start + text["size"]
    funcs = {}

    for sec in elf.iter_sections():
        if not isinstance(sec, SymbolTableSection):
            continue

        for sym in sec.iter_symbols():
            if sym["st_info"]["type"] != "STT_FUNC":
                continue

            name = sym.name
            if not name or name in {".text", ".init", ".fini"}:
                continue

            addr = int(sym["st_value"])
            size = int(sym["st_size"])

            if not (text_start <= addr < text_end):
                continue

            # Keep dot-prefixed code-entry symbols.
            if not name.startswith("."):
                continue

            funcs[addr] = {
                "addr": addr,
                "size": size,
                "symbol": name,
            }

    return funcs

def infer_function_sizes(funcs, text):
    addrs = sorted(funcs)
    text_end = text["addr"] + text["size"]

    for i, addr in enumerate(addrs):
        next_addr = addrs[i + 1] if i + 1 < len(addrs) else text_end
        size = funcs[addr]["size"]

        if size <= 0 or addr + size > text_end:
            size = next_addr - addr

        funcs[addr]["size"] = size
        funcs[addr]["offset"] = text["offset"] + (addr - text["addr"])

    return funcs

def collect_dwarf_cu_ranges(elf):
    if not elf.has_dwarf_info():
        return []

    dwarf = elf.get_dwarf_info()
    rows = []

    for cu in dwarf.iter_CUs():
        top = cu.get_top_DIE()

        name_attr = top.attributes.get("DW_AT_name")
        comp_attr = top.attributes.get("DW_AT_comp_dir")

        cu_name = name_attr.value.decode(errors="replace") if name_attr else "unknown.cpp"
        comp_dir = comp_attr.value.decode(errors="replace") if comp_attr else ""

        source_path = sanitize_path(cu_name)
        if comp_dir and not source_path.startswith(comp_dir):
            # Keep this simple for now. Later we can strip original project root.
            pass

        for die in cu.iter_DIEs():
            if die.tag != "DW_TAG_subprogram":
                continue

            low_attr = die.attributes.get("DW_AT_low_pc")
            if not low_attr:
                continue

            low = int(low_attr.value)
            high = high_pc_value(die, low)
            if high is None or high <= low:
                continue

            rows.append({
                "low": low,
                "high": high,
                "cu_name": cu_name,
                "comp_dir": comp_dir,
                "source_path": source_path,
            })

    rows.sort(key=lambda r: (r["low"], r["high"]))
    return rows

def find_cu_for_addr(cu_ranges, addr):
    # First exact containing range.
    for r in cu_ranges:
        if r["low"] <= addr < r["high"]:
            return r

    return None

def main():
    elf_path = ROOT / "orig" / "EBOOT.ELF"

    with elf_path.open("rb") as f:
        elf = ELFFile(f)
        text = get_text_section(elf)
        funcs = infer_function_sizes(collect_symtab_functions(elf, text), text)
        cu_ranges = collect_dwarf_cu_ranges(elf)

    sources = {}
    unknown = sources.setdefault("__unknown__/unknown.cpp", {
        "cu_name": "unknown.cpp",
        "comp_dir": "",
        "source_path": "__unknown__/unknown.cpp",
        "functions": [],
        "opd": [],
        "objects": [],
    })

    for addr in sorted(funcs):
        fn = funcs[addr]
        cu = find_cu_for_addr(cu_ranges, addr)

        if cu:
            source_path = sanitize_path(cu["source_path"])
            group = sources.setdefault(source_path, {
                "cu_name": cu["cu_name"],
                "comp_dir": cu["comp_dir"],
                "source_path": source_path,
                "functions": [],
                "opd": [],
                "objects": [],
            })
        else:
            group = unknown

        group["functions"].append({
            "addr": hx(fn["addr"]),
            "size": hx(fn["size"]),
            "offset": hx(fn["offset"]),
            "symbol": fn["symbol"],
        })

    db = {
        "text": {
            "addr": hx(text["addr"]),
            "size": hx(text["size"]),
            "offset": hx(text["offset"]),
        },
        "sources": sources,
    }

    out = ROOT / "build" / "decomp_db.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(db, indent=2) + "\n")

    print(f"wrote {out}")
    print(f"sources: {len(sources)}")
    print(f"functions: {sum(len(s['functions']) for s in sources.values())}")

if __name__ == "__main__":
    main()