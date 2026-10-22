# Baku Baku X2 (SHiZNO / AfterShock). The rules are in tools/xbox.mk:
#
#   make -j4            build/xbox/default.xbe
#   make xiso           build/xbox/BakuBaku2.iso

# The original's certificate; init flags: mount the utility drive.
TITLE_ID := 0x0014680D
TITLE_NAME := Baku Baku X²
INIT_FLAGS := 0x1
ISO_NAME := BakuBaku2
# It loads skins/ and D:\shizno.png as well as media/.
DATA_DIRS := media skins shizno.png
LINK_FLAGS := /OPT:REF /OPT:ICF
EXTRA_LIBS := dmusic.lib

include ../../tools/xbox.mk
