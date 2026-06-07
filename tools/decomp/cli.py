from __future__ import annotations

from pathlib import Path
import argparse
import json
import struct
import subprocess
import sys
import os

from .elf import ElfImage, Section
from .layout import write_generated, write_manifest
from .objdiff import dwarf_bss_addresses, dwarf_bss_symbols, inferred_bss_address, source_bss_symbols, source_path_for_obj, unmangle_last_component, write_objdiff
from .replacements import load_replacements, scan_objects
from .toolchain import ppu_tool

ROOT = Path(__file__).resolve().parents[2]
ORIG = ROOT / "orig" / "EBOOT.ELF"

def cmd_analyze(_args: argparse.Namespace) -> int:
    with ElfImage(ORIG) as image:
        out_dir = ROOT / "build" / "decomp"
        write_manifest(image, out_dir)
        print(f"wrote {out_dir / 'manifest.json'}")
        print(f"alloc sections: {len(image.alloc_sections())}")
        print(f"text functions: {len(image.text_functions())}")
        print(f"elf relocations: {len(image.relocations)}")
    return 0


def cmd_scan_replacements(args: argparse.Namespace) -> int:
    objects = [Path(p) for p in args.objects]
    mapping = scan_objects(objects, ppu_tool("ppu-lv2-nm"), ROOT / "build" / "replacement_map.json")
    print(f"wrote build/replacement_map.json ({len(mapping)} symbols)")
    return 0


def cmd_replacement_objects(_args: argparse.Namespace) -> int:
    replacements = load_replacements(ROOT / "build" / "replacement_map.json")
    objects = sorted({str(item["object"]) for item in replacements.values() if "object" in item})
    if objects:
        print(" ".join(objects))
    return 0


def cmd_generate(_args: argparse.Namespace) -> int:
    with ElfImage(ORIG) as image:
        replacements = load_replacements(ROOT / "build" / "replacement_map.json")
        write_manifest(image, ROOT / "build" / "decomp")
        write_generated(image, ROOT, replacements)
        print("wrote asm/text/text_blobs.s")
        print("wrote asm/section_blobs.s")
        print("wrote build/linker.ld")
    return 0


def cmd_compare_loads(args: argparse.Namespace) -> int:
    with ElfImage(Path(args.base)) as base, ElfImage(Path(args.other)) as other:
        ok = True
        for seg in base.segments:
            if not seg.is_load or seg.filesz == 0:
                continue
            theirs = next((s for s in other.segments if s.index == seg.index), None)
            if theirs is None:
                print(f"missing load segment {seg.index}")
                ok = False
                continue
            a = bytearray(base.read_at_offset(seg.offset, seg.filesz))
            b = bytearray(other.read_at_offset(theirs.offset, min(theirs.filesz, seg.filesz)))
            if seg.offset == 0:
                # The rebuilt ELF keeps its own section table, so these ELF
                # header fields legitimately differ while loader bytes match.
                for off in [*range(0x28, 0x30), 0x3c, 0x3d, 0x3e, 0x3f]:
                    if off < len(a) and off < len(b):
                        b[off] = a[off]
            if a != b:
                print(f"LOAD {seg.index} differs")
                ok = False
            else:
                print(f"LOAD {seg.index} matches ({seg.filesz} bytes)")
        return 0 if ok else 1


def validate_elf64_be(data: bytes, name: str) -> None:
    if data[:4] != b"\x7fELF" or data[4] != 2 or data[5] != 2:
        raise RuntimeError(f"{name}: expected ELF64 big-endian")


