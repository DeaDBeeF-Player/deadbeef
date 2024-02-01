bin_PROGRAMS = deadbeef

deadbeef_SOURCES =\
	buffered_file_writer.c buffered_file_writer.h\
	conf.c  conf.h\
	cueutil.c cueutil.h playlist.c playlist.h \
	decodedblock.c decodedblock.h\
	dsp.c dsp.h\
	dsppreset.c dsppreset.h\
	escape.c escape.h\
	../external/wcwidth/wcwidth.c ../external/wcwidth/wcwidth.h\
	fft.c fft.h\
	handler.c handler.h\
	junklib.h junklib.c utf8.c utf8.h\
	logger.c logger.h\
	main.c\
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
	streamer.c streamer.h\
	streamreader.c streamreader.h\
	tf.c tf.h\
	threading_pthread.c threading.h\
	u8_lc_map.h\
	u8_uc_map.h\
	vfs.c vfs.h vfs_stdio.c\
	volume.c volume.h\
	viz.c viz.h\
	undo/undo_playlist.c undo/undo_playlist.h

deadbeef_LDADD = $(LDADD) $(ICONV_LIB) $(DL_LIBS) -lm -lpthread $(DISPATCH_LIBS) $(LTLIBINTL) ../shared/libctmap.la ../shared/undo/libundo.la ../plugins/libparser/libparser.la

deadbeef_CFLAGS = $(DEPS_CFLAGS) $(DISPATCH_CFLAGS) -std=c99 -DLOCALEDIR=\"@localedir@\" -I@top_srcdir@/include -I@top_srcdir@/shared -I@top_srcdir@

EXTRA_DIST = sj_to_unicode.h 
