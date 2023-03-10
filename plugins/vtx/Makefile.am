if HAVE_VTX
pkglib_LTLIBRARIES = vtx.la
vtx_la_SOURCES = vtx.c ay8912.c ayemu_8912.h ayemu.h ayemu_vtxfile.h lh5dec.c vtxfile.c
vtx_la_LDFLAGS = -module -avoid-version

vtx_la_CFLAGS = $(CFLAGS) -std=c99 -I@top_srcdir@/include
endif
