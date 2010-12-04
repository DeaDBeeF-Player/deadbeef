CC=gcc

ZLIB_LIBS=-lz

CFLAGS=-Wall -DPATH_MAX=1024 -DHAS_PSXCPU=1 -I../.. -I./ -Ieng_ssf -Ieng_qsf -Ieng_dsf -lm

LDFLAGS=-module -shared -fPIC $(ZLIB_LIBS) -lm

SOURCES=plugin.c main.c corlett.c\
eng_dsf/eng_dsf.c eng_dsf/dc_hw.c eng_dsf/aica.c eng_dsf/aicadsp.c eng_dsf/arm7.c eng_dsf/arm7i.c\
eng_ssf/m68kcpu.c eng_ssf/m68kopac.c eng_ssf/m68kopdm.c eng_ssf/m68kopnz.c eng_ssf/m68kops.c \
eng_ssf/scsp.c eng_ssf/scspdsp.c eng_ssf/sat_hw.c eng_ssf/eng_ssf.c\
eng_qsf/eng_qsf.c eng_qsf/kabuki.c eng_qsf/qsound.c eng_qsf/z80.c eng_qsf/z80dasm.c\
eng_psf/eng_psf.c eng_psf/psx.c eng_psf/psx_hw.c eng_psf/peops/spu.c \
eng_psf/eng_psf2.c eng_psf/peops2/spu2.c eng_psf/peops2/dma2.c eng_psf/peops2/registers2.c\
eng_psf/eng_spu.c

#EXTRA_DIST=eng_psf/peops/reverb.c eng_psf/peops/adsr.c eng_psf/peops/registers.c eng_psf/peops/dma.c eng_psf/peops2/spu2.c eng_psf/peops2/reverb2.c eng_psf/peops2/adsr2.c eng_dsf/arm7memil.c eng_dsf/aicalfo.c eng_ssf/scsplfo.c

OBJECTS=$(SOURCES:.c=.o)

OUT=ao.so

all: $(SOURCES) $(OUT)

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	echo sources: $(SOURCES)
	echo objects: $(OBJECTS)
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm $(OBJECTS) $(OUT)
