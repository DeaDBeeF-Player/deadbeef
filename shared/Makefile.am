noinst_LTLIBRARIES = libmp4tagutil.la libtrkpropertiesutil.la

libmp4tagutil_la_SOURCES = mp4tagutil.h mp4tagutil.c
libmp4tagutil_la_CFLAGS = -DUSE_MP4FF -DUSE_TAGGING -fPIC -std=c99 -I@top_srcdir@/plugins/libmp4ff

libtrkpropertiesutil_la_SOURCES = trkproperties_shared.h trkproperties_shared.c
libtrkpropertiesutil_la_CFLAGS = -fPIC -std=c99

