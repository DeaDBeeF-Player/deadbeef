## Process this file with automake to produce Makefile.in

mans = vcut.1
vcutsources = vcut.c vcut.h

datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

bin_PROGRAMS = vcut
mandir = @MANDIR@
man_MANS = $(mans)

INCLUDES = @OGG_CFLAGS@ @VORBIS_CFLAGS@ @SHARE_CFLAGS@ @I18N_CFLAGS@

vcut_LDADD = @VORBIS_LIBS@ @OGG_LIBS@ @I18N_LIBS@

vcut_SOURCES = $(vcutsources)
EXTRA_vcut_SOURCES = $(man_MANS)

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
