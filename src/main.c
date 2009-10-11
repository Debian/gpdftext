/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Neil Williams 2009 <linux@codehelp.co.uk>
 * 
 * gpdftext is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gpdftext is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <config.h>

#include <gtk/gtk.h>

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

#include "callbacks.h"

static GtkBuilder * builder;

void
about_show (void)
{
	GdkPixbuf *logo;
	static GtkWidget *dialog = NULL;
	static const gchar *authors[] =
	{
		"Neil Williams  <linux@codehelp.co.uk>",
		"",
		NULL
	};
	static const gchar *documentors[] = 
	{
		"Neil Williams  <linux@codehelp.co.uk>",
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
	gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (dialog), _("eBook PDF editor"));
	gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dialog),
		"Copyright 2009 Neil Williams");
	gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (dialog),
		_("gPDFText - edit ebook PDF files as ASCII text.\n"));
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dialog), authors);
	gtk_about_dialog_set_documenters (GTK_ABOUT_DIALOG (dialog), documentors);
	gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG (dialog), translators);
	gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (dialog), logo);
	gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (dialog), "http://gpdftext.sourceforge.net/");
	gtk_about_dialog_set_website_label (GTK_ABOUT_DIALOG (dialog), _("Homepage"));
	gtk_window_set_icon (GTK_WINDOW(dialog), logo);
	gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (dialog),
		" gpdftext is free software: you can redistribute it and/or modify it\n"
		" under the terms of the GNU General Public License as published by the\n"
		" Free Software Foundation, either version 3 of the License, or\n"
		" (at your option) any later version.\n"
		"\n"
		" gpdftext is distributed in the hope that it will be useful, but\n"
		" WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
		" See the GNU General Public License for more details.\n"
		"\n"
		" You should have received a copy of the GNU General Public License along\n"
		" with this program.  If not, see <http://www.gnu.org/licenses/>.\n");

//	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (dc->current_window));
	
	g_object_add_weak_pointer (G_OBJECT (dialog), (void **)&dialog);
	
	gtk_dialog_run (GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);
	
	g_free (translators);
	
	return;
}

extern void
set_text (gchar * text)
{
	GtkTextView * textview;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	gssize size;
	GRegex * line, * page, *hyphen;
	GError * err;

	err = NULL;
	size = strlen (text);
	/* FIXME: don't compile these per page, once per doc instead. */
	line = g_regex_new ("\n(.[^\\s])", 0, 0, &err);
	if (err)
		g_warning ("new line: %s", err->message);
	page = g_regex_new ("\n\\s+\\d+\\s?\n", 0, 0, &err);
	if (err)
		g_warning ("new para: %s", err->message);
	hyphen = g_regex_new ("\\w- ", 0, 0, &err);
	if (err)
		g_warning ("new hyphen: %s", err->message);
	text = g_regex_replace (line, text, -1, 0, " \\1",0 , &err);
	if (err)
		g_warning ("line replace: %s", err->message);
	text = g_regex_replace_literal (page, text, -1, 0, " ",0 , &err);
	if (err)
		g_warning ("page replace: %s", err->message);
	text = g_regex_replace_literal (hyphen, text, -1, 0, " ",0 , &err);
	if (err)
		g_warning ("hyphen replace: %s", err->message);
	g_regex_unref (line);
	g_regex_unref (page);
	if (!g_utf8_validate (text, -1, NULL))
	{
		g_warning ("validate %s", text);
		return;
	}
	if (!text)
		return;
	size = strlen (text);
	text = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
	textview = GTK_TEXT_VIEW(gtk_builder_get_object (builder, "textview"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (builder, "textbuffer1"));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_insert (buffer, &end, text, size);
	gtk_widget_show (GTK_WIDGET(textview));
}

void
new_pdf_cb (GtkImageMenuItem *self, gpointer user_data)
{
	GtkTextBuffer * buffer;

	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (builder, "textbuffer1"));
	gtk_text_buffer_set_text (buffer, "", 0);
}

