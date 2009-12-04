include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook
DOC_MODULE = gpdftext
DOC_ENTITIES = legal.xml
DOC_INCLUDES =
DOC_FIGURES = 

gpdftext-help.pot: Makefile
	xml2po -e -o $(PACKAGE)-help.pot $(top_srcdir)/help/C/${PACKAGE}.xml

update-po: Makefile
	for l in $(DOC_LINGUAS); do \
	xml2po -u $(top_srcdir)/help/$$l/$$l.po $(top_srcdir)/help/C/$(PACKAGE).xml; \
	done

report-po: Makefile
	if [ ! -d po ]; then \
		mkdir po/ ; \
	fi
	cp $(PACKAGE)-help.pot po
	cd po ; \
	for l in $(DOC_LINGUAS); do \
		if [ ! -L $$l.po ]; then \
			ln -s ../$$l/$$l.po . ; \
		fi; \
		echo -n $$l: ; \
		msgfmt -c --statistics $$l.po; \
		rm -f messages.mo; \
	done

EXTRA_DIST = gpdftext-help.pot ChangeLog

DISTCLEANFILES = gpdftext-help.pot

#figures/main_window.png  figures/open_document.png

DOC_LINGUAS = cs de es pt ru sv

