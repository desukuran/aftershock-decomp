# Madness Interactive SX. The rules are in tools/xbox.mk:
#
#   make -j4            build/xbox/default.xbe
#   make xiso           build/xbox/MadnessInteractiveSX.iso
#   make XEMU=1 xiso    the same for xemu (see InitDevice)

# The original's certificate; init flags: mount the utility drive
# (T: xmadness.cfg, Z:).
TITLE_ID := 0x07398CDB
TITLE_NAME := Madness Interactive SX
INIT_FLAGS := 0x1
ISO_NAME := MadnessInteractiveSX

# Text from the original release that the source doesn't carry (the credits,
# the story and tutorial text), extracted from its default.xbe.
GENERATED = $(GEN)/credits.inc $(GEN)/text.h
XEMU_PATCH := tools/xemu_patch.py

include ../../tools/xbox.mk

$(GENERATED): $(ORIGINAL) tools/extract_data.py
	@mkdir -p $(GEN)
	$(PYTHON) -I tools/extract_data.py $(ORIGINAL) $(GEN)
