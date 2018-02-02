if OS_WINDOWS
noinst_LTLIBRARIES = libwin.la
libwin_la_SOURCES = mingw32_mmap.c mingw32_scandir.c mingw32_strcasestr.c mingw32_strndup.c rename.c mkstemp.c mingw32_layer.h
libwin_la_LIBADD = $(ICONV_LIB)
AM_CFLAGS = -fPIC
endif
