## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

gpdficonsdir = ${datadir}/pixmaps/
gpdficons_DATA = gpdftext.png gpdftext.xpm

gpdftextdocdir = ${prefix}/doc/gpdftext
gpdftextdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

EXTRA_DIST = $(gpdftextdoc_DATA) \
	$(gpdficons_DATA) \
	po/$(PACKAGE).pot

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache

all-local: Makefile
	make -C po $(PACKAGE).pot

clean-local: Makefile
	find . -name '*~' -delete

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(gpdftextdocdir)
