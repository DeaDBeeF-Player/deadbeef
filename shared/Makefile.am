noinst_LIBRARIES = libmp4tagutil.a libtrkpropertiesutil.a libeqpreset.a libctmap.a libmp4parser.a

libmp4tagutil_a_SOURCES = mp4tagutil.h mp4tagutil.c
libmp4tagutil_a_CFLAGS = -DUSE_MP4FF -DUSE_TAGGING -fPIC -std=c99 -I@top_srcdir@/plugins/libmp4ff

libtrkpropertiesutil_a_SOURCES = trkproperties_shared.h trkproperties_shared.c
libtrkpropertiesutil_a_CFLAGS = -fPIC -std=c99

libeqpreset_a_SOURCES = eqpreset.h eqpreset.c
libeqpreset_a_CFLAGS = -fPIC -std=c99

libctmap_a_SOURCES = ctmap.h ctmap.c
libctmap_a_CFLAGS = -fPIC -std=c99

libmp4parser_a_SOURCES = mp4parser/mp4p.c mp4parser/mp4p.h mp4parser/mp4pfile.c mp4parser/mp4pfile.h
libmp4parser_a_CFLAGS = -fPIC -std=c99
