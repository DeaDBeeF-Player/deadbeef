# Makefile for sc68 SSD player
#
# by Benjamin Gerard AKA Ben / OVR
#
# Override VERSION: >make VERSION=xxx clean install

VERSION=106
VERSIONS=103 104 105 106

AS=vasmm68k_mot
BINFLAGS=-m68000 -Fbin -devpac
TOSFLAGS=-m68000 -Ftos -devpac
ZIPFILE:=ssd-$(shell date +%F).zip
SSDS:=$(foreach VER,$(VERSIONS),ssd-$(VER).s)
BINS:=$(SSDS:%.s=%.bin)

## You should not edit after this

all: ssd1.bin

ssd: $(BINS)

install: ssd1.bin
	@echo "CPY $< -> $@"
	@cp -v -- $< ../../Replay/$<

ssd-103.bin: ssd-103.tos
	@echo "BIN $< -> $@"
	@dd if=$< of=$@ bs=1 skip=28 status=none

ssd-%.bin: ssd-%.s
	@echo "ASM $< -> $@"
	@$(AS) $(BINFLAGS) -o $@ $< >/dev/null

ssd.bin: ssd-$(VERSION).bin
	@echo "CPY $< -> $@"
	@cp -- "$<" "$@"

ssd1.bin: ssd1-sc68.s ssd.bin
	@echo "ASM $< -> $@"
	@$(AS) $(BINFLAGS) -DVERSION=$(VERSION) -o $@ $< >/dev/null

%.tos: %.s
	@echo "TOS $< -> $@ ($(VERSION))"
	@$(AS) $(TOSFLAGS) -DTEST=0 -o $@ $<

zip: $(ZIPFILE)

$(ZIPFILE): README Makefile ssd1-sc68.s $(BINS) $(SSDS)
	@echo "ZIP $@"
	@zip $@ $^

clean:
	-rm -f -- ssd1.bin ssd.bin ssd-1[0-9][0-9].bin

.PHONY: all clean install ssd zip

.INTERMEDIATE: ssd-103.tos
