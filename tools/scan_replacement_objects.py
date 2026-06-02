#!/usr/bin/env python3
from pathlib import Path
import json
import sys
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

ROOT = Path(__file__).resolve().parents[1]
SHF_EXECINSTR = 0x4

def is_defined(sym):
    return sym["st_shndx"] != "SHN_UNDEF"

def normalize_code_symbol(name: str) -> str:
    # Original .text symbols are dot-prefixed on PPC64 ELFv1.
    if name.startswith("."):
        return name
    return "." + name

def scan_object(path: Path):
    found = {}

    with path.open("rb") as f:
        elf = ELFFile(f)

        sections = list(elf.iter_sections())

        for sec in sections:
            if not isinstance(sec, SymbolTableSection):
                continue

            for sym in sec.iter_symbols():
                name = sym.name
                if not name:
                    continue

                if sym["st_info"]["type"] != "STT_FUNC":
                    continue

                if not is_defined(sym):
                    continue

                shndx = sym["st_shndx"]
                if not isinstance(shndx, int):
                    continue

                target_sec = sections[shndx]
                sec_flags = int(target_sec["sh_flags"])

                # Ignore .opd function descriptors and anything non-code.
                if not (sec_flags & SHF_EXECINSTR):
                    continue

                norm = normalize_code_symbol(name)

                found[norm] = {
                    "object": path.as_posix(),
                    "section": target_sec.name,
                    "symbol": name,
                }

    return found

def main() -> int:
    if len(sys.argv) < 2:
        out_path = ROOT / "build" / "replacement_map.json"
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text("{}\n")
        print(f"wrote {out_path}")
        print("replacement functions: 0")
        return 0

    replacement_map = {}

    for arg in sys.argv[1:]:
        path = Path(arg)
        if not path.exists():
            continue

        funcs = scan_object(path)

        for sym, info in funcs.items():
            if sym in replacement_map:
                old = replacement_map[sym]
                raise RuntimeError(
                    f"duplicate replacement for {sym}:\n"
                    f"  {old['object']}:{old['section']}\n"
                    f"  {info['object']}:{info['section']}"
                )

            replacement_map[sym] = info

    out_path = ROOT / "build" / "replacement_map.json"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(replacement_map, indent=2) + "\n")

    print(f"wrote {out_path}")
    print(f"replacement functions: {len(replacement_map)}")

    for sym, info in sorted(replacement_map.items())[:50]:
        print(f"{sym} -> {info['object']}({info['section']})")

    return 0

if __name__ == "__main__":
    raise SystemExit(main())