include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook
DOC_MODULE = gpdftext
DOC_ENTITIES = legal.xml
DOC_INCLUDES =
DOC_FIGURES = 

gpdftext-help.pot: Makefile
	xml2po -e -o $(PACKAGE)-help.pot $(top_srcdir)/help/C/${PACKAGE}.xml

EXTRA_DIST = gpdftext-help.pot ChangeLog

DISTCLEANFILES = gpdftext-help.pot

#figures/main_window.png  figures/open_document.png

DOC_LINGUAS = de pt ru

