# Cross-platform Makefile for yellowhead
#
# Linux / WSL / POSIX:
#   make
#
# Windows native cmd.exe:
#   make
#
# Windows expects SDK tools here by default:
#   sdk/host-win32/ppu/bin/ppu-lv2-gcc.exe
#   sdk/host-win32/ppu/bin/ppu-lv2-g++.exe
#   sdk/host-win32/ppu/bin/ppu-lv2-ld.exe
#   sdk/host-win32/ppu/bin/ppu-lv2-readelf.exe

ROOT := $(CURDIR)
TOOLS_DIR := $(ROOT)/tools

ifeq ($(OS),Windows_NT)
HOST_WINDOWS := 1
else
HOST_WINDOWS := 0
endif

ifeq ($(HOST_WINDOWS),1)
PYTHON ?= py -3
SDK_PPU_BIN ?= $(ROOT)/sdk/host-win32/ppu/bin

CC      := $(SDK_PPU_BIN)/ppu-lv2-gcc.exe
CXX     := $(SDK_PPU_BIN)/ppu-lv2-g++.exe
LD      := $(SDK_PPU_BIN)/ppu-lv2-ld.exe
READELF := $(SDK_PPU_BIN)/ppu-lv2-readelf.exe

OBJDIFF_CLI ?= $(TOOLS_DIR)/agents/objdiff-cli-windows-x86_64.exe

MKDIR_P = $(PYTHON) -c "import pathlib,sys; pathlib.Path(sys.argv[1]).mkdir(parents=True, exist_ok=True)"
RM_RF = $(PYTHON) -c "import pathlib,shutil,sys; [shutil.rmtree(p, ignore_errors=True) if p.is_dir() and not p.is_symlink() else (p.unlink() if p.exists() or p.is_symlink() else None) for p in map(pathlib.Path, sys.argv[1:])]"
CAT = $(PYTHON) -c "import pathlib,sys; sys.stdout.buffer.write(pathlib.Path(sys.argv[1]).read_bytes())"
CHECK_FILE = $(PYTHON) -c "import pathlib,sys; sys.exit(0 if pathlib.Path(sys.argv[1]).is_file() else 1)"

SRC_CPP := $(shell $(PYTHON) -c "from pathlib import Path; exts={'.cpp','.cc','.cxx'}; print(' '.join(sorted(p.as_posix() for p in Path('src').rglob('*') if p.suffix in exts)))")
SRC_C   := $(shell $(PYTHON) -c "from pathlib import Path; print(' '.join(sorted(p.as_posix() for p in Path('src').rglob('*.c'))))")
else
PYTHON ?= .venv/bin/python

CC      := $(TOOLS_DIR)/ppu-lv2-gcc
CXX     := $(TOOLS_DIR)/ppu-lv2-g++
LD      := $(TOOLS_DIR)/ppu-lv2-ld
READELF := $(TOOLS_DIR)/ppu-lv2-readelf

OBJDIFF_CLI ?= $(TOOLS_DIR)/agents/objdiff-cli-linux-x86_64

MKDIR_P = mkdir -p
RM_RF = rm -rf
CAT = cat
CHECK_FILE = test -f

SRC_CPP := $(shell find src -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \) 2>/dev/null)
SRC_C   := $(shell find src -type f -name '*.c' 2>/dev/null)
endif

DECOMP := $(PYTHON) "$(TOOLS_DIR)/decomp_cli.py"
BRANCH_HINTS ?= 1

ORIG_ELF := orig/EBOOT.ELF

RAW_ELF := build/yellowhead.raw.elf
REBUILT_ELF := build/yellowhead.elf

SRC_CXX_OBJS := \
	$(patsubst src/%.cpp,build/src/%.o,$(filter %.cpp,$(SRC_CPP))) \
	$(patsubst src/%.cc,build/src/%.o,$(filter %.cc,$(SRC_CPP))) \
	$(patsubst src/%.cxx,build/src/%.o,$(filter %.cxx,$(SRC_CPP)))

SRC_C_OBJS := $(patsubst src/%.c,build/src/%.o,$(SRC_C))
SRC_OBJS := $(SRC_CXX_OBJS) $(SRC_C_OBJS)
DEPFILES := $(SRC_OBJS:.o=.d)

PROJECT_INCLUDES := \
	-Iinclude \
	-Isrc/CoreLib/src \
	-Isrc/CWLib/src

CXXFLAGS_RECOMP := \
	-mcpu=cell \
	-mtune=cell \
	-O2 \
	-fno-exceptions \
	-fno-rtti \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-ffunction-sections \
	-falign-functions=4 \
	-fsection-anchors \
	$(PROJECT_INCLUDES)

CFLAGS_RECOMP := \
	-O2 \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-ffunction-sections \
	$(PROJECT_INCLUDES)

ifeq ($(BRANCH_HINTS),0)
CXXFLAGS_RECOMP += -DDECOMP_DISABLE_BRANCH_HINTS=1
CFLAGS_RECOMP += -DDECOMP_DISABLE_BRANCH_HINTS=1
endif

ASMFLAGS := -x assembler -Wa,-mcellppu

build/src/CWLib/%.o: CXXFLAGS_RECOMP += -mcallprof=1
build/src/CWLib/%.o: CFLAGS_RECOMP += -mcallprof=1

