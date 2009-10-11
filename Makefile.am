## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

gpdficonsdir = ${datadir}/pixmaps/
gpdficons_DATA = gpdftext.png gpdftext.xpm

EXTRA_DIST = $(gpdftextdoc_DATA) \
	$(gpdficons_DATA) \
	$(PACKAGE).1 \
	manpage.xml \
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

DB2MAN = /usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl
XP = xsltproc -''-nonet -''-param man.charmap.use.subset "0"

$(PACKAGE).1: manpage.xml
	$(XP) $(DB2MAN) $<

man_MANS=$(PACKAGE).1

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(gpdftextdocdir)
