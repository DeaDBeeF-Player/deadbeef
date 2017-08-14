if OS_WINDOWS
noinst_LTLIBRARIES = libwin.la
libwin_la_SOURCES =  mingw32_scandir.c mingw32_strcasestr.c mingw32_strndup.c rename.c mingw32_layer.h
AM_CFLAGS = $(CFLAGS)-fPIC -I/usr/x86_64-w64-mingw32/include
endif
