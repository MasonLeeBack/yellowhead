#!/usr/bin/env python3
from pathlib import Path
import sys
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[1]

def gas_flags(p_flags: int) -> str:
    # ELF PF_X=1, PF_W=2, PF_R=4
    flags = "a"
    if p_flags & 0x1:
        flags += "x"
    if p_flags & 0x2:
        flags += "w"
    return flags

def main() -> int:
    if len(sys.argv) != 2:
        print("usage: prepare_segments.py orig/EBOOT.ELF")
        return 1

    elf_path = Path(sys.argv[1])
    asm_path = ROOT / "asm" / "blob_segments.s"
    ld_path = ROOT / "build" / "linker.ld"

    asm_path.parent.mkdir(parents=True, exist_ok=True)
    ld_path.parent.mkdir(parents=True, exist_ok=True)

    loads = []

    with elf_path.open("rb") as f:
        elf = ELFFile(f)
        entry = int(elf.header["e_entry"])

        for seg in elf.iter_segments():
            if seg["p_type"] != "PT_LOAD":
                continue

            filesz = int(seg["p_filesz"])
            memsz = int(seg["p_memsz"])

            # Your ELF has a few zero-size LOADs at vaddr 0. Ignore those.
            if filesz == 0 and memsz == 0:
                continue

            loads.append({
                "offset": int(seg["p_offset"]),
                "vaddr": int(seg["p_vaddr"]),
                "paddr": int(seg["p_paddr"]),
                "filesz": filesz,
                "memsz": memsz,
                "flags": int(seg["p_flags"]),
                "align": int(seg["p_align"]),
            })

    if not loads:
        raise RuntimeError("no non-empty PT_LOAD segments found")

    with asm_path.open("w") as out:
        out.write("/* Auto-generated. Do not edit. */\n\n")

        for i, seg in enumerate(loads):
            flags = gas_flags(seg["flags"])
            out.write(f'.section .seg{i}, "{flags}", @progbits\n')
            out.write(f'.global __seg{i}_start\n')
            out.write(f'__seg{i}_start:\n')

            if seg["filesz"]:
                out.write(
                    f'    .incbin "{elf_path.as_posix()}", '
                    f'{seg["offset"]}, {seg["filesz"]}\n'
                )

            out.write(f'.global __seg{i}_end\n')
            out.write(f'__seg{i}_end:\n\n')

    with ld_path.open("w") as out:
        out.write("/* Auto-generated. Do not edit. */\n")
        out.write('OUTPUT_FORMAT("elf64-powerpc")\n')
        out.write("OUTPUT_ARCH(powerpc:common64)\n")
        out.write(f"PROVIDE(__entry = 0x{entry:x});\n")
        out.write("ENTRY(__entry)\n\n")

        out.write("PHDRS\n{\n")
        for i, seg in enumerate(loads):
            out.write(f"  seg{i} PT_LOAD FLAGS(0x{seg['flags']:x});\n")
        out.write("}\n\n")

        out.write("SECTIONS\n{\n")
        for i, seg in enumerate(loads):
            vaddr = seg["vaddr"]
            filesz = seg["filesz"]
            memsz = seg["memsz"]

            out.write(f"  . = 0x{vaddr:x};\n")
            out.write(f"  .seg{i} 0x{vaddr:x} : AT(0x{vaddr:x})\n")
            out.write("  {\n")
            out.write(f"    KEEP(*(.seg{i}))\n")
            out.write(f"  }} :seg{i}\n\n")

            if memsz > filesz:
                bss_start = vaddr + filesz
                bss_size = memsz - filesz

                out.write(f"  .seg{i}_noload 0x{bss_start:x} (NOLOAD) : AT(0x{bss_start:x})\n")
                out.write("  {\n")
                out.write(f"    . += 0x{bss_size:x};\n")
                out.write(f"  }} :seg{i}\n\n")

        out.write("}\n")

    print(f"wrote {asm_path}")
    print(f"wrote {ld_path}")
    for i, seg in enumerate(loads):
        print(
            f"seg{i}: off=0x{seg['offset']:x} "
            f"vaddr=0x{seg['vaddr']:x} "
            f"filesz=0x{seg['filesz']:x} "
            f"memsz=0x{seg['memsz']:x} "
            f"flags=0x{seg['flags']:x}"
        )

    return 0

if __name__ == "__main__":
    raise SystemExit(main())