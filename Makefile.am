SUBDIRS = pixmaps\
		  icons\
		  external\
		  shared\
		  analyzer\
		  scope\
		  ${PLUGINS_DIRS}\
		  po\
		  intl

bin_PROGRAMS = deadbeef

INTLTOOL_FILES = \
		intltool-extract.in \
		intltool-merge.in \
		intltool-update.in

DISTCLEANFILES = \
		intltool-extract \
		intltool-merge \
		intltool-update

deadbeef_SOURCES =\
	buffered_file_writer.c buffered_file_writer.h\
	conf.c  conf.h\
	cueutil.c cueutil.h playlist.c playlist.h \
	decodedblock.c decodedblock.h\
	dsp.c dsp.h\
	dsppreset.c dsppreset.h\
	escape.c escape.h\
	external/wcwidth/wcwidth.c external/wcwidth/wcwidth.h\
	fastftoi.h\
	fft.c fft.h\
	gettext.h\
	handler.c handler.h\
	junklib.h junklib.c utf8.c utf8.h\
	logger.c logger.h\
	main.c common.h deadbeef.h\
	md5/md5.c md5/md5.h\
	messagepump.c messagepump.h\
	metacache.c metacache.h\
	playmodes.c playmodes.h\
	playqueue.c playqueue.h\
	plmeta.c plmeta.h\
	pltmeta.c pltmeta.h\
	plugins.c plugins.h moduleconf.h\
	premix.c premix.h\
	replaygain.c replaygain.h\
	resizable_buffer.c resizable_buffer.h\
	ringbuf.c ringbuf.h\
	sort.c sort.h\
	strdupa.h\
	streamer.c streamer.h\
	streamreader.c streamreader.h\
	tf.c tf.h\
	threading_pthread.c threading.h\
	u8_lc_map.h\
	u8_uc_map.h\
	vfs.c vfs.h vfs_stdio.c\
	viz.c viz.h\
	volume.c volume.h
	
#	ConvertUTF/ConvertUTF.c ConvertUTF/ConvertUTF.h

sdkdir = $(pkgincludedir)
sdk_HEADERS = deadbeef.h
deadbeef_LDADD = $(LDADD) $(ICONV_LIB) $(DL_LIBS) -lm -lpthread $(DISPATCH_LIBS) $(LTLIBINTL) shared/libctmap.la plugins/libparser/libparser.la

deadbeef_CFLAGS = $(DEPS_CFLAGS) $(DISPATCH_CFLAGS) -std=c99 -DLOCALEDIR=\"@localedir@\"

docsdir = $(docdir)

docs_DATA = README help.txt about.txt translators.txt ChangeLog\
	COPYING.GPLv2 COPYING.LGPLv2.1

desktopdir = $(datadir)/applications
desktop_DATA = deadbeef.desktop

EXTRA_DIST = $(docs_DATA) $(desktop_DATA) $(INTLTOOL_FILES) translation/extra.c translation/plugins.c sj_to_unicode.h examples/decoder_template.c examples/dsp_template.c yasmwrapper.sh

ACLOCAL_AMFLAGS = -I m4
