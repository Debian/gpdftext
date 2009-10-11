## Process this file with automake to produce Makefile.in

## Created by Anjuta

gladedir = $(datadir)/gpdftext/glade
glade_DATA = gpdftext.glade

AM_CPPFLAGS = \
	-DPACKAGE_GLADE_FILE=\"$(gladedir)/gpdftext.glade\" \
	-DDATADIR=\"$(datadir)\" \
	$(GPDFTEXT_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = gpdftext

gpdftext_SOURCES = \
	callbacks.c \
	callbacks.h \
	main.c

gpdftext_LDFLAGS = \
	-Wl,--export-dynamic

gpdftext_LDADD = $(GPDFTEXT_LIBS)

EXTRA_DIST = $(glade_DATA)

# Remove glade directory on uninstall
uninstall-local:
	-rm -r $(gladedir)
	-rm -r $(datadir)/gpdftext