GtkWidget*
create_window (void)
{
	GtkWidget *window, *open, *save, *cancel, *about, *newbtn;
	GtkWidget *newmenu, *openmenu, *quitmenu, *savemenu, *saveasmenu, *aboutmenu;
	GtkTextBuffer * buffer;
	GtkActionGroup  *action_group;
	GtkUIManager * uimanager;
	GdkPixbuf *logo;
	gchar *path;

	builder = load_builder_xml (NULL);
	if (!builder)
		return NULL;
	path = g_build_filename (DATADIR, "pixmaps", "gpdftext.png", NULL);
	logo = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);
	window = GTK_WIDGET(gtk_builder_get_object (builder, "gpdfwindow"));
	gtk_window_set_icon (GTK_WINDOW(window), logo);
	newbtn = GTK_WIDGET(gtk_builder_get_object (builder, "new_button"));
	open = GTK_WIDGET(gtk_builder_get_object (builder, "open_button"));
	save = GTK_WIDGET(gtk_builder_get_object (builder, "save_button"));
	cancel = GTK_WIDGET(gtk_builder_get_object (builder, "quit_button"));
	about = GTK_WIDGET(gtk_builder_get_object (builder, "about_button"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (builder, "textbuffer1"));
	action_group = GTK_ACTION_GROUP(gtk_builder_get_object (builder, "actiongroup"));
	uimanager = GTK_UI_MANAGER(gtk_builder_get_object (builder, "uimanager"));
	aboutmenu = GTK_WIDGET(gtk_builder_get_object (builder, "aboutmenuitem"));
	openmenu = GTK_WIDGET(gtk_builder_get_object (builder, "openmenuitem"));
	newmenu = GTK_WIDGET(gtk_builder_get_object (builder, "newmenuitem"));
	quitmenu = GTK_WIDGET(gtk_builder_get_object (builder, "quitmenuitem"));
	savemenu = GTK_WIDGET(gtk_builder_get_object (builder, "savemenuitem"));
	saveasmenu = GTK_WIDGET(gtk_builder_get_object (builder, "saveasmenuitem"));
	gtk_widget_set_sensitive (saveasmenu, FALSE);
	gtk_text_buffer_set_text (buffer, "", 0);
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_ui_manager_insert_action_group (uimanager, action_group, 0);
	g_signal_connect (G_OBJECT (newbtn), "clicked", 
			G_CALLBACK (new_pdf_cb), window);
	g_signal_connect (G_OBJECT (open), "clicked", 
			G_CALLBACK (open_pdf_cb), window);
	g_signal_connect (G_OBJECT (save), "clicked", 
			G_CALLBACK (save_txt_cb), window);
	g_signal_connect (G_OBJECT (cancel), "clicked", 
			G_CALLBACK (destroy), window);
	g_signal_connect (G_OBJECT (about), "clicked", 
			G_CALLBACK (about_show), window);
	g_signal_connect (G_OBJECT (window), "delete_event",
			G_CALLBACK (gtk_main_quit), window);
	g_signal_connect (G_OBJECT (aboutmenu), "activate",
			G_CALLBACK (about_show), window);
	g_signal_connect (G_OBJECT (openmenu), "activate",
			G_CALLBACK (open_pdf_cb), window);
	g_signal_connect (G_OBJECT (newmenu), "activate",
			G_CALLBACK (new_pdf_cb), window);
	g_signal_connect (G_OBJECT (quitmenu), "activate",
			G_CALLBACK (gtk_main_quit), window);
	g_signal_connect (G_OBJECT (savemenu), "activate",
			G_CALLBACK (save_txt_cb), window);
	
	return window;
}

int
main (int argc, char *argv[])
{
	GtkWidget *window;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	window = create_window ();
	if (!window)
		return -1;
	gtk_widget_show (window);

	gtk_main ();
	return 0;
}

