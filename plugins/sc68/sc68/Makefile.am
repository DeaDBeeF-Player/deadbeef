# ,----------------------------------------------------------------------.
# |                                                                      |
# |    sc68 -- Makefile.am -- Use with automake to produce Makefile.in   |
# |                                                                      |
# |               Copyright (C) 2007-2015 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

SUBDIRS              = $(subdirs)
ACLOCAL_AMFLAGS      = -I m4 --install
AM_CFLAGS            = $(ALL_CFLAGS) $(BIN_CFLAGS)
AM_CPPFLAGS          = $(sc68_inc) $(file68_inc)
AM_LDFLAGS           = $(ALL_LFLAGS) $(BIN_LFLAGS)

# ,----------------------------------------------------------------------.
# | Libtool program                                                      |
# `----------------------------------------------------------------------'

if SOURCE_SC68
SC68_LA = $(sc68_dir)/libsc68.la
@SET_MAKE@
$(sc68_dir)/libsc68.la:
	$(MAKE) -C "$(sc68_dir)" $(AM_MAKEFLAGS)
else
SC68_LA =
endif

if SOURCE_FILE68
FILE68_LA = $(file68_dir)/libfile68.la
@SET_MAKE@
$(file68_dir)/libfile68.la:
	$(MAKE) -C "$(file68_dir)" $(AM_MAKEFLAGS)
else
FILE68_LA =
endif

bin_PROGRAMS         = sc68
sc68_SOURCES         = sc68.c
sc68_LDADD           = $(SC68_LA) $(FILE68_LA)

# ,----------------------------------------------------------------------.
# | Documentation                                                        |
# `----------------------------------------------------------------------'

# dist_man1_MANS       = sc68.man
# info_TEXINFOS        = sc68.texi
# MAINTAINERCLEANFILES = $(srcdir)/sc68.1

# TEXINFO2MAN          = $(texinfo2man)
# fail                 = { test -e '$@' && rm '$@'; test a = b; }

# $(srcdir)/sc68.1: sc68.1.tpl.in $(srcdir)/sc68.texi
# 	$(TEXINFO2MAN) sc68.1.tpl $(srcdir)/sc68.texi >$@ || $(fail)