def cmd_finalize(args: argparse.Namespace) -> int:
    orig = bytearray(Path(args.orig).read_bytes())
    raw = bytearray(Path(args.raw).read_bytes())
    validate_elf64_be(orig, args.orig)
    validate_elf64_be(raw, args.raw)

    orig_phoff = struct.unpack_from(">Q", orig, 0x20)[0]
    raw_phoff = struct.unpack_from(">Q", raw, 0x20)[0]
    phentsize = struct.unpack_from(">H", orig, 0x36)[0]
    raw_phentsize = struct.unpack_from(">H", raw, 0x36)[0]
    phnum = struct.unpack_from(">H", orig, 0x38)[0]
    raw_phnum = struct.unpack_from(">H", raw, 0x38)[0]
    if phentsize != raw_phentsize or phnum != raw_phnum:
        raise RuntimeError("program header table shape changed")

    raw[7] = orig[7]
    raw[8] = orig[8]
    raw[raw_phoff:raw_phoff + phentsize * phnum] = orig[orig_phoff:orig_phoff + phentsize * phnum]
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(raw)
    print(f"wrote {out}")
    return 0


def cmd_object_relocs(args: argparse.Namespace) -> int:
    out = ROOT / "build" / "relocs" / "objects.tsv"
    out.parent.mkdir(parents=True, exist_ok=True)
    readelf = ppu_tool("ppu-lv2-readelf")
    with out.open("w") as fh:
        fh.write("object\trelocations\n")
        for obj in args.objects:
            result = subprocess.run([str(readelf), "-r", obj], text=True, errors="replace", stdout=subprocess.PIPE, check=False)
            fh.write(f"{obj}\t{result.stdout.count('R_PPC64_') + result.stdout.count('R_POWERPC_')}\n")
            (out.parent / (Path(obj).name + ".readelf.txt")).write_text(result.stdout)
    print(f"wrote {out}")
    return 0


def cmd_objdiff(args: argparse.Namespace) -> int:
    source_objs = [Path(p) for p in args.objects]
    with ElfImage(ORIG) as image:
        write_objdiff(ROOT, image, source_objs)
    print("wrote objdiff.json")
    print("wrote build/objdiff/targets.json")
    return 0


def cmd_bss(args: argparse.Namespace) -> int:
    obj = Path(args.object)
    if not obj.is_absolute():
        obj = ROOT / obj
    symbols = source_bss_symbols(obj)
    if not symbols:
        print(f"{obj}: no .bss symbols")
        return 0

    with ElfImage(ORIG) as image:
        addresses = dwarf_bss_addresses(image, source_path_for_obj(ROOT, obj))

    source_offsets = {name: offset for name, offset, _size, _bind in symbols}
    mapped = [(name, offset, size, bind, inferred_bss_address(addresses, source_offsets, name, offset)) for name, offset, size, bind in symbols]
    mapped_addresses = [addr for _name, _offset, _size, _bind, addr in mapped if addr is not None]
    base = min(mapped_addresses) if mapped_addresses else None

    print(unit_name_from_target_key(str(obj.relative_to(ROOT))))
    print(f"{'source':>8} {'dwarf':>8} {'delta':>8} {'size':>5} symbol")
    for name, offset, size, _bind, addr in mapped:
        dwarf = addr - base if addr is not None and base is not None else None
        if dwarf is None:
            dwarf_text = "missing"
            delta_text = ""
        else:
            dwarf_text = f"0x{dwarf:04x}"
            delta = offset - dwarf
            delta_text = f"{delta:+#06x}"
        print(f"0x{offset:04x} {dwarf_text:>8} {delta_text:>8} {size:5} {name}")
    return 0


def load_objdiff_targets() -> dict[str, dict[str, object]]:
    path = ROOT / "build" / "objdiff" / "targets.json"
    if not path.exists():
        raise RuntimeError("missing build/objdiff/targets.json; run `make objdiff` first")
    return json.loads(path.read_text())


