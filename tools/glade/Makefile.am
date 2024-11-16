## Process this file with automake to produce Makefile.in

SUBDIRS = glade omf-install

Developmentdir = $(datadir)/applications
Development_in_files = glade-2.desktop.in
Development_DATA = $(Development_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	FAQ \
	glade.spec \
	glade.spec.in \
	glade-2.desktop.in \
	glade-2.png \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	examples/editor/AUTHORS \
	examples/editor/ChangeLog \
	examples/editor/Makefile.am \
	examples/editor/NEWS \
	examples/editor/README \
	examples/editor/acconfig.h \
	examples/editor/autogen.sh \
	examples/editor/configure.in \
	examples/editor/editor.glade \
	examples/editor/editor.gladep \
	examples/editor/src/Makefile.am \
	examples/editor/src/callbacks.c \
	examples/editor/src/callbacks.h \
	examples/editor/src/interface.c \
	examples/editor/src/interface.h \
	examples/editor/src/main.c \
	examples/editor/src/support.c \
	examples/editor/src/support.h

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = glade-2.png

# Install all the Gnome m4 macros we use to build Glade, so we can copy them
# to each project's directory (for Gnome projects).
install-data-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps/glade-2
	$(INSTALL_DATA) $(srcdir)/glade/graphics/glade_logo.png $(DESTDIR)$(datadir)/pixmaps/glade-2

uninstall-local:
	@rm -f $(DESTDIR)$(datadir)/pixmaps/glade-2/glade_logo.png

#distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper'
distuninstallcheck:
	@:

distcleancheck_listfiles = find . -type f -print | grep -v '^\./doc/.*\.xml'

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

DISTCLEANFILES = 		\
	intltool-extract 	\
	intltool-merge 		\
	intltool-update		\
	$(Development_DATA)	\
	$(NULL)
