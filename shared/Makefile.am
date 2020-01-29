noinst_LIBRARIES = libmp4tagutil.a libtrkpropertiesutil.a libeqpreset.a

libmp4tagutil_a_SOURCES = mp4tagutil.h mp4tagutil.c
libmp4tagutil_a_CFLAGS = -DUSE_MP4FF -DUSE_TAGGING -fPIC -std=c99 -I@top_srcdir@/plugins/libmp4ff

libtrkpropertiesutil_a_SOURCES = trkproperties_shared.h trkproperties_shared.c
libtrkpropertiesutil_a_CFLAGS = -fPIC -std=c99

libeqpreset_a_SOURCES = eqpreset.h eqpreset.c
libeqpreset_a_CFLAGS = -fPIC -std=c99
