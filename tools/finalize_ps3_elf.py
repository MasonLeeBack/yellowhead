#!/usr/bin/env python3
from pathlib import Path
import struct
import sys


EI_OSABI = 7
EI_ABIVERSION = 8


def read_u16_be(data, off):
    return struct.unpack_from(">H", data, off)[0]


def read_u64_be(data, off):
    return struct.unpack_from(">Q", data, off)[0]


def write_u64_be(data, off, value):
    struct.pack_into(">Q", data, off, value)


def validate_elf64_be(data: bytes, name: str) -> None:
    if data[0:4] != b"\x7fELF":
        raise RuntimeError(f"{name}: not an ELF")
    if data[4] != 2:
        raise RuntimeError(f"{name}: not ELF64")
    if data[5] != 2:
        raise RuntimeError(f"{name}: not big-endian")


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: finalize_ps3_elf.py orig/EBOOT.ELF build/raw.elf build/final.elf")
        return 1

    orig_path = Path(sys.argv[1])
    raw_path = Path(sys.argv[2])
    out_path = Path(sys.argv[3])

    orig = bytearray(orig_path.read_bytes())
    raw = bytearray(raw_path.read_bytes())

    validate_elf64_be(orig, str(orig_path))
    validate_elf64_be(raw, str(raw_path))

    orig_e_phoff = read_u64_be(orig, 0x20)
    raw_e_phoff = read_u64_be(raw, 0x20)

    orig_e_phentsize = read_u16_be(orig, 0x36)
    raw_e_phentsize = read_u16_be(raw, 0x36)

    orig_e_phnum = read_u16_be(orig, 0x38)
    raw_e_phnum = read_u16_be(raw, 0x38)

    if orig_e_phentsize != raw_e_phentsize:
        raise RuntimeError(
            f"program header entry size mismatch: "
            f"orig={orig_e_phentsize}, raw={raw_e_phentsize}"
        )

    if orig_e_phnum != raw_e_phnum:
        raise RuntimeError(
            f"program header count mismatch: "
            f"orig={orig_e_phnum}, raw={raw_e_phnum}. "
            f"Check generated PHDRS block first."
        )

    phdr_size = orig_e_phentsize * orig_e_phnum

    # Preserve the rebuilt ELF header/section table.
    # Only copy PS3 runtime identity + program headers.
    raw[EI_OSABI] = orig[EI_OSABI]
    raw[EI_ABIVERSION] = orig[EI_ABIVERSION]

    # The PHDR table is loader-facing and should match the original.
    raw[orig_e_phoff:orig_e_phoff + phdr_size] = orig[
        orig_e_phoff:orig_e_phoff + phdr_size
    ]

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(raw)

    print(f"wrote {out_path}")
    print(f"copied OSABI byte: 0x{orig[EI_OSABI]:02x}")
    print(f"copied ABI version: 0x{orig[EI_ABIVERSION]:02x}")
    print(f"copied {orig_e_phnum} program headers from 0x{orig_e_phoff:x}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())