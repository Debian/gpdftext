
gpdficonsdir = ${datadir}/pixmaps/
gpdficons_DATA = gpdftext.png gpdftext.xpm

EXTRA_DIST = \
	$(gpdficons_DATA) \
	$(PACKAGE).1 \
	$(desktop_in_files) \
	$(PACKAGE).xml

DB2MAN = /usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl
XP = xsltproc -''-nonet -''-param man.charmap.use.subset "0"

$(PACKAGE).1: $(PACKAGE).xml
	$(XP) $(DB2MAN) $<

man_MANS=$(PACKAGE).1

desktopdir  = $(datadir)/applications
desktop_in_files = gpdftext.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES = \
	gpdftext.desktop \
	gpdftext.schemas

@INTLTOOL_DESKTOP_RULE@

install-data-local:
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)"; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA); \
	fi
endif

install-data-hook:
if HAVE_NEW_DESKTOP
	if test -z "$(DESTDIR)"; then \
		$(UPDATE_DESKTOP_DATABASE); \
	fi
endif

uninstall-hook:
if HAVE_NEW_DESKTOP
	if test -z "$(DESTDIR)"; then \
		$(UPDATE_DESKTOP_DATABASE); \
	fi
endif
