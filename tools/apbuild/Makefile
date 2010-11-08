.PHONY: all install uninstall

PREFIX:=$(shell test x"`id -u`" = "x0" && if test -f /usr/bin/apgcc; then echo /usr; else echo /usr/local; fi || echo ~/.local)
BINDIR:=$(PREFIX)/bin
PERLDIR:=$(PREFIX)/share/apbuild
INCLUDEDIR:=$(PREFIX)/include/apbuild
ACLOCALDIR:=$(PREFIX)/share/aclocal

PACKAGE=apbuild
# Don't forget to bump the version in apgcc too.
VERSION=2.0.9

PROGS:=apgcc apg++ relaytool scandeps make-icons

all:
	@echo No compilation is required. To install, type 'make install'.
	@echo "(Current prefix=$(PREFIX))"

install:
	mkdir -p $(BINDIR)
	cp $(PROGS) $(BINDIR)
	chmod +x $(BINDIR)/apgcc $(BINDIR)/apg++ $(BINDIR)/scandeps $(BINDIR)/make-icons $(BINDIR)/relaytool
	mkdir -p $(PERLDIR)/Apbuild
	cp Apbuild/*.pm $(PERLDIR)/Apbuild/
	mkdir -p $(INCLUDEDIR)
	cp ctype.h apsymbols.h $(INCLUDEDIR)
	mkdir -p $(ACLOCALDIR)
	cp relaytool.m4 $(ACLOCALDIR)
	echo >> $(INCLUDEDIR)/apsymbols.h
	echo "/* apbuild version" $(VERSION) "*/" >> $(INCLUDEDIR)/apsymbols.h
	@echo --------------
	@echo "Installation complete. Please read README for usage."

uninstall:
	rm -f $(addprefix $(BINDIR)/, $(PROGS))
	rm -f $(PERLDIR)/Apbuild/*.pm
	rmdir $(PERLDIR)/Apbuild $(PERLDIR)
	rm -f $(INCLUDEDIR)/ctype.h
	rm -f $(INCLUDEDIR)/apsymbols.h
	rm -f $(ACLOCALDIR)/relaytool.m4

distdir:
	rm -rf $(PACKAGE)-$(VERSION)
	mkdir $(PACKAGE)-$(VERSION)
	cp -R BINARY-PORTABILITY-NOTES Makefile apsymbols.h ctype.h README $(PROGS) buildlist relaytool.m4 $(PACKAGE)-$(VERSION)/
	mkdir $(PACKAGE)-$(VERSION)/test-app
	mkdir $(PACKAGE)-$(VERSION)/Apbuild
	cp Apbuild/*.pm $(PACKAGE)-$(VERSION)/Apbuild/
	cp test-app/randomapp1.c $(PACKAGE)-$(VERSION)/test-app/

dist: distdir
	rm -f $(PACKAGE)-$(VERSION).tar.gz
	tar -cf $(PACKAGE)-$(VERSION).tar $(PACKAGE)-$(VERSION)
	gzip --best $(PACKAGE)-$(VERSION).tar
	rm -rf $(PACKAGE)-$(VERSION)
