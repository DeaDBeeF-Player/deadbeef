SUBDIRS = analyzer scope scriptable

noinst_LTLIBRARIES = libmp4tagutil.la libtrkpropertiesutil.la libeqpreset.la libctmap.la libdeletefromdisk.la libtftintutil.la

libmp4tagutil_la_SOURCES = mp4tagutil.h mp4tagutil.c
libmp4tagutil_la_CFLAGS = -fPIC -std=c99 -I@top_srcdir@/external/mp4p/include -I@top_srcdir@/include

libtrkpropertiesutil_la_SOURCES = trkproperties_shared.h trkproperties_shared.c
libtrkpropertiesutil_la_CFLAGS = -fPIC -std=c99 -I@top_srcdir@/include

libeqpreset_la_SOURCES = eqpreset.h eqpreset.c
libeqpreset_la_CFLAGS = -fPIC -std=c99 -I@top_srcdir@/include

libctmap_la_SOURCES = ctmap.h ctmap.c
libctmap_la_CFLAGS = -fPIC -std=c99 -I@top_srcdir@/include

libdeletefromdisk_la_SOURCES = deletefromdisk.h deletefromdisk.c
libdeletefromdisk_la_CFLAGS = -fPIC -std=c99 -I@top_srcdir@/include

libtftintutil_la_SOURCES = tftintutil.h tftintutil.c
libtftintutil_la_CFLAGS = -fPIC -std=c99 -I@top_srcdir@/include
