# yellowhead

`yellowhead` is an example repository for Codex-assisted reverse-engineering and decompilation work on LittleBigPlanet PS3 code. It is intentionally a workbench: source is recovered from symbols, DWARF, disassembly, objdiff feedback, and repeated build checks rather than from original source code.

The goal is a faithful decompilation project, not a pile of convenient lookalikes. Reconstructed structures should live in real headers, functions should match for normal compiler/code-shape reasons, and progress should be measured against the whole original ELF rather than only the files currently under active decompilation.

Contributions are welcome. Small, well-supported decompilation improvements are especially useful: matching functions, DWARF-backed type declarations, relocation/data accounting fixes, tool improvements, and documentation that makes the project easier to reproduce.

## Status

Progress is generated with objdiff plus a project-level correction step that accounts for code, data, and function totals from the full original ELF. This keeps the report honest while many source objects are still missing.

```sh
make objdiff-report
```

That writes `build/report.json` and prints the current report. The report totals are corrected against the full original ELF, while the interactive objdiff config only lists real source objects so the GUI stays responsive.

The generated objdiff config uses `tools/objdiff-make` as its build command. It delegates selected-object builds to `make`, but a no-argument GUI build request is a no-op so opening or refreshing objdiff does not accidentally run the full default rebuild.

```sh
make objdiff-objects
make objdiff-functions UNIT=CoreLib/src/Clock.o
make objdiff-todo UNIT=CoreLib/src/Clock.o
make objdiff-diff UNIT=CoreLib/src/Clock.o SYMBOL='._Z8GetClockv'
```

## Layout

- `src/` - handwritten and recovered C/C++ source.
- `include/` - shared project headers.
- `tools/decomp/` - ELF analysis, layout generation, objdiff generation, progress correction, finalization, and relocation reporting.
- `tools/ppu-lv2-*` - wrappers around the local PS3 SDK toolchain.
- `tools/agents/` - optional local agent tools such as `objdiff-cli`; ignored unless provided separately.
- `replacements.json` - explicit opt-in list for source functions that should replace fallback blobs in the rebuilt ELF.
- `asm/` - generated assembly output; only placeholder directories are tracked.
- `analysis/` - local reverse-engineering notes and dumps; large artifacts are ignored.
- `orig/` - local original ELF input; ignored.
- `sdk/` - local PS3 SDK/toolchain payload; ignored.
- `build/` - generated objects, reports, linker scripts, and rebuilt ELF output; ignored.

## Build Inputs

The repository does not commit proprietary game or SDK files. Local builds expect:

- `orig/EBOOT.ELF`
- `sdk/host-win32/ppu/bin/ppu-lv2-*.exe`
- `tools/agents/objdiff-cli-linux-x86_64` for objdiff reports and one-shot diffs

Install Python dependencies with:

```sh
python -m pip install -r requirements.txt
```

## Common Commands

```sh
make toolcheck
make analyze
make generate
make rebuild
make check
make relocs
make objdiff
make objdiff-report
make clean
make distclean
```

`make check` rebuilds `build/yellowhead.elf` and compares loadable segments against `orig/EBOOT.ELF`. Source files compile by default, but they do not replace original functions unless their dot-prefixed PPC64 code symbol is listed in `replacements.json`.

Example replacement entry:

```json
{
  "symbols": [
    "._Z8GetClockv"
  ]
}
```

This keeps exploratory or nonmatching code from breaking the rebuilt ELF. Once a function matches and is ready to replace the fallback bytes, opt it in and run `make check`.

## Contributing

Please keep changes faithful to the recovered binary:

- Put DWARF-backed structures and declarations in appropriate headers.
- Prefer matching source shape and compiler behavior over local tricks.
- Avoid inline assembly, section attributes, or other hacks unless there is a documented project-level reason.
- Keep objdiff/report tooling honest; missing bytes should be counted as missing, not hidden.
- Run `make check` for changes that affect generated ELF output.

This project is meant to show what a careful Codex-assisted reverse-engineering loop can look like. Contributions that make it more accurate, reproducible, or understandable are very welcome.
