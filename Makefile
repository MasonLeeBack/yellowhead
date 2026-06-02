PYTHON ?= .venv/bin/python

TOOLS_DIR := $(CURDIR)/tools

AS      := $(TOOLS_DIR)/ppu-lv2-as
LD      := $(TOOLS_DIR)/ppu-lv2-ld
CXX     := $(TOOLS_DIR)/ppu-lv2-g++
READELF := $(TOOLS_DIR)/ppu-lv2-readelf

ORIG_ELF := orig/EBOOT.ELF
REBUILT  := build/EBOOT.relinked.elf
SECTION_RAW := build/EBOOT.sections.raw.elf
SECTION_REBUILT := build/EBOOT.sections.elf
TEXT_ASM := asm/text/text_blobs.s
TEXT_OBJ := build/text_blobs.o

.PHONY: all clean toolcheck info compare

all: $(REBUILT) compare

toolcheck:
	. ./config/toolchain.env && $(CXX) --version
	. ./config/toolchain.env && $(AS) --version
	. ./config/toolchain.env && $(LD) --version

info:
	. ./config/toolchain.env && $(READELF) -h $(ORIG_ELF)
	. ./config/toolchain.env && $(READELF) -l $(ORIG_ELF)
	. ./config/toolchain.env && $(READELF) -S $(ORIG_ELF)

asm/blob_segments.s build/linker.ld: tools/prepare_segments.py $(ORIG_ELF)
	$(PYTHON) tools/prepare_segments.py $(ORIG_ELF)

build/blob_segments.o: asm/blob_segments.s
	@mkdir -p build
	. ./config/toolchain.env && $(AS) -mppc64 -o $@ $<

$(REBUILT): build/blob_segments.o build/linker.ld
	. ./config/toolchain.env && $(LD) -T build/linker.ld -o $@ build/blob_segments.o

sections: $(SECTION_REBUILT)
	$(PYTHON) tools/compare_loads.py $(ORIG_ELF) $(SECTION_REBUILT)

asm/section_blobs.s build/linker_sections.ld: tools/prepare_sections.py $(ORIG_ELF) build/text_layout.ldinc
	$(PYTHON) tools/prepare_sections.py $(ORIG_ELF)

build/section_blobs.o: asm/section_blobs.s $(TEXT_ASM)
	@mkdir -p build
	. ./config/toolchain.env && $(AS) -mppc64 -o $@ asm/section_blobs.s

$(SECTION_RAW): build/section_blobs.o $(TEXT_OBJ) build/linker_sections.ld build/text_layout.ldinc
	. ./config/toolchain.env && $(LD) -T build/linker_sections.ld -o $@ build/section_blobs.o $(TEXT_OBJ)

$(SECTION_REBUILT): $(SECTION_RAW) tools/finalize_ps3_elf.py
	$(PYTHON) tools/finalize_ps3_elf.py $(ORIG_ELF) $(SECTION_RAW) $(SECTION_REBUILT)

$(TEXT_ASM) build/text_functions.tsv build/text_layout.ldinc: tools/prepare_text_split.py $(ORIG_ELF)
	$(PYTHON) tools/prepare_text_split.py $(ORIG_ELF)

$(TEXT_OBJ): $(TEXT_ASM)
	@mkdir -p build
	. ./config/toolchain.env && $(AS) -mppc64 -o $@ $<

compare: $(REBUILT)
	$(PYTHON) tools/compare_loads.py $(ORIG_ELF) $(REBUILT)

clean:
	rm -rf build/blob_segments.o build/EBOOT.relinked.elf build/linker.ld asm/blob_segments.s