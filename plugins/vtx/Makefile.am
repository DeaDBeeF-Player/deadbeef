if HAVE_VTX
pkglib_LTLIBRARIES = vtx.la
vtx_la_SOURCES =\
	vtx.c\
	libayemu/ay8912.c\
	libayemu/ayemu_8912.h\
	libayemu/ayemu.h\
	libayemu/ayemu_vtxfile.h\
	libayemu/lh5dec.c\
	libayemu/vtxfile.c

vtx_la_LDFLAGS = -module -avoid-version

vtx_la_CFLAGS = $(CFLAGS) -std=c99 -I@top_srcdir@/include -Ilibayemu
endif
