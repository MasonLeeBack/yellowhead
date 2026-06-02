PYTHON ?= .venv/bin/python

TOOLS_DIR := $(CURDIR)/tools

CC      := $(TOOLS_DIR)/ppu-lv2-gcc
CXX     := $(TOOLS_DIR)/ppu-lv2-g++
LD      := $(TOOLS_DIR)/ppu-lv2-ld
READELF := $(TOOLS_DIR)/ppu-lv2-readelf

# Native assembler for objdiff target objects.
# sudo apt install binutils-powerpc64-linux-gnu
OBJDIFF_AS ?= powerpc64-linux-gnu-as

ASMFLAGS := -x assembler -Wa,-mcellppu

ORIG_ELF := orig/EBOOT.ELF

SECTION_RAW := build/EBOOT.sections.raw.elf
SECTION_REBUILT := build/EBOOT.sections.elf

TEXT_ASM := asm/text/text_blobs.s
TEXT_OBJ := build/text_blobs.o

SRC_CPP := $(shell find src -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \) 2>/dev/null)
SRC_C   := $(shell find src -type f -name '*.c' 2>/dev/null)

SRC_CXX_OBJS := \
	$(patsubst src/%.cpp,build/src/%.o,$(filter %.cpp,$(SRC_CPP))) \
	$(patsubst src/%.cc,build/src/%.o,$(filter %.cc,$(SRC_CPP))) \
	$(patsubst src/%.cxx,build/src/%.o,$(filter %.cxx,$(SRC_CPP)))

SRC_C_OBJS := $(patsubst src/%.c,build/src/%.o,$(SRC_C))

SRC_OBJS := $(SRC_CXX_OBJS) $(SRC_C_OBJS)

CXXFLAGS_RECOMP := \
    -mcpu=cell \
	-mtune=cell \
	-mcallprof=1 \
	-O2 \
	-fno-exceptions \
	-fno-rtti \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-ffunction-sections \
	-fdata-sections \
	-Iinclude

CFLAGS_RECOMP := \
	-O2 \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-ffunction-sections \
	-fdata-sections \
	-Iinclude

.PHONY: all rebuild sections check clean clean-objdiff
.PHONY: toolcheck info print-src
.PHONY: decomp-db objdiff-bootstrap objdiff-config objdiff-targets

# Normal default: build runnable ELF, do not byte-compare.
all: rebuild

# ---------------------------------------------------------------------
# Tool / debug helpers
# ---------------------------------------------------------------------

toolcheck:
	. ./config/toolchain.env && $(CXX) --version
	. ./config/toolchain.env && $(CC) --version
	. ./config/toolchain.env && $(LD) --version
	@command -v $(OBJDIFF_AS) >/dev/null || echo "warning: $(OBJDIFF_AS) not found; install binutils-powerpc64-linux-gnu"

info:
	. ./config/toolchain.env && $(READELF) -h $(ORIG_ELF)
	. ./config/toolchain.env && $(READELF) -l $(ORIG_ELF)
	. ./config/toolchain.env && $(READELF) -S $(ORIG_ELF)

print-src:
	@echo "SRC_CPP=$(SRC_CPP)"
	@echo "SRC_C=$(SRC_C)"
	@echo "SRC_OBJS=$(SRC_OBJS)"

# ---------------------------------------------------------------------
# Source tree compile
# ---------------------------------------------------------------------

build/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	. ./config/toolchain.env && $(CXX) -c $< -o $@ $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.cc
	@mkdir -p $(dir $@)
	. ./config/toolchain.env && $(CXX) -c $< -o $@ $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.cxx
	@mkdir -p $(dir $@)
	. ./config/toolchain.env && $(CXX) -c $< -o $@ $(CXXFLAGS_RECOMP)

build/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	. ./config/toolchain.env && $(CC) -c $< -o $@ $(CFLAGS_RECOMP)

build/replacement_map.json: $(SRC_OBJS) tools/scan_replacement_objects.py
	@mkdir -p build
	$(PYTHON) tools/scan_replacement_objects.py $(SRC_OBJS)

# ---------------------------------------------------------------------
# Runnable rebuilt ELF
# ---------------------------------------------------------------------

$(TEXT_ASM) build/text_functions.tsv build/text_layout.ldinc: tools/prepare_text_split.py $(ORIG_ELF) build/replacement_map.json
	$(PYTHON) tools/prepare_text_split.py $(ORIG_ELF)

asm/section_blobs.s build/linker_sections.ld: tools/prepare_sections.py $(ORIG_ELF) build/text_layout.ldinc
	$(PYTHON) tools/prepare_sections.py $(ORIG_ELF)

build/section_blobs.o: asm/section_blobs.s $(TEXT_ASM)
	@mkdir -p build
	. ./config/toolchain.env && $(CC) -c $(ASMFLAGS) -o $@ asm/section_blobs.s

$(TEXT_OBJ): $(TEXT_ASM)
	@mkdir -p build
	. ./config/toolchain.env && $(CC) -c $(ASMFLAGS) -o $@ $<

$(SECTION_RAW): build/section_blobs.o $(TEXT_OBJ) $(SRC_OBJS) build/linker_sections.ld build/text_layout.ldinc
	. ./config/toolchain.env && $(LD) -T build/linker_sections.ld -o $@ build/section_blobs.o $(TEXT_OBJ) $(SRC_OBJS)

$(SECTION_REBUILT): $(SECTION_RAW) tools/finalize_ps3_elf.py
	$(PYTHON) tools/finalize_ps3_elf.py $(ORIG_ELF) $(SECTION_RAW) $(SECTION_REBUILT)

rebuild sections: $(SECTION_REBUILT)
	@echo "rebuilt $(SECTION_REBUILT)"

# Strict byte check. Expected to fail while source functions are nonmatching.
check: $(SECTION_REBUILT)
	$(PYTHON) tools/compare_loads.py $(ORIG_ELF) $(SECTION_REBUILT)

# ---------------------------------------------------------------------
# Objdiff
# ---------------------------------------------------------------------

build/decomp_db.json: tools/build_decomp_db.py $(ORIG_ELF)
	@mkdir -p build
	$(PYTHON) tools/build_decomp_db.py

decomp-db: build/decomp_db.json

# Generate decomp DB, target asm, and objdiff.json.
# This does NOT build every target object. Objdiff can request individual objs.
objdiff-bootstrap: $(SRC_OBJS) build/decomp_db.json tools/gen_objdiff_targets.py tools/gen_objdiff.py
	$(PYTHON) tools/gen_objdiff_targets.py
	$(PYTHON) tools/gen_objdiff.py
	@echo "objdiff bootstrap complete"

objdiff-config: objdiff-bootstrap

# Objdiff can call: make build/objdiff/target/path/to/file.o
build/objdiff/target/%.o: asm/objdiff/target/%.s
	@mkdir -p $(dir $@)
	$(OBJDIFF_AS) -o $@ $<

# Optional: build all generated target objects once.
objdiff-targets: objdiff-bootstrap tools/build_objdiff_targets.py
	$(PYTHON) tools/build_objdiff_targets.py

# ---------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------

clean:
	rm -rf build
	rm -f asm/section_blobs.s
	rm -f asm/text/text_blobs.s

clean-objdiff:
	rm -rf build/objdiff
	rm -rf asm/objdiff
	rm -f objdiff.json