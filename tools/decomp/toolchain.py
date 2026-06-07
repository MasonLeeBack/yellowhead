from __future__ import annotations

from pathlib import Path
import os
import sys

ROOT = Path(__file__).resolve().parents[2]

def ps3_sdk_root() -> Path:
    for name in ("PS3_SDK", "SCE_PS3_ROOT", "PS3SDK"):
        value = os.environ.get(name)
        if value:
            return Path(value)
    return ROOT / "sdk"

def ppu_tool(name: str) -> Path:
    if sys.platform == "win32":
        exe = ps3_sdk_root() / "host-win32" / "ppu" / "bin" / f"{name}.exe"
        if exe.exists():
            return exe

    wrapper = ROOT / "tools" / name
    if wrapper.exists():
        return wrapper

    return Path(f"{name}.exe" if sys.platform == "win32" else name)