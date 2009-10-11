## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

gpdficonsdir = ${datadir}/pixmaps/
gpdficons_DATA = gpdftext.png gpdftext.xpm

EXTRA_DIST = $(gpdftextdoc_DATA) \
	$(gpdficons_DATA) \
	COPYING \
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

debiantarball: Makefile dist
	ln $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)_$(VERSION).orig.tar.gz;

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(gpdftextdocdir)
