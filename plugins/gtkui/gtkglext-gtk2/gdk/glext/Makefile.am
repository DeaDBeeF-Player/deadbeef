## -*- Makefile -*-
## Makefile.am for gtkglext/gdk/glext

EXTRA_DIST = \
	README			\
	list-ext.pl		\
	gen-gdkglglext-c.pl	\
	gen-gdkglglext-h.pl	\
	gen-gdkglglxext-c.pl	\
	gen-gdkglglxext-h.pl	\
	gen-gdkglwglext-c.pl	\
	gen-gdkglwglext-h.pl	\
	common-header.h

gdkglext_glext_public_h_sources = \
	glext.h		\
	glxext.h	\
	wglext.h	\
	glext-extra.h	\
	glxext-extra.h	\
	wglext-extra.h

gdkglextglextincludedir = $(includedir)/gtkglext-@GTKGLEXT_API_VERSION@/gdk/glext
gdkglextglextinclude_HEADERS = $(gdkglext_glext_public_h_sources)

