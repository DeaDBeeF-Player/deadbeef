# 
# Modify the definition of $omf_dest_dir, replacing "gdp-example1" with
# the name of your package.
# 
# No other modifications should be necessary.
#
# Note that you must configure your package with --localstatedir=/var/lib
# so that the scrollkeeper-update command below will update the database
# in the standard scrollkeeper directory.
#
# If it is impossible to configure with --localstatedir=/var/lib, then
# modify the definition of scrollkeeper_localstate_dir so that
# it points to the correct location. Note that you must still use 
# $(localstatedir) in this or when people build RPMs it will update
# the real database on their system instead of the one under RPM_BUILD_ROOT.

omf_dest_dir=$(datadir)/omf/glade-2
scrollkeeper_localstate_dir = $(localstatedir)/scrollkeeper

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(omf_dest_dir)
	-for file in $(srcdir)/*.omf; do \
		$(INSTALL_DATA) $(srcdir)/$$file $(DESTDIR)$(omf_dest_dir); \
	done
	-scrollkeeper-update -p $(scrollkeeper_localstate_dir) 

uninstall-local:
	-for file in $(srcdir)/*.omf; do \
		basefile=`basename $$file`; \
		rm -f $(omf_dest_dir)/$$basefile; \
        done
	-rmdir $(omf_dest_dir)
	-scrollkeeper-update -p $(scrollkeeper_localstate_dir) 
