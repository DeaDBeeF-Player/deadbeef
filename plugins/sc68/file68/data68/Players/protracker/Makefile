# Makefile for sc68 Amiga Protracker replay
#

ASM		= vasmm68k_mot
ASMFLAGS	= -Fbin -devpac
TARGET		= pt110bfix.bin
REPLAY		= ../../Replay/protracker.bin

all: $(TARGET)

install: $(TARGET)
	@cp -v -- $< $(REPLAY)

uninstall: ;@test ! -e $(REPLAY) || rm -v $(REPLAY)

%.bin: %.s
	@$(ASM) $(ASMFLAGS) -o $@ $<

clean: ; @rm -f -- $(REPLAY) $(TARGET)

.PHONY: all clean install uninstall

