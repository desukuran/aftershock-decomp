# Star Wars: A New Hope for Homebrew (v1.3). The rules are in tools/xbox.mk:
#
#   make -j4            build/xbox/default.xbe
#   make xiso           build/xbox/StarWars.iso

# The original's certificate; init flags: mount the utility drive.
TITLE_ID := 0x00A9E351
TITLE_NAME := STAR WARS [AfterShock]
INIT_FLAGS := 0x1
ISO_NAME := StarWars
# It loads from hud/ and models/ as well as media/.
DATA_DIRS := media hud models
# Linked like the original: unreferenced code dropped and identical functions
# folded (the original shares one function between CTie and CInterceptor).
LINK_FLAGS := /OPT:REF /OPT:ICF

# The intro's scrolling text, extracted from the original's default.xbe.
GENERATED = $(GEN)/crawl.inc

include ../../tools/xbox.mk

$(GENERATED): $(ORIGINAL) tools/extract_data.py
	@mkdir -p $(GEN)
	$(PYTHON) -I tools/extract_data.py $(ORIGINAL) $(GEN)
