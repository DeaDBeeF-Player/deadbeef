# Makefile to build sc68 replay
#
# by Benjamin Gerard
#
# Time-stamp: <2011-09-12 17:56:34 ben>

pic_failed := hpn-jam.s tsd-jam.s 

pic_sources := blipp079.s blipblop.s chipmon2.s cosotfmx.s crapman1.s	\
crapman2.s deltamusic2.s digitfmx.s fc13_14.s fred.s lastninja3.s	\
lx_mmme.s mcoder.s megatizer.s midi.s mmme.s mmmesid.s musmon1.s	\
musmon2.s ninjatfmx.s note.s note2.s psg1.s puma99.s quartet.s		\
sndh_ice.s tao_digi.s tao_hubbard.s tao_ms211.s tao_ms25.s tao_ms27.s	\
tao_tsd.s tcbtracker.s tfmx.s tfmx_old.s tfmx_st.s timerc.s tsd1.s	\
tur2tfmx.s wodtfmx.s

abs_sources := jamcracker10.s puma2.s bs22.s deltamusic1.s	\
quartet_mindbomb.s sidmon1.s sidmon2.s soundfx.s ssd1.s

mus_sources := sh11-alter_m.s sh11-unkown.s

pic_binaries := $(pic_sources:%.s=bin/pic/%.bin)
abs_binaries := $(abs_sources:%.s=bin/abs/%.bin)
mus_binaries := $(mus_sources:%.s=bin/mus/%.bin)

all: pic abs

pic: $(pic_binaries)
abs: $(abs_binaries)
mus: $(mus_binaries)

bin/pic/%.bin: %.s
	@test -d '$(@D)' || mkdir -p '$(@D)'
	@echo "AS PIC '$<'"
	@as68 -p -o $@ $< >/dev/null

bin/mus/%.bin: %.s
	@test -d '$(@D)' || mkdir -p '$(@D)'
	@as68 -p -o $@ $< >/dev/null

bin/abs/%.bin: %.s
	@test -d '$(@D)' || mkdir -p '$(@D)'
	@test -d 'rel'   || mkdir -p 'rel'
	@echo "AS ABS '$<'"
	@as68 -r -o $@ -s rel/$*.rel $< >/dev/null


install: all install_abs install_pic

install_%: all
	@test '!' -d 'bin/$*' || find 'bin/$*/' -type f -name '*.bin' | \
	while read l; do \
		 d=../Replay/`basename $$l`;\
		 if [ ! -e $$d ]; then echo "NEW $$l"; \
		 elif cmp -s $$l $$d; then echo "EQU $$l";\
		 else echo "DIF $$l"; fi;\
	done

clean:
	test '!' -d 'bin' || rm -f -- `find 'bin/' -type f -name '*.bin'`
	test '!' -d 'rel' || rm -f -- `find 'rel/' -type f -name '*.rel'`

.PHONY: pic abs install clean
