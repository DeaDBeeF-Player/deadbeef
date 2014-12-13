## Process this file with automake to produce Makefile.in
if HAVE_LIBFLAC
flac_sources = flac.c flac.h easyflac.c easyflac.h
else
flac_sources =
endif

if HAVE_KATE
kate_sources = lyrics.c lyrics.h
else
kate_sources =
endif

SUBDIRS = man

datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

bin_PROGRAMS = oggenc

INCLUDES = @SHARE_CFLAGS@ @OGG_CFLAGS@ @VORBIS_CFLAGS@ @KATE_CFLAGS@ @I18N_CFLAGS@

oggenc_LDADD = @SHARE_LIBS@ \
	       @VORBISENC_LIBS@ @VORBIS_LIBS@ @KATE_LIBS@ @OGG_LIBS@ \
	       @LIBICONV@ @I18N_LIBS@ @FLAC_LIBS@

oggenc_DEPENDENCIES = @SHARE_LIBS@

oggenc_SOURCES = $(flac_sources) $(kate_sources) \
                 oggenc.c audio.c encode.c platform.c resample.c skeleton.c \
                 audio.h encode.h platform.h resample.h skeleton.h


debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
