EXTRA_DIST=\
	aosdk/eng_dsf/aicalfo.c\
	aosdk/eng_dsf/arm7memil.c\
	aosdk/eng_psf/peops/adsr.c\
	aosdk/eng_psf/peops/dma.c\
	aosdk/eng_psf/peops/registers.c\
	aosdk/eng_psf/peops/reverb.c\
	aosdk/eng_psf/peops2/adsr2.c\
	aosdk/eng_psf/peops2/reverb2.c\
	aosdk/eng_ssf/scsplfo.c

if HAVE_PSF
pkglib_LTLIBRARIES = psf.la
psf_la_SOURCES = plugin.c\
	aosdk/ao.h\
	aosdk/corlett.c\
	aosdk/corlett.h\
	aosdk/cpuintrf.h\
	aosdk/eng_dsf/aica.c\
	aosdk/eng_dsf/aica.h\
	aosdk/eng_dsf/aicadsp.c\
	aosdk/eng_dsf/aicadsp.h\
	aosdk/eng_dsf/arm7.c\
	aosdk/eng_dsf/arm7.h\
	aosdk/eng_dsf/arm7i.c\
	aosdk/eng_dsf/arm7i.h\
	aosdk/eng_dsf/arm7thumb.h\
	aosdk/eng_dsf/dc_hw.c\
	aosdk/eng_dsf/dc_hw.h\
	aosdk/eng_dsf/eng_dsf.c\
	aosdk/eng_protos.h\
	aosdk/eng_psf/cpuintrf.h\
	aosdk/eng_psf/eng_psf.c\
	aosdk/eng_psf/eng_psf2.c\
	aosdk/eng_psf/eng_spu.c\
	aosdk/eng_psf/mamemem.h\
	aosdk/eng_psf/peops/adsr.h\
	aosdk/eng_psf/peops/dma.h\
	aosdk/eng_psf/peops/externals.h\
	aosdk/eng_psf/peops/gauss_i.h\
	aosdk/eng_psf/peops/registers.h\
	aosdk/eng_psf/peops/regs.h\
	aosdk/eng_psf/peops/spu.c\
	aosdk/eng_psf/peops/spu.h\
	aosdk/eng_psf/peops/stdafx.h\
	aosdk/eng_psf/peops2/adsr.h\
	aosdk/eng_psf/peops2/dma.h\
	aosdk/eng_psf/peops2/dma2.c\
	aosdk/eng_psf/peops2/externals.h\
	aosdk/eng_psf/peops2/gauss_i.h\
	aosdk/eng_psf/peops2/psemuxa.h\
	aosdk/eng_psf/peops2/registers.h\
	aosdk/eng_psf/peops2/registers2.c\
	aosdk/eng_psf/peops2/regs.h\
	aosdk/eng_psf/peops2/reverb.h\
	aosdk/eng_psf/peops2/spu.h\
	aosdk/eng_psf/peops2/spu2.c\
	aosdk/eng_psf/peops2/stdafx.h\
	aosdk/eng_psf/psx.c\
	aosdk/eng_psf/psx.h\
	aosdk/eng_psf/psx_hw.c\
	aosdk/eng_qsf/eng_qsf.c\
	aosdk/eng_qsf/kabuki.c\
	aosdk/eng_qsf/qsound.c\
	aosdk/eng_qsf/qsound.h\
	aosdk/eng_qsf/z80.c\
	aosdk/eng_qsf/z80.h\
	aosdk/eng_qsf/z80dasm.c\
	aosdk/eng_qsf/z80dasm.h\
	aosdk/eng_ssf/eng_ssf.c\
	aosdk/eng_ssf/m68k.h\
	aosdk/eng_ssf/m68kconf.h\
	aosdk/eng_ssf/m68kcpu.c\
	aosdk/eng_ssf/m68kcpu.h\
	aosdk/eng_ssf/m68kmame.h\
	aosdk/eng_ssf/m68kopac.c\
	aosdk/eng_ssf/m68kopdm.c\
	aosdk/eng_ssf/m68kopnz.c\
	aosdk/eng_ssf/m68kops.c\
	aosdk/eng_ssf/m68kops.h\
	aosdk/eng_ssf/sat_hw.c\
	aosdk/eng_ssf/sat_hw.h\
	aosdk/eng_ssf/scsp.c\
	aosdk/eng_ssf/scsp.h\
	aosdk/eng_ssf/scspdsp.c\
	aosdk/eng_ssf/scspdsp.h\
	aosdk/mem.h\
	aosdk/osd_cpu.h\
	aosdk/psfmain.c

psf_la_LDFLAGS = -module -avoid-version $(ZLIB_LIBS)

psf_la_LIBADD = $(LDADD)

psfpath=@top_srcdir@/plugins/psf

psf_la_CFLAGS = $(CFLAGS) $(ZLIB_CFLAGS) -Wall -DHAS_PSXCPU=1 -I$(psfpath)/aosdk -I$(psfpath)/.. -I$(psfpath)/aosdk/eng_ssf -I$(psfpath)/aosdk/eng_qsf -I$(psfpath)/aosdk/eng_dsf -lm -fPIC -I@top_srcdir@/include
endif

