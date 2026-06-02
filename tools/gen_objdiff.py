#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]


DEFAULT_STRIP_PREFIXES = [
    "dev/tgsdemo2009/code/CWLib/src/",
    "dev/tgsdemo2009/code/",
    "../../../../",
]


def load_json(path: Path, default):
    if not path.exists():
        return default
    return json.loads(path.read_text())


def sanitize_component(name: str) -> str:
    name = re.sub(r"[^A-Za-z0-9_.+-]+", "_", name)
    name = name.strip("._")
    return name or "unnamed"


def normalize_path(path: str) -> str:
    path = path.replace("\\", "/")
    path = re.sub(r"^[A-Za-z]:/", "", path)
    while path.startswith("../"):
        path = path[3:]
    path = path.lstrip("/")
    return path


def strip_known_prefixes(source: str, strip_prefixes: list[str]) -> list[str]:
    source = normalize_path(source)
    out = [source]

    for prefix in strip_prefixes:
        prefix = normalize_path(prefix)
        if source.startswith(prefix):
            out.append(source[len(prefix):])

    # Useful generic fallback:
    # dev/tgsdemo2009/code/CWLib/src/Foo/Bar.cpp -> Foo/Bar.cpp
    marker = "/src/"
    if marker in source:
        out.append(source.split(marker, 1)[1])

    # Another useful fallback:
    # anything/code/Foo.cpp -> Foo.cpp relative-ish
    marker = "/code/"
    if marker in source:
        out.append(source.split(marker, 1)[1])

    # Last-ditch basename fallback. Only used if exact object exists.
    out.append(Path(source).name)

    # Deduplicate while preserving order.
    seen = set()
    result = []
    for x in out:
        x = normalize_path(x)
        if x and x not in seen:
            seen.add(x)
            result.append(x)

    return result


def cpp_to_obj_rel(path: str) -> str:
    p = Path(path)
    if p.suffix:
        p = p.with_suffix(".o")
    else:
        p = Path(str(p) + ".o")
    return p.as_posix()


def find_base_obj(source: str, strip_prefixes: list[str]) -> str | None:
    candidates = strip_known_prefixes(source, strip_prefixes)

    for rel_src in candidates:
        rel_obj = cpp_to_obj_rel(rel_src)
        base = ROOT / "build" / "src" / rel_obj
        if base.exists():
            return base.relative_to(ROOT).as_posix()

    return None


def source_display_name(source: str, strip_prefixes: list[str]) -> str:
    candidates = strip_known_prefixes(source, strip_prefixes)

    # Prefer a stripped path if available.
    if len(candidates) > 1:
        return candidates[1]

    return candidates[0]


def main() -> int:
    targets_path = ROOT / "build" / "objdiff" / "targets.json"
    if not targets_path.exists():
        raise RuntimeError("missing build/objdiff/targets.json; run tools/gen_objdiff_targets.py first")

    settings = load_json(ROOT / "config" / "objdiff_paths.json", {})
    strip_prefixes = settings.get("strip_prefixes", DEFAULT_STRIP_PREFIXES)

    targets = json.loads(targets_path.read_text())

    units = []
    missing = []

    for target in targets:
        source = target["source"]
        target_obj = target["obj"]

        base_obj = find_base_obj(source, strip_prefixes)

        if base_obj is None:
            missing.append(source)
            continue

        units.append({
            "name": source_display_name(source, strip_prefixes),
            "target_path": target_obj,
            "base_path": base_obj,
        })

    config = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "custom_make": "make",
        "custom_args": ["objdiff-prep"],
        "build_target": True,
        "build_base": True,
        "watch_patterns": [
            "src/**/*.cpp",
            "src/**/*.c",
            "src/**/*.h",
            "include/**/*.h",
            "tools/**/*.py",
            "Makefile",
            "config/**/*.json",
        ],
        "ignore_patterns": [
            "build/**/*",
            "orig/**/*",
            "sdk/**/*",
            ".venv/**/*",
            "asm/**/*",
        ],
        "units": units,
    }

    out_path = ROOT / "objdiff.json"
    out_path.write_text(json.dumps(config, indent=2) + "\n")

    missing_path = ROOT / "build" / "objdiff" / "missing_base_objects.txt"
    missing_path.parent.mkdir(parents=True, exist_ok=True)
    missing_path.write_text("\n".join(missing) + ("\n" if missing else ""))

    print(f"wrote {out_path}")
    print(f"units: {len(units)}")
    print(f"missing base objects: {len(missing)}")
    print(f"wrote {missing_path}")

    for unit in units[:30]:
        print(f"{unit['name']}: {unit['target_path']} vs {unit['base_path']}")

    if len(units) > 30:
        print(f"... {len(units) - 30} more units")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())