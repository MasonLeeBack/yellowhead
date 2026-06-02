#!/usr/bin/env python3
from pathlib import Path
import json
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def main() -> int:
    targets_path = ROOT / "build" / "objdiff" / "targets.json"
    if not targets_path.exists():
        print("missing build/objdiff/targets.json; run tools/gen_objdiff.py first")
        return 1

    targets = json.loads(targets_path.read_text())
    if not targets:
        print("no objdiff targets")
        return 0

    cc = ROOT / "tools" / "ppu-lv2-gcc"

    env = os.environ.copy()

    for t in targets:
        asm_path = ROOT / t["asm"]
        obj_path = ROOT / t["obj"]
        obj_path.parent.mkdir(parents=True, exist_ok=True)

        cmd = [
            str(cc),
            "-c",
            "-x", "assembler",
            "-Wa,-mcellppu",
            "-o", str(obj_path),
            str(asm_path),
        ]

        print(" ".join(cmd))
        rc = subprocess.call(cmd, env=env)
        if rc != 0:
            return rc

    return 0

if __name__ == "__main__":
    raise SystemExit(main())