INCLUDES = \
	-I $(top_srcdir)/include

lib_LTLIBRARIES = libdumb.la

libdumb_la_SOURCES = \
	core/atexit.c \
	core/duhlen.c \
	core/duhtag.c \
	core/dumbfile.c \
	core/loadduh.c \
	core/makeduh.c \
	core/rawsig.c \
	core/readduh.c \
	core/register.c \
	core/rendduh.c \
	core/rendsig.c \
	core/unload.c \
	helpers/clickrem.c \
	helpers/memfile.c \
	helpers/resample.c \
	helpers/resample.inc \
	helpers/sampbuf.c \
	helpers/silence.c \
	helpers/stdfile.c \
	it/itload.c \
	it/itmisc.c \
	it/itorder.c \
	it/itread.c \
	it/itrender.c \
	it/itunload.c \
	it/loadmod.c \
	it/loads3m.c \
	it/loadxm.c \
	it/readmod.c \
	it/reads3m.c \
	it/readxm.c \
	it/xmeffect.c

libdumb_la_LDFLAGS = \
	-release $(VERSION)
