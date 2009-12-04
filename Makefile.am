## Process this file with automake to produce Makefile.in

SUBDIRS = src po data help

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = $(gpdftextdoc_DATA) \
	$(desktop_in_files) \
	COPYING \
	COPYING-DOCS \
	po/$(PACKAGE).pot
	README\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache

all-local: Makefile
	make -C po $(PACKAGE).pot

clean-local: Makefile
	find . -name '*~' -delete
	rm -f intltool-*.in

debiantarball: Makefile dist
	ln $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)_$(VERSION).orig.tar.gz;

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(gpdftextdocdir)

DISTCHECK_CONFIGURE_FLAGS = \
 --disable-schemas-install  \
 --disable-desktop-update   \
 --disable-scrollkeeper
