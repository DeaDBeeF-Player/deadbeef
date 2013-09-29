EXTRA_DIST = \
	autogen.sh

# Install the autogen.sh script for GTK+ apps.
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)-2/gtk
	$(INSTALL_DATA) $(srcdir)/autogen.sh $(DESTDIR)$(pkgdatadir)-2/gtk

uninstall-local:
	@rm -f $(DESTDIR)$(pkgdatadir)-2/gtk/autogen.sh

