# Builds every game (or one: make pool, make madness); arguments after the
# game names go to each game's Makefile (games/<game>/Makefile), e.g.
#   make -j4 madness XEMU=1 TARGET=xiso
GAMES := $(patsubst games/%/Makefile,%,$(wildcard games/*/Makefile))
TARGET ?= all

all: $(GAMES)

$(GAMES):
	$(MAKE) -C games/$@ $(TARGET)

clean:
	for g in $(GAMES); do $(MAKE) -C games/$$g clean; done

.PHONY: all clean $(GAMES)
