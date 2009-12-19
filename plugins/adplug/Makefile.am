adlibdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = adplug.la
adplug_la_SOURCES = adplug-db.cpp\
				   plugin.c\
				   adplug/adplug.cpp\
				   adplug/emuopl.cpp\
				   adplug/fmopl.c\
				   adplug/diskopl.cpp\
				   adplug/debug.c\
				   adplug/debug.h\
				   adplug/fprovide.cpp\
				   adplug/player.cpp\
				   adplug/database.cpp\
				   adplug/hsc.cpp\
				   adplug/sng.cpp\
				   adplug/imf.cpp\
				   adplug/players.cpp\
				   adplug/protrack.cpp\
				   adplug/a2m.cpp\
				   adplug/adtrack.cpp\
				   adplug/amd.cpp\
				   adplug/bam.cpp\
				   adplug/d00.cpp\
				   adplug/dfm.cpp\
				   adplug/hsp.cpp\
				   adplug/ksm.cpp\
				   adplug/mad.cpp\
				   adplug/mid.cpp\
				   adplug/mkj.cpp\
				   adplug/cff.cpp\
				   adplug/dmo.cpp\
				   adplug/s3m.cpp\
				   adplug/dtm.cpp\
				   adplug/fmc.cpp\
				   adplug/mtk.cpp\
				   adplug/rad.cpp\
				   adplug/raw.cpp\
				   adplug/sa2.cpp\
				   adplug/xad.cpp\
				   adplug/flash.cpp\
				   adplug/bmf.cpp\
				   adplug/hybrid.cpp\
				   adplug/hyp.cpp\
				   adplug/psi.cpp\
				   adplug/rat.cpp\
				   adplug/u6m.cpp\
				   adplug/rol.cpp\
				   adplug/mididata.h\
				   adplug/xsm.cpp\
				   adplug/adlibemu.c\
				   adplug/dro.cpp\
				   adplug/lds.cpp\
				   adplug/realopl.cpp\
				   adplug/analopl.cpp\
				   adplug/temuopl.cpp\
				   adplug/msc.cpp\
				   adplug/rix.cpp\
				   adplug/adl.cpp\
				   libbinio/binfile.h\
				   libbinio/binio.h\
				   libbinio/binstr.h\
				   libbinio/binwrap.h\
				   libbinio/binfile.cpp\
				   libbinio/binio.cpp\
				   libbinio/binstr.cpp\
				   libbinio/binwrap.cpp

adplug_la_LDFLAGS = -module

AM_CXXFLAGS = $(CFLAGS) -Dstricmp=strcasecmp -DVERSION=\"2.1\" -I adplug -I libbinio
AM_CFLAGS = $(CFLAGS) -std=c99
