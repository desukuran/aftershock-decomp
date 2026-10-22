# The Xbox build shared by every game: default.xbe from games/<game>/src/
# with the XDK's compiler (MSVC 7.1), linker and imagebld. On Linux they run
# under Wine. Each game's Makefile sets its certificate and includes this,
# from its own folder:
#
#   make -j4                    build/xbox/default.xbe
#   make info                   show what was found
#   make xiso                   build/xbox/<ISO_NAME>.iso: the rebuild and the
#                               original's media/, to play
#   make original-xiso          build/xbox/Original.iso, to compare against
#   make XEMU=1 ...             the same in build/xemu, with the game's xemu
#                               workarounds (XEMU_FIX in the source)
#
# XDK is the XDK's folder (default <repo>/xdk): an installed one (xbox/bin,
# xbox/include, xbox/lib) or a flat copy (bin, include, lib). The games were
# built with XDK 5558.
#
# A game's Makefile sets:
#   TITLE_ID, TITLE_NAME, INIT_FLAGS   the original's certificate
#   ISO_NAME                           the disc image's name
# and may set:
#   DATA_DIRS                          the release's folders (and files) for
#                                      the disc (default media)
#   TITLE_VERSION                      the certificate's version (default 0, as the originals)
#   LINK_FLAGS                         extra linker flags the original was
#                                      linked with
#   EXTRA_LIBS                         XDK libraries beyond the usual ones
#   EXTRA_CFLAGS                       compiler flags the original differs in
#   GENERATED                          files the sources include from $(GEN),
#                                      with rules to make them
#   XEMU_PATCH                         a script <in.xbe> <out.xbe> that gives
#                                      the original XEMU=1's workarounds

REPO ?= ../..
XDK ?= $(REPO)/xdk
ifeq ($(XEMU),1)
OUT := build/xemu
XEMU_DEFS := /DXEMU_FIX
else
OUT := build/xbox
endif
OBJ := $(OUT)/obj
GEN := $(OUT)/gen
ORIGINAL ?= original/default.xbe
# The release's data folders that go on the disc next to default.xbe.
DATA_DIRS ?= media
MEDIA := $(addprefix $(dir $(ORIGINAL)),$(DATA_DIRS))

ifeq ($(OS),Windows_NT)
WINE :=
HOST_OS := __WIN32__
# python3 is often only the Microsoft Store alias on Windows.
PYTHON ?= python
else
WINE ?= WINEDEBUG=-all wine
HOST_OS := $(if $(filter Darwin,$(shell uname -s)),__DARWIN__,__LINUX__)
PYTHON ?= python3
endif
HOSTCC ?= cc

XDK_ROOT := $(firstword $(wildcard $(XDK)/xbox/include $(XDK)/Xbox/include $(XDK)/Xbox/Include))
XDK_ROOT := $(if $(XDK_ROOT),$(patsubst %/,%,$(dir $(XDK_ROOT))),$(XDK))
XDK_INC := $(firstword $(wildcard $(XDK_ROOT)/include $(XDK_ROOT)/Include))
XDK_LIB := $(firstword $(wildcard $(XDK_ROOT)/lib $(XDK_ROOT)/Lib))
XDK_BIN := $(firstword $(wildcard $(XDK_ROOT)/bin $(XDK_ROOT)/Bin))
find_tool = $(firstword $(wildcard $(addprefix $(1)/,$(2))))
CL_EXE := $(call find_tool,$(XDK_BIN)/vc71,CL.Exe CL.exe cl.exe)
LINK_EXE := $(call find_tool,$(XDK_BIN)/vc71,Link.Exe LINK.EXE Link.exe link.exe)
IMAGEBLD_EXE := $(call find_tool,$(XDK_BIN),imagebld.exe ImageBld.exe IMAGEBLD.EXE)

CL = $(WINE) $(CL_EXE)
LINK = $(WINE) $(LINK_EXE)
IMAGEBLD = $(WINE) $(abspath $(IMAGEBLD_EXE))

# As the originals: optimized, buffer security checks, SSE, C++ exceptions
# (for std::string), no RTTI.
CFLAGS := /nologo /c /Z7 /O2 /GS /arch:SSE /EHsc /GR- /MT /W3 /D_XBOX /DNDEBUG /DWIN32 $(XEMU_DEFS) $(EXTRA_CFLAGS) \
	/I$(XDK_INC) /I$(GEN)