.PHONY: all analyze generate relocs objdiff objdiff-targets objdiff-report objdiff-objects objdiff-functions objdiff-bss objdiff-todo objdiff-diff rebuild check clean distclean toolcheck print-src

all: rebuild

toolcheck:
	"$(CXX)" --version
	"$(CC)" --version
	"$(LD)" --version

print-src:
	@echo SRC_CPP=$(SRC_CPP)
	@echo SRC_C=$(SRC_C)
	@echo SRC_OBJS=$(SRC_OBJS)

build/src/%.o: src/%.cpp
	@$(MKDIR_P) "$(dir $@)"
	"$(CXX)" -MMD -MP -MF "$(@:.o=.d)" -c "$<" -o "$@" $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.cc
	@$(MKDIR_P) "$(dir $@)"
	"$(CXX)" -MMD -MP -MF "$(@:.o=.d)" -c "$<" -o "$@" $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.cxx
	@$(MKDIR_P) "$(dir $@)"
	"$(CXX)" -MMD -MP -MF "$(@:.o=.d)" -c "$<" -o "$@" $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.c
	@$(MKDIR_P) "$(dir $@)"
	"$(CC)" -MMD -MP -MF "$(@:.o=.d)" -c "$<" -o "$@" $(CFLAGS_RECOMP)

-include $(DEPFILES)

build/decomp/manifest.json: $(ORIG_ELF) tools/decomp/*.py tools/decomp_cli.py
	$(DECOMP) analyze

analyze: build/decomp/manifest.json

build/replacement_map.json: $(SRC_OBJS) replacements.json tools/decomp/*.py tools/decomp_cli.py
	$(DECOMP) scan-replacements $(SRC_OBJS)

asm/text/text_blobs.s asm/section_blobs.s build/linker.ld build/text_layout.ldinc: \
		$(ORIG_ELF) build/replacement_map.json tools/decomp/*.py tools/decomp_cli.py
	$(DECOMP) generate

generate: asm/text/text_blobs.s asm/section_blobs.s build/linker.ld build/text_layout.ldinc

build/text_blobs.o: asm/text/text_blobs.s
	@$(MKDIR_P) "$(dir $@)"
	"$(CC)" -c $(ASMFLAGS) -o "$@" "$<"

build/section_blobs.o: asm/section_blobs.s
	@$(MKDIR_P) "$(dir $@)"
	"$(CC)" -c $(ASMFLAGS) -o "$@" "$<"

$(RAW_ELF): build/linker.ld build/text_blobs.o build/section_blobs.o build/replacement_map.json
	"$(LD)" -T build/linker.ld -o "$@" build/section_blobs.o build/text_blobs.o $$($(DECOMP) replacement-objects)

$(REBUILT_ELF): $(RAW_ELF)
	$(DECOMP) finalize $(ORIG_ELF) $(RAW_ELF) $(REBUILT_ELF)

rebuild: $(REBUILT_ELF)
	@echo rebuilt $(REBUILT_ELF)

check: $(REBUILT_ELF)
	$(DECOMP) compare-loads $(ORIG_ELF) $(REBUILT_ELF)

relocs: $(SRC_OBJS)
	$(DECOMP) object-relocs $(SRC_OBJS)

objdiff.json build/objdiff/targets.json &: $(ORIG_ELF) $(SRC_OBJS) tools/decomp/*.py tools/decomp_cli.py
	$(DECOMP) objdiff $(SRC_OBJS)

build/objdiff/orig/%.o: build/objdiff/orig/%.s
	@$(MKDIR_P) "$(dir $@)"
	@"$(CC)" -c $(ASMFLAGS) -o "$@" "$<"

build/objdiff/orig/%.s:
	@$(CHECK_FILE) "$@"

OBJDIFF_ORIG_OBJS := $(patsubst build/src/%.o,build/objdiff/orig/%.o,$(SRC_OBJS))

objdiff: objdiff.json build/objdiff/targets.json $(OBJDIFF_ORIG_OBJS)

objdiff-targets: objdiff

build/report.json: objdiff
	@$(DECOMP) objdiff-report -o "$@" -f json-pretty

objdiff-report: build/report.json
	@$(CAT) build/report.json

objdiff-objects:
	@$(DECOMP) objdiff-list --objects

objdiff-functions:
	@$(DECOMP) objdiff-list $(if $(UNIT),--unit "$(UNIT)",)

objdiff-bss:
	@$(DECOMP) objdiff-list --bss $(if $(UNIT),--unit "$(UNIT)",)

objdiff-todo:
	@$(DECOMP) objdiff-list --todo-only $(if $(UNIT),--unit "$(UNIT)",)

ifeq ($(filter objdiff-diff,$(MAKECMDGOALS)),objdiff-diff)
ifeq ($(strip $(UNIT)),)
$(error usage: make objdiff-diff UNIT=CoreLib/src/Clock.o SYMBOL='._Z8GetClockv')
endif
ifeq ($(strip $(SYMBOL)),)
$(error usage: make objdiff-diff UNIT=CoreLib/src/Clock.o SYMBOL='._Z8GetClockv')
endif
endif

objdiff-diff: objdiff
	@"$(OBJDIFF_CLI)" diff -p . -u "$(UNIT)" "$(SYMBOL)" -o - --format json-pretty

clean:
	$(RM_RF) build asm/section_blobs.s asm/text/text_blobs.s

distclean: clean
	$(RM_RF) asm/objdiff objdiff.json