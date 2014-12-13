## Process this file with automake to produce Makefile.in
if HAVE_LIBFLAC
flac_sources = flac_format.c easyflac.c easyflac.h 
else
flac_sources =
endif
if HAVE_LIBSPEEX
speex_sources = speex_format.c
else
speex_sources =
endif
if HAVE_OV_READ_FILTER
vgfilter_sources = vgfilter.c vgfilter.h
else
vgfilter_sources =
endif

datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DSYSCONFDIR=\"$(sysconfdir)\" -DLOCALEDIR=\"$(localedir)\" @DEFS@

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)
mandir = @MANDIR@

bin_PROGRAMS = ogg123

INCLUDES = @OGG_CFLAGS@ @VORBIS_CFLAGS@ @AO_CFLAGS@ @CURL_CFLAGS@ \
           @PTHREAD_CFLAGS@ @SHARE_CFLAGS@ @I18N_CFLAGS@

ogg123_LDADD = @SHARE_LIBS@ \
	       @VORBISFILE_LIBS@ @VORBIS_LIBS@ @OGG_LIBS@ @AO_LIBS@ \
               @SOCKET_LIBS@ @LIBICONV@ @CURL_LIBS@ @PTHREAD_CFLAGS@ \
               @PTHREAD_LIBS@ @I18N_LIBS@ @FLAC_LIBS@ @SPEEX_LIBS@

ogg123_DEPENDENCIES = @SHARE_LIBS@
ogg123_SOURCES = audio.c buffer.c callbacks.c \
                cfgfile_options.c cmdline_options.c \
                file_transport.c format.c http_transport.c \
                ogg123.c oggvorbis_format.c playlist.c \
                status.c remote.c transport.c vorbis_comments.c \
                audio.h buffer.h callbacks.h compat.h \
                cfgfile_options.h cmdline_options.h \
                format.h ogg123.h playlist.h status.h \
                transport.h remote.h vorbis_comments.h \
                $(flac_sources) $(speex_sources) $(vgfilter_sources)

man_MANS = ogg123.1
doc_DATA = ogg123rc-example

EXTRA_ogg123_SOURCES = \
                $(man_MANS) $(doc_DATA)

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