SRCS := $(sort $(wildcard src/*.cpp))
OBJS := $(patsubst src/%.cpp,$(OBJ)/%.obj,$(SRCS))
HEADERS := $(wildcard src/*.h)

LIBS := $(EXTRA_LIBS) xapilib.lib d3d8.lib d3dx8.lib xgraphics.lib dsound.lib xacteng.lib xboxkrnl.lib libcmt.lib libcpmt.lib

all: $(OUT)/default.xbe

info:
	@echo "Game:       $(TITLE_NAME)"
	@echo "XDK:        $(XDK)"
	@echo "  include:  $(or $(XDK_INC),(not found))"
	@echo "  lib:      $(or $(XDK_LIB),(not found))"
	@echo "  cl:       $(or $(CL_EXE),(not found))"
	@echo "  link:     $(or $(LINK_EXE),(not found))"
	@echo "  imagebld: $(or $(IMAGEBLD_EXE),(not found))"
	@echo "Wine:       $(or $(WINE),(not used))"
	@echo "Original:   $(if $(wildcard $(ORIGINAL)),$(ORIGINAL),(not found: $(ORIGINAL)))"

# Compiles one file; the full output goes to <object>.log.
$(OBJ)/%.obj: src/%.cpp $(HEADERS) $(GENERATED)
	@mkdir -p $(dir $@)
	@$(CL) $(CFLAGS) /Fo$@ $< > $@.log 2>&1; status=$$?; grep -E " (error|warning) " $@.log | head -30; \
		if [ $$status -ne 0 ]; then rm -f $@; exit 1; fi

# Every member offset and class size written in src/*.h, checked by the compiler.
$(OBJ)/layout_check.obj: $(HEADERS) $(GENERATED) $(REPO)/tools/gen_src_structs.py
	@mkdir -p $(dir $@)
	@$(PYTHON) -I $(REPO)/tools/gen_src_structs.py > /dev/null
	@$(CL) $(CFLAGS) /Isrc /Fo$@ build/layout_check.cpp > $@.log 2>&1; status=$$?; grep -E " (error|warning) " $@.log | head -30; \
		if [ $$status -ne 0 ]; then rm -f $@; exit 1; fi

objs: $(OBJS) $(OBJ)/layout_check.obj

$(OUT)/default.exe: $(OBJS) $(OBJ)/layout_check.obj
	@echo "Linking $@"
	@$(LINK) /nologo /SUBSYSTEM:XBOX /FIXED:NO /DEBUG /INCREMENTAL:NO $(LINK_FLAGS) /OUT:$@ /MAP:$(OUT)/default.map \
		/LIBPATH:$(XDK_LIB) $(OBJS) $(LIBS) > $(OUT)/link.log 2>&1 || (cat $(OUT)/link.log; false)

# The original's certificate: title ID, media types, regions, version 1.00.
TITLE_VERSION ?= 0x0
IMAGEBLD_FLAGS := /NOLOGO /NOLIBWARN /STACK:0x10000 /INITFLAGS:$(INIT_FLAGS) /TESTID:$(TITLE_ID) \
	"/TESTNAME:$(TITLE_NAME)" /TESTMEDIATYPES:0x207 /TESTREGION:0x80000007 /TESTRATINGS:0xFFFFFFFF \
	/TESTVERSION:$(TITLE_VERSION)

$(OUT)/default.xbe: $(OUT)/default.exe
	cd $(OUT) && $(IMAGEBLD) $(IMAGEBLD_FLAGS) /IN:default.exe /OUT:debug.xbe
	$(PYTHON) -I $(REPO)/tools/xbe.py retail $(OUT)/debug.xbe $@
	@echo "Built $@"

# extract-xiso, built from thirdparty/ unless EXTRACT_XISO names one.
EXTRACT_XISO ?= $(REPO)/build/tools/extract-xiso
$(REPO)/build/tools/extract-xiso: $(REPO)/thirdparty/extract-xiso/extract-xiso.c
	@mkdir -p $(dir $@)
	$(HOSTCC) -O2 -w -D$(HOST_OS) -o $@ $<

# The disc: the rebuilt default.xbe with the original release's media/.
XISO_DIR := $(OUT)/disc
$(OUT)/$(ISO_NAME).iso: $(OUT)/default.xbe $(EXTRACT_XISO) $(shell find $(MEDIA) -type f 2>/dev/null)
	rm -rf $(XISO_DIR) && mkdir -p $(XISO_DIR)
	cp -r $(MEDIA) $(XISO_DIR)/
	cp $(OUT)/default.xbe $(XISO_DIR)/default.xbe
	rm -f $@
	$(EXTRACT_XISO) -c $(XISO_DIR) $@ > $(OUT)/xiso.log || (cat $(OUT)/xiso.log; false)
	@echo "Built $@"

xiso: $(OUT)/$(ISO_NAME).iso

# The original game on a disc, to compare against; with XEMU=1 it gets the
# same workarounds as the rebuild (XEMU_PATCH).
ORIG_DIR := $(OUT)/disc_orig
$(OUT)/Original.iso: $(ORIGINAL) $(EXTRACT_XISO) $(XEMU_PATCH)
	rm -rf $(ORIG_DIR) && mkdir -p $(ORIG_DIR)
	cp -r $(MEDIA) $(ORIG_DIR)/
ifeq ($(XEMU)$(if $(XEMU_PATCH),1),11)
	$(PYTHON) -I $(XEMU_PATCH) $(ORIGINAL) $(ORIG_DIR)/default.xbe
else
	cp $(ORIGINAL) $(ORIG_DIR)/default.xbe
endif
	rm -f $@
	$(EXTRACT_XISO) -c $(ORIG_DIR) $@ > $(OUT)/xiso_orig.log || (cat $(OUT)/xiso_orig.log; false)
	@echo "Built $@"

original-xiso: $(OUT)/Original.iso

clean:
	rm -rf $(OUT)

.PHONY: all info objs clean xiso original-xiso