def load_objdiff_report() -> dict[str, object]:
    cli = ROOT / "tools" / "agents" / "objdiff-cli-linux-x86_64"
    result = subprocess.run(
        [str(cli), "report", "generate", "-p", str(ROOT), "-o", "-", "-f", "json"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        errors="replace",
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "objdiff report generation failed")
    return json.loads(result.stdout)


def int_measure(measures: dict[str, object], name: str) -> int:
    value = measures.get(name, 0)
    if isinstance(value, str):
        return int(value) if value.isdigit() else 0
    if isinstance(value, (int, float)):
        return int(value)
    return 0


def set_percent(measures: dict[str, object], matched_name: str, total_name: str, percent_name: str) -> None:
    total = int_measure(measures, total_name)
    matched = int_measure(measures, matched_name)
    measures[percent_name] = (matched / total * 100.0) if total else 100.0


def elf_code_sections(sections: list[Section]) -> list[Section]:
    return [section for section in sections if section.is_alloc and section.is_exec and section.size > 0]


def elf_data_sections(sections: list[Section]) -> list[Section]:
    return [section for section in sections if section.is_alloc and not section.is_exec and section.size > 0]


def correct_report_totals(report: dict[str, object], image: ElfImage) -> dict[str, object]:
    measures = report.get("measures")
    if not isinstance(measures, dict):
        return report

    code_total = sum(section.size for section in elf_code_sections(image.sections))
    data_total = sum(section.size for section in elf_data_sections(image.sections))
    function_total = len(image.text_functions())

    units = report.get("units", [])
    if isinstance(units, list):
        other_code = 0
        other_data = 0
        other_functions = 0
        unaccounted: dict[str, object] | None = None
        for unit in units:
            if not isinstance(unit, dict):
                continue
            unit_measures = unit.get("measures", {})
            if not isinstance(unit_measures, dict):
                continue
            if unit.get("name") == "__unaccounted.o":
                unaccounted = unit_measures
                continue
            other_code += int_measure(unit_measures, "total_code")
            other_data += int_measure(unit_measures, "total_data")
            other_functions += int_measure(unit_measures, "total_functions")
        if unaccounted is not None:
            unaccounted["total_code"] = str(max(0, code_total - other_code))
            unaccounted["total_data"] = str(max(0, data_total - other_data))
            unaccounted["total_functions"] = max(0, function_total - other_functions)

    measures["total_code"] = str(code_total)
    measures["matched_code"] = str(min(int_measure(measures, "matched_code"), code_total))
    measures["total_data"] = str(data_total)
    measures["matched_data"] = str(min(int_measure(measures, "matched_data"), data_total))
    measures["total_functions"] = function_total
    measures["matched_functions"] = min(int_measure(measures, "matched_functions"), function_total)
    set_percent(measures, "matched_code", "total_code", "matched_code_percent")
    set_percent(measures, "matched_data", "total_data", "matched_data_percent")
    set_percent(measures, "matched_functions", "total_functions", "matched_functions_percent")
    return report


def write_json_report(report: dict[str, object], output: str, fmt: str) -> None:
    indent = 2 if fmt == "json-pretty" else None
    text = json.dumps(report, indent=indent)
    if indent is not None:
        text += "\n"
    if output == "-":
        print(text, end="" if text.endswith("\n") else "\n")
        return
    path = Path(output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


def cmd_objdiff_report(args: argparse.Namespace) -> int:
    report = load_objdiff_report()
    with ElfImage(ORIG) as image:
        correct_report_totals(report, image)
    write_json_report(report, args.output, args.format)
    return 0


def unit_name_from_target_key(key: str) -> str:
    prefix = "build/src/"
    return key[len(prefix):] if key.startswith(prefix) else key


def normalize_unit(name: str) -> str:
    name = name.replace("\\", "/")
    for prefix in ("build/src/", "build/objdiff/orig/"):
        if name.startswith(prefix):
            name = name[len(prefix):]
    return name


def report_unit_map(report: dict[str, object]) -> dict[str, dict[str, object]]:
    units = report.get("units", [])
    if not isinstance(units, list):
        return {}
    return {str(unit.get("name")): unit for unit in units if isinstance(unit, dict)}


def function_percent(function: dict[str, object]) -> float:
    try:
        return float(function.get("fuzzy_match_percent", 0.0))
    except (TypeError, ValueError):
        return 0.0


def is_done(function: dict[str, object]) -> bool:
    return function_percent(function) >= 99.999


def target_symbols(target: dict[str, object]) -> list[str]:
    symbols = target.get("symbols", [])
    if not isinstance(symbols, list):
        return []
    return [str(symbol) for symbol in symbols]


def target_object_path(target_key: str) -> Path:
    obj = ROOT / target_key
    if not obj.exists():
        raise RuntimeError(f"missing source object {target_key}; run the build first")
    return obj


def local_static_guard_alias(symbol: str) -> str | None:
    if not symbol.startswith("_ZGVZ"):
        return None
    return unmangle_last_component("_Z" + symbol[4:])


def cmd_objdiff_list_bss(rows: list[tuple[str, dict[str, object] | None, dict[str, object]]]) -> int:
    with ElfImage(ORIG) as image:
        for unit_name, _report_unit, _target in rows:
            obj = target_object_path("build/src/" + unit_name)
            source_symbols = source_bss_symbols(obj)
            source_by_name = {name: (offset, size) for name, offset, size, _bind in source_symbols}
            source_offsets = {name: offset for name, offset, _size, _bind in source_symbols}
            source_aliases: dict[str, tuple[str, int, int]] = {}
            ambiguous_aliases: set[str] = set()
            for source_name, offset, size, _bind in source_symbols:
                alias = unmangle_last_component(source_name)
                if alias is None:
                    continue
                if alias in source_aliases:
                    ambiguous_aliases.add(alias)
                    continue
                source_aliases[alias] = (source_name, offset, size)
            for alias in ambiguous_aliases:
                source_aliases.pop(alias, None)
            addresses = dwarf_bss_addresses(image, source_path_for_obj(ROOT, obj))
            target_symbols_by_name = dwarf_bss_symbols(image, source_path_for_obj(ROOT, obj))
            if not target_symbols_by_name and not source_symbols:
                continue

            print(unit_name)
            print(f"  {'status':<7} {'source':>8} {'target':>10} {'size':>6} symbol")
            matched_source_names: set[str] = set()
            matched_aliases: set[str] = set()
            for name, addr, size in target_symbols_by_name:
                source = source_by_name.get(name)
                source_name = name if source is not None else None
                if source is None:
                    aliased = source_aliases.get(name)
                    if aliased is not None and aliased[2] == size:
                        source_name, source = aliased[0], (aliased[1], aliased[2])
                status = "OK" if source is not None else "TODO"
                source_text = f"0x{source[0]:04x}" if source is not None else "-"
                if source_name is not None:
                    matched_source_names.add(source_name)
                    matched_aliases.add(name)
                print(f"  {status:<7} {source_text:>8} 0x{addr:08x} {size:6d} {name}")

            target_names = {name for name, _addr, _size in target_symbols_by_name}
            for name, offset, size, _bind in source_symbols:
                if name in target_names or name in matched_source_names:
                    continue
                guard_alias = local_static_guard_alias(name)
                if guard_alias is not None and guard_alias in matched_aliases:
                    continue
                addr = inferred_bss_address(addresses, source_offsets, name, offset)
                target_text = f"0x{addr:08x}" if addr is not None else "-"
                print(f"  EXTRA   0x{offset:04x} {target_text:>10} {size:6d} {name}")
    return 0


def cmd_objdiff_list(args: argparse.Namespace) -> int:
    targets = load_objdiff_targets()

    selected = normalize_unit(args.unit) if args.unit else None
    rows: list[tuple[str, dict[str, object] | None, dict[str, object]]] = []
    for target_key, target in sorted(targets.items(), key=lambda item: unit_name_from_target_key(item[0])):
        unit_name = unit_name_from_target_key(target_key)
        if selected and normalize_unit(unit_name) != selected:
            continue
        rows.append((unit_name, None, target))

    if selected and not rows:
        known = ", ".join(unit_name_from_target_key(key) for key in sorted(targets))
        raise RuntimeError(f"unknown objdiff unit {args.unit!r}; known units: {known}")

    if args.bss:
        return cmd_objdiff_list_bss(rows)

    report = load_objdiff_report()
    units_by_name = report_unit_map(report)
    rows = [(unit_name, units_by_name.get(unit_name), target) for unit_name, _report_unit, target in rows]

    if args.objects:
        for unit_name, report_unit, target in rows:
            symbols = target_symbols(target)
            funcs = report_unit.get("functions", []) if report_unit else []
            reported = {str(func.get("name", "")) for func in funcs if isinstance(func, dict)}
            missing = [symbol for symbol in symbols if symbol not in reported]
            todo = sum(1 for func in funcs if isinstance(func, dict) and not is_done(func))
            todo += len(missing)
            total = len(funcs) + len(missing)
            match = ""
            if report_unit:
                measures = report_unit.get("measures", {})
                if isinstance(measures, dict) and "fuzzy_match_percent" in measures:
                    match = f"  {float(measures['fuzzy_match_percent']):6.2f}%"
            status = "OK  " if todo == 0 else "todo"
            print(f"{unit_name:<36} {todo:>3}/{total:<3} {status}{match}")
        return 0

    for unit_name, report_unit, target in rows:
        lines: list[str] = []
        funcs = report_unit.get("functions", []) if report_unit else []
        reported: set[str] = set()
        if funcs:
            for func in funcs:
                if not isinstance(func, dict):
                    continue
                done = is_done(func)
                name = str(func.get("name", ""))
                reported.add(name)
                if args.todo_only and done:
                    continue
                meta = func.get("metadata", {})
                demangled = ""
                if isinstance(meta, dict) and meta.get("demangled_name"):
                    demangled = f"  {meta['demangled_name']}"
                status = "OK  " if done else "TODO"
                lines.append(f"  {status} {function_percent(func):6.2f}%  {name}{demangled}")
        for symbol in target_symbols(target):
            if symbol not in reported:
                lines.append(f"  TODO    n/a   {symbol}  (missing from objdiff report)")
        if args.todo_only and not lines and not selected:
            continue
        print(unit_name)
        for line in lines:
            print(line)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="decomp")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("analyze").set_defaults(func=cmd_analyze)
    scan = sub.add_parser("scan-replacements")
    scan.add_argument("objects", nargs="*")
    scan.set_defaults(func=cmd_scan_replacements)
    sub.add_parser("replacement-objects").set_defaults(func=cmd_replacement_objects)
    sub.add_parser("generate").set_defaults(func=cmd_generate)
    compare = sub.add_parser("compare-loads")
    compare.add_argument("base")
    compare.add_argument("other")
    compare.set_defaults(func=cmd_compare_loads)
    final = sub.add_parser("finalize")
    final.add_argument("orig")
    final.add_argument("raw")
    final.add_argument("out")
    final.set_defaults(func=cmd_finalize)
    relocs = sub.add_parser("object-relocs")
    relocs.add_argument("objects", nargs="*")
    relocs.set_defaults(func=cmd_object_relocs)
    objdiff = sub.add_parser("objdiff")
    objdiff.add_argument("objects", nargs="*")
    objdiff.set_defaults(func=cmd_objdiff)
    objdiff_report = sub.add_parser("objdiff-report")
    objdiff_report.add_argument("-o", "--output", default="-", help="Output path, or - for stdout")
    objdiff_report.add_argument("-f", "--format", choices=["json", "json-pretty"], default="json-pretty")
    objdiff_report.set_defaults(func=cmd_objdiff_report)
    bss = sub.add_parser("bss")
    bss.add_argument("object", help="Built object, e.g. build/src/CWLib/src/SystemCommon.o")
    bss.set_defaults(func=cmd_bss)
    objdiff_list = sub.add_parser("objdiff-list")
    objdiff_list.add_argument("--unit", help="Objdiff unit, e.g. CoreLib/src/Clock.o")
    objdiff_list.add_argument("--objects", action="store_true", help="List objdiff objects instead of functions")
    objdiff_list.add_argument("--todo-only", action="store_true", help="Only show functions below 100%%")
    objdiff_list.add_argument("--bss", action="store_true", help="List DWARF-backed .bss symbols instead of functions")
    objdiff_list.set_defaults(func=cmd_objdiff_list)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
