#!/usr/bin/env python3
from pathlib import Path
import re
import sys
import json
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

ROOT = Path(__file__).resolve().parents[1]

IGNORE_NAMES = {
    ".text",
    ".init",
    ".fini",
}

def sanitize(name: str) -> str:
    # Keep the address as the real identity; name is just for readability.
    s = name.lstrip(".")
    s = re.sub(r"[^A-Za-z0-9_]+", "_", s)
    if not s:
        s = "unnamed"
    if s[0].isdigit():
        s = "_" + s
    return s[:120]

def get_text_section(elf: ELFFile):
    sec = elf.get_section_by_name(".text")
    if sec is None:
        raise RuntimeError("no .text section found")

    return {
        "addr": int(sec["sh_addr"]),
        "offset": int(sec["sh_offset"]),
        "size": int(sec["sh_size"]),
    }

def symbol_binding_rank(sym) -> int:
    bind = sym["st_info"]["bind"]
    if bind == "STB_GLOBAL":
        return 3
    if bind == "STB_WEAK":
        return 2
    return 1

def load_replacement_map() -> dict:
    path = ROOT / "build" / "replacement_map.json"
    if not path.exists():
        return {}
    return json.loads(path.read_text())

def collect_text_symbols(elf: ELFFile, text):
    text_start = text["addr"]
    text_end = text["addr"] + text["size"]

    by_addr = {}

    for sec in elf.iter_sections():
        if not isinstance(sec, SymbolTableSection):
            continue

        for sym in sec.iter_symbols():
            name = sym.name
            if not name:
                continue

            value = int(sym["st_value"])
            size = int(sym["st_size"])
            typ = sym["st_info"]["type"]

            if typ != "STT_FUNC":
                continue

            if not (text_start <= value < text_end):
                continue

            # PS3/PPC64 code-entry symbols are usually dot-prefixed.
            # Keep them. Ignore section aliases like ".text".
            if name in IGNORE_NAMES:
                continue

            current = by_addr.get(value)
            candidate = {
                "addr": value,
                "name": name,
                "size": size,
                "bind_rank": symbol_binding_rank(sym),
            }

            if current is None:
                by_addr[value] = candidate
                continue

            # Prefer global/weak names, then names with a real size, then longer/more descriptive names.
            cur_score = (
                current["bind_rank"],
                1 if current["size"] else 0,
                len(current["name"]),
            )
            cand_score = (
                candidate["bind_rank"],
                1 if candidate["size"] else 0,
                len(candidate["name"]),
            )

            if cand_score > cur_score:
                by_addr[value] = candidate

    symbols = list(by_addr.values())
    symbols.sort(key=lambda x: x["addr"])
    return symbols

def build_items(text, symbols):
    text_start = text["addr"]
    text_end = text["addr"] + text["size"]
    text_file_off = text["offset"]

    items = []
    cur = text_start

    for i, sym in enumerate(symbols):
        addr = sym["addr"]

        if addr < cur:
            # Duplicate/interior symbol. Skip for now.
            continue

        if addr > cur:
            items.append({
                "kind": "gap",
                "addr": cur,
                "size": addr - cur,
                "name": f".text_gap_{cur:08x}",
                "source": "",
            })

        next_addr = symbols[i + 1]["addr"] if i + 1 < len(symbols) else text_end

        # Prefer st_size if it is sane; otherwise infer from next symbol.
        if sym["size"] > 0 and addr + sym["size"] <= text_end:
            end = addr + sym["size"]
        else:
            end = next_addr

        if end < addr:
            continue

        items.append({
            "kind": "func",
            "addr": addr,
            "size": end - addr,
            "name": f".text_func_{addr:08x}_{sanitize(sym['name'])}",
            "source": sym["name"],
        })

        cur = end

    if cur < text_end:
        items.append({
            "kind": "gap",
            "addr": cur,
            "size": text_end - cur,
            "name": f".text_gap_{cur:08x}",
            "source": "",
        })

    # Fill file offsets.
    for item in items:
        item["offset"] = text_file_off + (item["addr"] - text_start)

    return items

def read_bytes(path: Path, offset: int, size: int) -> bytes:
    with path.open("rb") as f:
        f.seek(offset)
        return f.read(size)

