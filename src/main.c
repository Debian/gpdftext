/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Neil Williams 2009 <linux@codehelp.co.uk>
 * 
 * gpdftext is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * gpdftext is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <gconf/gconf.h>
#include <gtk/gtk.h>

#include <config.h>


/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#include "ebookui.h"

void
about_show (void)
{
	GdkPixbuf *logo;
	static GtkWidget *dialog = NULL;
	static const gchar *documentors[] = 
	{
		"Neil Williams  <linux@codehelp.co.uk>",
		NULL
	};
	static const gchar *authors[] =
	{
		"Neil Williams  <linux@codehelp.co.uk>",
		"",
		NULL
	};
	
	/*
	* Translators should localize the following string
	* which will give them credit in the About box.
	* E.g. "Fulano de Tal <fulano@detal.com>" */
	gchar *path, *translators = g_strdup (_("translator-credits"));

	if (dialog)
	{
		gtk_window_present (GTK_WINDOW (dialog));
		return;
	}

	path = g_build_filename (DATADIR, "pixmaps", "gpdftext.png", NULL);
	logo = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);
	dialog = gtk_about_dialog_new ();
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dialog), VERSION);
	gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (dialog),_("eBook PDF editor"));
	gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dialog),
		"Copyright 2009 Neil Williams");
	gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (dialog),
		_("gPDFText converts ebook PDF content into ASCII text, "
		  "reformatted for long line paragraphs."));
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dialog), authors);
	gtk_about_dialog_set_documenters (GTK_ABOUT_DIALOG (dialog), documentors);
	gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG (dialog), translators);
	gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (dialog), logo);
	gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (dialog), "http://gpdftext.sourceforge.net/");
	gtk_about_dialog_set_website_label (GTK_ABOUT_DIALOG (dialog), _("Homepage"));
	gtk_window_set_icon (GTK_WINDOW(dialog), logo);
	gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (dialog),
		" gpdftext is free software: you can redistribute it and/or modify it\n"
		" under the terms of the GNU General Public License\n"
		" version 2 as published by the Free Software Foundation.\n"
		"\n"
		" gpdftext is distributed in the hope that it will be useful, but\n"
		" WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
		" See the GNU General Public License for more details.\n"
		"\n"
		" You should have received a copy of the GNU General Public License\n"
		" along with this program; if not, write to the Free Software\n"
		" Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,\n"
		" MA 02110-1301, USA.\n"
		);

	g_object_add_weak_pointer (G_OBJECT (dialog), (void **)&dialog);
	gtk_dialog_run (GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);
	g_free (translators);
}

int
main (int argc, char *argv[])
{
	GError * error;
	GFile * gfile;
	gint num_args;
	GtkWidget *window;
	gchar **remaining_args;
	GOptionContext *option_context;
	Ebook * ebook;

	GOptionEntry option_entries[] = {
		/* option that collects filenames */
		{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
			&remaining_args, NULL, _("file")},
		{NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL}
	};

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	remaining_args = NULL;
	gfile = NULL;
	num_args = 0;
	error = NULL;
	g_type_init ();
	option_context = g_option_context_new ("");
	g_option_context_add_main_entries (option_context,
		option_entries, GETTEXT_PACKAGE);
	g_option_context_parse (option_context, &argc, &argv, NULL);

	/* GConf setup */
	if (gconf_init (argc, argv, &error) == FALSE)
	{
		g_assert (error != NULL);
		g_message (_("GConf init failed: %s"), error->message);
		g_error_free (error);
		return 1;
	}
	
	/* create a context struct here and pass it around. */
	ebook = new_ebook ();

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	window = create_window (ebook);
	if (!window)
		return 2;
	if (remaining_args != NULL)
		num_args = g_strv_length (remaining_args);
	if (num_args >= 1)
	{
		gchar * filename;
		filename = g_strdup (remaining_args[num_args - 1]);
		open_file (ebook, filename);
	}
	gtk_widget_show (window);
	gtk_main ();
	return 0;
}
