## Process this file with automake to produce Makefile.in

mans = oggdec.1
oggdecsources = oggdec.c

datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

bin_PROGRAMS = oggdec

INCLUDES = @OGG_CFLAGS@ @VORBIS_CFLAGS@ @SHARE_CFLAGS@ @I18N_CFLAGS@

oggdec_LDADD = @LIBICONV@ @SHARE_LIBS@ @VORBISFILE_LIBS@ @VORBIS_LIBS@ @OGG_LIBS@ @I18N_LIBS@
oggdec_DEPENDENCIES = @SHARE_LIBS@

oggdec_SOURCES = $(oggdecsources)

man_MANS = $(mans)
mandir = @MANDIR@

EXTRA_oggdec_SOURCES = $(man_MANS)

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"


