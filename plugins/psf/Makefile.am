EXTRA_DIST=eng_psf/peops/reverb.c eng_psf/peops/adsr.c eng_psf/peops/registers.c eng_psf/peops/dma.c eng_psf/peops2/reverb2.c eng_psf/peops2/adsr2.c eng_dsf/arm7memil.c eng_dsf/aicalfo.c eng_ssf/scsplfo.c

if HAVE_PSF
pkglib_LTLIBRARIES = psf.la
psf_la_SOURCES = plugin.c psfmain.c corlett.c\
eng_dsf/eng_dsf.c eng_dsf/dc_hw.c eng_dsf/aica.c eng_dsf/aicadsp.c eng_dsf/arm7.c eng_dsf/arm7i.c\
eng_ssf/m68kcpu.c eng_ssf/m68kopac.c eng_ssf/m68kopdm.c eng_ssf/m68kopnz.c eng_ssf/m68kops.c \
eng_ssf/scsp.c eng_ssf/scspdsp.c eng_ssf/sat_hw.c eng_ssf/eng_ssf.c\
eng_qsf/eng_qsf.c eng_qsf/kabuki.c eng_qsf/qsound.c eng_qsf/z80.c eng_qsf/z80dasm.c\
eng_psf/eng_psf.c eng_psf/psx.c eng_psf/psx_hw.c eng_psf/peops/spu.c \
eng_psf/eng_psf2.c eng_psf/peops2/spu2.c eng_psf/peops2/dma2.c eng_psf/peops2/registers2.c\
eng_psf/eng_spu.c\
ao.h corlett.h cpuintrf.h eng_protos.h mem.h osd_cpu.h\
eng_dsf/aicadsp.h eng_dsf/aica.h eng_dsf/arm7.h eng_dsf/arm7i.h eng_dsf/arm7thumb.h eng_dsf/dc_hw.h\
eng_ssf/m68kconf.h eng_ssf/m68kcpu.h eng_ssf/m68k.h eng_ssf/m68kmame.h eng_ssf/m68kops.h eng_ssf/sat_hw.h eng_ssf/scspdsp.h eng_ssf/scsp.h \
eng_qsf/qsound.h eng_qsf/z80dasm.h eng_qsf/z80.h\
eng_psf/cpuintrf.h eng_psf/mamemem.h eng_psf/psx.h\
eng_psf/peops/adsr.h eng_psf/peops/dma.h eng_psf/peops/externals.h eng_psf/peops/gauss_i.h eng_psf/peops/registers.h eng_psf/peops/regs.h eng_psf/peops/spu.h eng_psf/peops/stdafx.h\
eng_psf/peops2/adsr.h eng_psf/peops2/dma.h eng_psf/peops2/externals.h eng_psf/peops2/gauss_i.h eng_psf/peops2/psemuxa.h eng_psf/peops2/registers.h eng_psf/peops2/regs.h eng_psf/peops2/reverb.h eng_psf/peops2/spu.h eng_psf/peops2/stdafx.h 

psf_la_LDFLAGS = -module -avoid-version $(ZLIB_LIBS)

psf_la_LIBADD = $(LDADD)
psf_la_CFLAGS = $(CFLAGS) $(ZLIB_CFLAGS) -Wall -DHAS_PSXCPU=1 -I.. -Ieng_ssf -Ieng_qsf -Ieng_dsf -lm -fPIC -I@top_srcdir@/include
endif

