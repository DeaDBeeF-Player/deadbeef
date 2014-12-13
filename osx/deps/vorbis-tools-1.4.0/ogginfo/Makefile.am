## Process this file with automake to produce Makefile.in

mans = ogginfo.1
ogginfosources = ogginfo2.c theora.c

noinst_HEADERS = \
	theora.h

datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

bin_PROGRAMS = ogginfo
mandir = @MANDIR@
man_MANS = $(mans)

INCLUDES = @SHARE_CFLAGS@ @OGG_CFLAGS@ @VORBIS_CFLAGS@ @KATE_CFLAGS@ @I18N_CFLAGS@

ogginfo_LDADD = @SHARE_LIBS@ @VORBIS_LIBS@ @KATE_LIBS@ @OGG_LIBS@ @LIBICONV@ @I18N_LIBS@
ogginfo_DEPENDENCIES = @SHARE_LIBS@

ogginfo_SOURCES = $(ogginfosources)
EXTRA_ogginfo_SOURCES = $(man_MANS) 

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
