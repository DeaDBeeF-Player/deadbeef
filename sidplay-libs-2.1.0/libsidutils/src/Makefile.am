
SUBDIRS = MD5 ini

# The library.

lib_LTLIBRARIES = libsidutils.la

libsidutils_la_SOURCES = SidDatabase.cpp SidFilter.cpp SidTuneMod.cpp SidUsage.cpp smm0.h

libsidutils_la_LIBADD  = ./MD5/libMD5.la ./ini/libini.la \
$(LIBSIDPLAY2_LDFLAGS)

# We substitute the libtool-specific library version in configure.in.
libsidutils_la_LDFLAGS = -version-info $(LIBVERSION)

# Remove bad default includes
DEFAULT_INCLUDES=
