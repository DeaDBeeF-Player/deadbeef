if HAVE_SUPEREQ
pkglib_LTLIBRARIES = supereq.la
supereq_la_SOURCES =\
	supereq.c\
	libsupereq/Equ.cpp\
	libsupereq/Equ.h\
	libsupereq/Fftsg_fl.c\
	libsupereq/paramlist.hpp

supereq_la_CFLAGS = $(CFLAGS) -std=c99 -DUSE_OOURA -I@top_srcdir@/include -Ilibsupereq
supereq_la_CXXFLAGS = $(CXXFLAGS) -fno-exceptions -fno-rtti -fno-unwind-tables -DUSE_OOURA -I@top_srcdir@/include -Ilibsupereq

supereq_la_LDFLAGS = -module -avoid-version $(NOCPPLIB)

supereq_la_LIBADD = $(LDADD)
endif
