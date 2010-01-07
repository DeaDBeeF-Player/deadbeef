adplugpath=@top_srcdir@/plugins/adplug

adlibdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = adplug.la

AM_CFLAGS = $(CFLAGS) -std=c99 -I$(adplugpath)/adplug -I$(adplugpath)/libbinio

AM_CPPFLAGS = $(CXXFLAGS) -Dstricmp=strcasecmp -DVERSION=\"2.1\" -I$(adplugpath)/adplug -I$(adplugpath)/libbinio

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
				   adplug/a2m.h\
				   adplug/adl.h\
				   adplug/adlibemu.h\
				   adplug/adplug.h\
				   adplug/adtrack.h\
				   adplug/amd.h\
				   adplug/analopl.h\
				   adplug/bam.h\
				   adplug/bmf.h\
				   adplug/cff.h\
				   adplug/d00.h\
				   adplug/database.h\
				   adplug/debug.h\
				   adplug/dfm.h\
				   adplug/diskopl.h\
				   adplug/dmo.h\
				   adplug/dro.h\
				   adplug/dtm.h\
				   adplug/emuopl.h\
				   adplug/flash.h\
				   adplug/fmc.h\
				   adplug/fmopl.h\
				   adplug/fprovide.h\
				   adplug/hsc.h\
				   adplug/hsp.h\
				   adplug/hybrid.h\
				   adplug/hyp.h\
				   adplug/imf.h\
				   adplug/kemuopl.h\
				   adplug/ksm.h\
				   adplug/lds.h\
				   adplug/mad.h\
				   adplug/mid.h\
				   adplug/mididata.h\
				   adplug/mkj.h\
				   adplug/msc.h\
				   adplug/mtk.h\
				   adplug/opl.h\
				   adplug/player.h\
				   adplug/players.h\
				   adplug/protrack.h\
				   adplug/psi.h\
				   adplug/rad.h\
				   adplug/rat.h\
				   adplug/raw.h\
				   adplug/realopl.h\
				   adplug/rix.h\
				   adplug/rol.h\
				   adplug/s3m.h\
				   adplug/sa2.h\
				   adplug/silentopl.h\
				   adplug/sng.h\
				   adplug/temuopl.h\
				   adplug/u6m.h\
				   adplug/xad.h\
				   adplug/xsm.h\
				   libbinio/binfile.h\
				   libbinio/binio.h\
				   libbinio/binstr.h\
				   libbinio/binwrap.h\
				   libbinio/binfile.cpp\
				   libbinio/binio.cpp\
				   libbinio/binstr.cpp\
				   libbinio/binwrap.cpp

adplug_la_LDFLAGS = -module


