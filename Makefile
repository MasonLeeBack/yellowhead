PYTHON ?= .venv/bin/python

ROOT := $(CURDIR)
TOOLS_DIR := $(ROOT)/tools

CC      := $(TOOLS_DIR)/ppu-lv2-gcc
CXX     := $(TOOLS_DIR)/ppu-lv2-g++
LD      := $(TOOLS_DIR)/ppu-lv2-ld
READELF := $(TOOLS_DIR)/ppu-lv2-readelf
DECOMP  := $(PYTHON) $(TOOLS_DIR)/decomp_cli.py
OBJDIFF_CLI ?= $(TOOLS_DIR)/agents/objdiff-cli-linux-x86_64
BRANCH_HINTS ?= 1

ORIG_ELF := orig/EBOOT.ELF

RAW_ELF := build/yellowhead.raw.elf
REBUILT_ELF := build/yellowhead.elf

SRC_CPP := $(shell find src -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \) 2>/dev/null)
SRC_C   := $(shell find src -type f -name '*.c' 2>/dev/null)

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
	$(CXX) --version
	$(CC) --version
	$(LD) --version

print-src:
	@echo "SRC_CPP=$(SRC_CPP)"
	@echo "SRC_C=$(SRC_C)"
	@echo "SRC_OBJS=$(SRC_OBJS)"

build/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $(@:.o=.d) -c $< -o $@ $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.cc
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $(@:.o=.d) -c $< -o $@ $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.cxx
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $(@:.o=.d) -c $< -o $@ $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $(@:.o=.d) -c $< -o $@ $(CFLAGS_RECOMP)

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
	@mkdir -p $(dir $@)
	$(CC) -c $(ASMFLAGS) -o $@ $<

build/section_blobs.o: asm/section_blobs.s
	@mkdir -p $(dir $@)
	$(CC) -c $(ASMFLAGS) -o $@ $<

$(RAW_ELF): build/linker.ld build/text_blobs.o build/section_blobs.o build/replacement_map.json
	$(LD) -T build/linker.ld -o $@ build/section_blobs.o build/text_blobs.o $$($(DECOMP) replacement-objects)

$(REBUILT_ELF): $(RAW_ELF)
	$(DECOMP) finalize $(ORIG_ELF) $(RAW_ELF) $(REBUILT_ELF)

rebuild: $(REBUILT_ELF)
	@echo "rebuilt $(REBUILT_ELF)"

check: $(REBUILT_ELF)
	$(DECOMP) compare-loads $(ORIG_ELF) $(REBUILT_ELF)

relocs: $(SRC_OBJS)
	$(DECOMP) object-relocs $(SRC_OBJS)

objdiff.json build/objdiff/targets.json &: $(ORIG_ELF) $(SRC_OBJS) tools/decomp/*.py tools/decomp_cli.py
	$(DECOMP) objdiff $(SRC_OBJS)

build/objdiff/orig/%.o: build/objdiff/orig/%.s
	@mkdir -p $(dir $@)
	@$(CC) -c $(ASMFLAGS) -o $@ $<

build/objdiff/orig/%.s:
	@test -f $@

OBJDIFF_ORIG_OBJS := $(patsubst build/src/%.o,build/objdiff/orig/%.o,$(SRC_OBJS))

objdiff: objdiff.json build/objdiff/targets.json $(OBJDIFF_ORIG_OBJS)

objdiff-targets: objdiff

build/report.json: objdiff
	@$(DECOMP) objdiff-report -o $@ -f json-pretty

objdiff-report: build/report.json
	@cat build/report.json

objdiff-objects:
	@$(DECOMP) objdiff-list --objects

objdiff-functions:
	@$(DECOMP) objdiff-list $(if $(UNIT),--unit "$(UNIT)",)

objdiff-bss:
	@$(DECOMP) objdiff-list --bss $(if $(UNIT),--unit "$(UNIT)",)

objdiff-todo:
	@$(DECOMP) objdiff-list --todo-only $(if $(UNIT),--unit "$(UNIT)",)

objdiff-diff: objdiff
	@test -n "$(UNIT)" || (echo "usage: make objdiff-diff UNIT=CoreLib/src/Clock.o SYMBOL='._Z8GetClockv'" >&2; exit 1)
	@test -n "$(SYMBOL)" || (echo "usage: make objdiff-diff UNIT=CoreLib/src/Clock.o SYMBOL='._Z8GetClockv'" >&2; exit 1)
	@$(OBJDIFF_CLI) diff -p . -u "$(UNIT)" "$(SYMBOL)" -o - --format json-pretty

clean:
	rm -rf build
	rm -f asm/section_blobs.s asm/text/text_blobs.s

distclean: clean
	rm -rf asm/objdiff
	rm -f objdiff.json
