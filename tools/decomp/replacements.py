from __future__ import annotations

from pathlib import Path
import json
import subprocess


def run_nm(nm: Path, obj: Path) -> list[str]:
    return subprocess.check_output([str(nm), "-a", "-P", str(obj)], text=True, errors="replace").splitlines()


def scan_objects(objects: list[Path], nm: Path, out_path: Path) -> dict[str, dict[str, str]]:
    mapping: dict[str, dict[str, str]] = {}
    allow = load_allowlist(out_path.parents[1] / "replacements.json")

    for obj in objects:
        for line in run_nm(nm, obj):
            parts = line.split()
            if len(parts) < 3:
                continue

            name, kind = parts[0], parts[1]
            if kind.lower() != "t":
                continue
            if name not in allow:
                continue

            if name == ".text":
                section = ".text"
            elif name.startswith("."):
                section = ".text" + name
            else:
                section = f".text.{name}"
            mapping[name] = {
                "object": obj.as_posix(),
                "section": section,
            }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(mapping, indent=2, sort_keys=True) + "\n")
    return mapping


def load_allowlist(path: Path) -> set[str]:
    if not path.exists():
        return set()
    raw = json.loads(path.read_text())
    if isinstance(raw, dict):
        raw = raw.get("symbols", [])
    return {str(item) for item in raw}


def load_replacements(path: Path) -> dict[str, dict[str, str]]:
    if not path.exists():
        return {}
    return json.loads(path.read_text())
