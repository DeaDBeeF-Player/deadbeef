# The library.

lib_LTLIBRARIES = libresid-builder.la

libresid_builder_la_SOURCES = resid.cpp resid-builder.cpp resid-emu.h
libresid_builder_la_LIBADD  = $(LIBRESID_LDFLAGS)

# We substitute the libtool-specific library version in configure.in.
libresid_builder_la_LDFLAGS = -version-info $(LIBVERSION)

# Remove bad default includes
DEFAULT_INCLUDES=
