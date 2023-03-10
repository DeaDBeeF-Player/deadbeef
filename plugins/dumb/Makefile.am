dumbpath=@top_srcdir@/plugins/dumb/dumb-kode54
EXTRA_DIST = $(dumbpath)/readme.txt $(dumbpath)/ChangeLog $(dumbpath)/licence.txt $(dumbpath)/release.txt $(dumbpath)/todo.txt

if HAVE_DUMB
pkglib_LTLIBRARIES = ddb_dumb.la

ddb_dumb_la_CFLAGS = $(CFLAGS) $(ALLOCA_H_CFLAGS) -I$(dumbpath)/include -I@top_srcdir@/include -std=gnu99
ddb_dumb_la_CXXFLAGS = $(CFLAGS) $(ALLOCA_H_CFLAGS) -I$(dumbpath)/include -I@top_srcdir@/include -fno-exceptions -fno-rtti -fno-unwind-tables
ddb_dumb_la_LDFLAGS = -module -avoid-version -lm $(NOCPPLIB)
if HAVE_SSE2
noinst_LTLIBRARIES = libdumbsse2.la
libdumbsse2_la_SOURCES = dumb-kode54/src/helpers/resampler_sse2.c
libdumbsse2_la_CFLAGS = $(CFLAGS) -I$(dumbpath)/include -std=gnu99 -msse2 -fPIC
ddb_dumb_la_LIBADD = libdumbsse2.la
endif


ddb_dumb_la_SOURCES =\
dumb-kode54/src/it/loadmod2.c\
dumb-kode54/src/it/itorder.c\
dumb-kode54/src/it/readokt2.c\
dumb-kode54/src/it/readptm.c\
dumb-kode54/src/it/loadxm.c\
dumb-kode54/src/it/ptmeffect.c\
dumb-kode54/src/it/loadany2.c\
dumb-kode54/src/it/loadokt2.c\
dumb-kode54/src/it/loadasy.c\
dumb-kode54/src/it/readasy.c\
dumb-kode54/src/it/loadpsm2.c\
dumb-kode54/src/it/readstm.c\
dumb-kode54/src/it/readxm2.c\
dumb-kode54/src/it/readmod.c\
dumb-kode54/src/it/readam.c\
dumb-kode54/src/it/loadoldpsm2.c\
dumb-kode54/src/it/loadxm2.c\
dumb-kode54/src/it/loadmod.c\
dumb-kode54/src/it/loadany.c\
dumb-kode54/src/it/loadmtm.c\
dumb-kode54/src/it/itrender.c\
dumb-kode54/src/it/loadasy2.c\
dumb-kode54/src/it/readpsm.c\
dumb-kode54/src/it/itload.c\
dumb-kode54/src/it/loadriff2.c\
dumb-kode54/src/it/itread.c\
dumb-kode54/src/it/loadmtm2.c\
dumb-kode54/src/it/loadriff.c\
dumb-kode54/src/it/readmtm.c\
dumb-kode54/src/it/reads3m.c\
dumb-kode54/src/it/itload2.c\
dumb-kode54/src/it/readxm.c\
dumb-kode54/src/it/loadpsm.c\
dumb-kode54/src/it/readany2.c\
dumb-kode54/src/it/loadamf.c\
dumb-kode54/src/it/loadptm2.c\
dumb-kode54/src/it/readokt.c\
dumb-kode54/src/it/itread2.c\
dumb-kode54/src/it/itmisc.c\
dumb-kode54/src/it/loadokt.c\
dumb-kode54/src/it/loads3m.c\
dumb-kode54/src/it/loadptm.c\
dumb-kode54/src/it/readdsmf.c\
dumb-kode54/src/it/readamf2.c\
dumb-kode54/src/it/itunload.c\
dumb-kode54/src/it/readmod2.c\
dumb-kode54/src/it/readamf.c\
dumb-kode54/src/it/readoldpsm.c\
dumb-kode54/src/it/loadamf2.c\
dumb-kode54/src/it/read6692.c\
dumb-kode54/src/it/read669.c\
dumb-kode54/src/it/readriff.c\
dumb-kode54/src/it/readany.c\
dumb-kode54/src/it/load669.c\
dumb-kode54/src/it/loadstm2.c\
dumb-kode54/src/it/loadstm.c\
dumb-kode54/src/it/load6692.c\
dumb-kode54/src/it/readstm2.c\
dumb-kode54/src/it/reads3m2.c\
dumb-kode54/src/it/loads3m2.c\
dumb-kode54/src/it/loadoldpsm.c\
dumb-kode54/src/it/xmeffect.c\
dumb-kode54/src/helpers/riff.c\
dumb-kode54/src/helpers/memfile.c\
dumb-kode54/src/helpers/silence.c\
dumb-kode54/src/helpers/stdfile.c\
dumb-kode54/src/helpers/clickrem.c\
dumb-kode54/src/helpers/tarray.c\
dumb-kode54/src/helpers/resample.c\
dumb-kode54/src/helpers/barray.c\
dumb-kode54/src/helpers/lpc.c\
dumb-kode54/src/helpers/resampler.c\
dumb-kode54/src/helpers/sampbuf.c\
dumb-kode54/src/core/unload.c\
dumb-kode54/src/core/loadduh.c\
dumb-kode54/src/core/duhlen.c\
dumb-kode54/src/core/register.c\
dumb-kode54/src/core/rawsig.c\
dumb-kode54/src/core/rendduh.c\
dumb-kode54/src/core/makeduh.c\
dumb-kode54/src/core/duhtag.c\
dumb-kode54/src/core/readduh.c\
dumb-kode54/src/core/dumbfile.c\
dumb-kode54/src/core/atexit.c\
dumb-kode54/src/core/rendsig.c\
dumb-kode54/src/tools/it/modulus.h\
dumb-kode54/include/internal/it.h\
dumb-kode54/include/internal/dumb.h\
dumb-kode54/include/internal/barray.h\
dumb-kode54/include/internal/tarray.h\
dumb-kode54/include/internal/resampler.h\
dumb-kode54/include/internal/riff.h\
dumb-kode54/include/internal/lpc.h\
dumb-kode54/include/internal/dumbfile.h\
dumb-kode54/include/internal/stack_alloc.h\
dumb-kode54/include/dumb.h\
dumb-kode54/src/helpers/resample.inc\
dumb-kode54/src/helpers/resamp2.inc\
dumb-kode54/src/helpers/resamp3.inc\
modloader.cpp\
modloader.h\
umr.h\
unmo3.h\
unrealfmt.cpp\
unrealfmtdata.cpp\
urf.h\
cdumb.c\
cdumb.h
endif
