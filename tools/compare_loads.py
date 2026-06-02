#!/usr/bin/env python3
from pathlib import Path
import sys
from elftools.elf.elffile import ELFFile

def load_segments(path: Path):
    with path.open("rb") as f:
        elf = ELFFile(f)
        result = []

        for seg in elf.iter_segments():
            if seg["p_type"] != "PT_LOAD":
                continue

            filesz = int(seg["p_filesz"])
            memsz = int(seg["p_memsz"])

            if filesz == 0 and memsz == 0:
                continue

            result.append({
                "vaddr": int(seg["p_vaddr"]),
                "offset": int(seg["p_offset"]),
                "filesz": filesz,
                "memsz": memsz,
            })

        return result

def read_vaddr(path: Path, vaddr: int, size: int) -> bytes:
    with path.open("rb") as f:
        elf = ELFFile(f)

        for seg in elf.iter_segments():
            if seg["p_type"] != "PT_LOAD":
                continue

            sv = int(seg["p_vaddr"])
            so = int(seg["p_offset"])
            sf = int(seg["p_filesz"])

            if sv <= vaddr and (vaddr + size) <= (sv + sf):
                f.seek(so + (vaddr - sv))
                return f.read(size)

    raise RuntimeError(f"{path}: cannot map vaddr=0x{vaddr:x}, size=0x{size:x}")

def first_diff(a: bytes, b: bytes):
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return i, x, y

    if len(a) != len(b):
        return min(len(a), len(b)), None, None

    return None

def mapped_header_size(path: Path) -> int:
    with path.open("rb") as f:
        elf = ELFFile(f)
        return int(elf.header["e_phoff"]) + int(elf.header["e_phentsize"]) * int(elf.header["e_phnum"])

def main() -> int:
    if len(sys.argv) != 3:
        print("usage: compare_loads.py orig/EBOOT.ELF build/EBOOT.relinked.elf")
        return 1

    orig = Path(sys.argv[1])
    rebuilt = Path(sys.argv[2])

    ok = True

    header_skip = mapped_header_size(orig)

    for i, seg in enumerate(load_segments(orig)):
        vaddr = seg["vaddr"]
        size = seg["filesz"]

        if size == 0:
            continue

        # First PT_LOAD maps the ELF header/PHDR table.
        # Rebuilt ELF header metadata may legitimately differ.
        if seg["offset"] == 0:
            skip = header_skip
            print(f"LOAD {i}: skipping mapped ELF header area size=0x{skip:x}")
            vaddr += skip
            size -= skip

        a = read_vaddr(orig, vaddr, size)
        b = read_vaddr(rebuilt, vaddr, size)

        diff = first_diff(a, b)
        if diff is None:
            print(f"LOAD {i}: OK vaddr=0x{vaddr:x} size=0x{size:x}")
        else:
            off, x, y = diff
            print(
                f"LOAD {i}: MISMATCH at vaddr=0x{vaddr + off:x} "
                f"orig={x!r} rebuilt={y!r}"
            )
            ok = False

    if ok:
        print("all original LOAD bytes match in rebuilt ELF")
        return 0

    return 2

if __name__ == "__main__":
    raise SystemExit(main())