def emit_ppc_words(out, blob: bytes):
    if len(blob) % 4 != 0:
        # Should not happen for .text, but preserve exact bytes if it does.
        whole = len(blob) & ~3
    else:
        whole = len(blob)

    for i in range(0, whole, 4):
        word = int.from_bytes(blob[i:i + 4], "big")
        out.write(f"    .4byte 0x{word:08x}\n")

    for b in blob[whole:]:
        out.write(f"    .byte 0x{b:02x}\n")

def ld_quote_path(path: str) -> str:
    path = path.replace("\\", "/")
    return '"' + path.replace('"', '\\"') + '"'


def write_outputs(elf_path: Path, text, symbols, items):
    build_dir = ROOT / "build"
    asm_dir = ROOT / "asm" / "text"
    build_dir.mkdir(parents=True, exist_ok=True)
    asm_dir.mkdir(parents=True, exist_ok=True)

    tsv_path = build_dir / "text_functions.tsv"
    asm_path = asm_dir / "text_blobs.s"
    ldinc_path = build_dir / "text_layout.ldinc"

    replacement_map = load_replacement_map()

    with tsv_path.open("w") as out:
        out.write("kind\taddr\tsize\toffset\tsection\tsymbol\n")
        for item in items:
            out.write(
                f"{item['kind']}\t"
                f"0x{item['addr']:x}\t"
                f"0x{item['size']:x}\t"
                f"0x{item['offset']:x}\t"
                f"{item['name']}\t"
                f"{item['source']}\n"
            )

    # Emit fallback text blobs for anything not implemented by src/**/*.cpp.
    with asm_path.open("w") as out:
        out.write("/* Auto-generated by prepare_text_split.py. Do not edit. */\n")
        out.write("/* PowerPC instructions are emitted as .4byte for byte-exact fallback. */\n\n")

        for item in items:
            if item["kind"] == "func" and item["source"] in replacement_map:
                # Implemented by source tree; don't emit fallback.
                continue

            out.write(f'.section {item["name"]}, "ax", @progbits\n')
            out.write(f'.global __{sanitize(item["name"])}_start\n')
            out.write(f'__{sanitize(item["name"])}_start:\n')

            if item["size"]:
                blob = read_bytes(elf_path, item["offset"], item["size"])
                emit_ppc_words(out, blob)

            out.write(f'.global __{sanitize(item["name"])}_end\n')
            out.write(f'__{sanitize(item["name"])}_end:\n\n')

    # Emit the exact .text layout. Fallback chunks and source replacements are
    # interleaved in original address order.
    with ldinc_path.open("w") as out:
        out.write("/* Auto-generated text layout include. */\n")

        text_start = text["addr"]

        for item in items:
            addr = item["addr"]
            end = item["addr"] + item["size"]

            if item["kind"] == "func" and item["source"] in replacement_map:
                repl = replacement_map[item["source"]]
                obj = ld_quote_path(repl["object"])
                sec = repl["section"]

                rel_start = addr - text_start
                rel_end = end - text_start

                out.write(f"    . = 0x{rel_start:x};\n")
                out.write(f"    __slot_{addr:08x}_start = .;\n")
                out.write(f"    KEEP({obj}({sec}))\n")
                out.write(f"    __slot_{addr:08x}_end = .;\n")
                out.write(f"    . = 0x{rel_end:x};\n")
            else:
                out.write(f"    KEEP(*({item['name']}))\n")

    print(f"text start: 0x{text['addr']:x}")
    print(f"text size:  0x{text['size']:x}")
    print(f"symbols:    {len(symbols)}")
    print(f"items:      {len(items)}")
    print(f"replacements: {len(replacement_map)}")
    print(f"wrote {tsv_path}")
    print(f"wrote {asm_path}")
    print(f"wrote {ldinc_path}")

def main() -> int:
    if len(sys.argv) != 2:
        print("usage: prepare_text_split.py orig/EBOOT.ELF")
        return 1

    elf_path = Path(sys.argv[1])

    with elf_path.open("rb") as f:
        elf = ELFFile(f)
        text = get_text_section(elf)
        symbols = collect_text_symbols(elf, text)
        items = build_items(text, symbols)

    if not symbols:
        raise RuntimeError("no .text function symbols found")

    write_outputs(elf_path, text, symbols, items)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())