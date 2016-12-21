# ,----------------------------------------------------------------------.
# |                                                                      |
# |   winui -- Makefile.am -- Use with automake to produce Makefile.in   | 
# |                                                                      |
# |               Copyright (C) 2007-2015 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

ACLOCAL_AMFLAGS = -I m4 --install
AM_CFLAGS       = $(ALL_CFLAGS) $(LIB_CFLAGS) -mwin32
AM_CPPFLAGS     = -I$(top_srcdir) -DFORCE_RSC=1

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

# Version with glued resource
noinst_LTLIBRARIES        = libwinui.la
libwinui_la_SOURCES       = winui_dlg.c resource.h
libwinui_la_DEPENDENCIES  = do-glued
libwinui_la_LIBADD        = winui_glu.lo
EXTRA_libwinui_la_SOURCES = resource.rc winui_rsc.c winui_dll.c
libwinui_la_LDFLAGS       = -avoid-version

do-glued: winui_glu.lo

# *** EVIL DIRTY HACK ***
#
# To create winui_glu.lo (and associated .o) we used libtool as it
# almost work. It runs the proper link command (-r) but it does not
# create the .lo file. The hack just transform winui_rsc.lo into a
# suitable winui_glu.lo
#
winui_glu.lo: winui_rsc.lo resource.lo
	$(LIBTOOL) $(AM_V_lt) --tag=CC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) \
	--mode=link $(CCLD) -o $@ $^
	test -f winui_glu.o && cp winui_glu.o .libs/winui_glu.o
	sed 's/winui_rsc/winui_glu/g' $< >$@

# Does not have a default automake rule for that
.rc.lo:
	$(LIBTOOL) $(AM_V_lt) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) \
	--mode=compile $(RC) -Jrc -Ocoff -o $@ -i $<

.res.lo:
	$(LIBTOOL) $(AM_V_lt) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) \
	--mode=compile $(RC) -Jres -Ocoff -o $@ -i $<

.PHONY: do-glued

## Version without glued resource
# noinst_LTLIBRARIES        = libwinui.la
# libwinui_la_SOURCES       = winui_dlg.c resource.h winui_rsc.c
# EXTRA_libwinui_la_SOURCES = resource.rc winui_dll.c
# libwinui_la_LDFLAGS       = -avoid-version
