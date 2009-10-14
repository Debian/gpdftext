/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) Neil Williams 2009 <linux@codehelp.co.uk>
 * 
 * callbacks.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * callbacks.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* main GTK+ interface functions */

#include <config.h>

#include "callbacks.h"
#include <glib/gi18n.h>
#include <glib/poppler-document.h>
#include <glib/poppler-page.h>

/** these need to be in a context struct */
static PopplerDocument * PDFDoc;
static GtkBuilder * builder = NULL;
static gchar * filename = NULL;

GtkBuilder*
load_builder_xml (const gchar *root)
{
	gchar *path;
	GError* error = NULL;

	if (builder)
		return builder;

	builder = gtk_builder_new ();

	if (g_file_test (PACKAGE_GLADE_FILE, G_FILE_TEST_EXISTS))
	{
		if (!gtk_builder_add_from_file (builder, PACKAGE_GLADE_FILE, &error))
		{
			g_warning ("Couldn't load builder file: %s", error->message);
			g_error_free (error);
		}
	}
	else
	{
		path = g_build_filename ("src", PACKAGE_GLADE_FILE, NULL);
		if (g_file_test (path, G_FILE_TEST_EXISTS))
		{
			if (!gtk_builder_add_from_file (builder, PACKAGE_GLADE_FILE, &error))
			{
				g_warning ("Couldn't load builder file: %s", error->message);
				g_error_free (error);
			}
		}
		g_free (path);
	}
	
	return builder;
}

void
destroy (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

static void
save_file (void)
{
	GtkProgressBar * progressbar;
	GtkStatusbar * statusbar;
	GtkTextView * textview;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	gchar * text;
	GError * err;
	guint id;

	builder = load_builder_xml (NULL);
	if (!builder)
		return;
	if (!filename)
		return;
	err = NULL;
	progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (builder, "progressbar"));
	gtk_progress_bar_set_fraction (progressbar, 0.0);
	statusbar = GTK_STATUSBAR(gtk_builder_get_object (builder, "statusbar"));
	id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
	gtk_statusbar_push (statusbar, id, _("Saving text file"));
	textview = GTK_TEXT_VIEW(gtk_builder_get_object (builder, "textview"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (builder, "textbuffer1"));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
	g_file_set_contents (filename, text, -1, &err);
	id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
	gtk_statusbar_push (statusbar, id, _("Saved text file."));
}

void
save_txt_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog, *window;
	GtkFileFilter *filter;

	/* for a save_as_txt_cb, just omit this check */
	if (filename)
	{
		save_file ();
		return;
	}
	window = (GtkWidget *)data;
	dialog = gtk_file_chooser_dialog_new (_("Save as text file"),
										  GTK_WINDOW (window),
										  GTK_FILE_CHOOSER_ACTION_SAVE,
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
										  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All text files"));
	gtk_file_filter_add_mime_type (filter, "text/plain");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GError * err;

		err = NULL;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		/* should set the text filename in the window title here? */
		gtk_widget_destroy (dialog);
		save_file ();
	}
	else
		gtk_widget_destroy (dialog);
}

extern void
set_text (gchar * text)
{
	GtkBuilder * builder;
	GtkTextView * textview;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	gssize size;
	GRegex * line, * page, *hyphen;
	GError * err;

	err = NULL;
	size = strlen (text);
	/* FIXME: don't compile these per page, once per doc instead. */
	/* FIXME: also make the header/footer removal configurable. */
	line = g_regex_new ("\n(.[^\\s])", 0, 0, &err);
	if (err)
		g_warning ("new line: %s", err->message);
	page = g_regex_new ("\n\\s+\\d+\\s?\n", 0, 0, &err);
	if (err)
		g_warning ("new para: %s", err->message);
	hyphen = g_regex_new ("(\\w)-[\\s\n]+", 0, 0, &err);
	if (err)
		g_warning ("new hyphen: %s", err->message);

	text = g_regex_replace (line, text, -1, 0, " \\1",0 , &err);
	if (err)
		g_warning ("line replace: %s", err->message);
	text = g_regex_replace_literal (page, text, -1, 0, " ",0 , &err);
	if (err)
		g_warning ("page replace: %s", err->message);
	text = g_regex_replace (hyphen, text, -1, 0, "\\1",0 , &err);
	if (err)
		g_warning ("hyphen replace: %s", err->message);
	g_regex_unref (line);
	g_regex_unref (page);
	if (!g_utf8_validate (text, -1, NULL))
	{
		/** FIXME: this should be a user-level warning. 
		 Needs a dialog. */
		g_warning ("validate %s", text);
		return;
	}
	if (!text)
		return;
	builder = load_builder_xml (NULL);
	if (!builder)
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
	guint id;
	GtkTextBuffer * buffer;
	GtkBuilder * builder;
	GtkProgressBar * progressbar;
	GtkStatusbar * statusbar;
	GtkWindow * window;

	builder = load_builder_xml (NULL);
	if (!builder)
		return;
	if (filename)
	{
		g_free (filename);
		filename = NULL;
	}
	window = GTK_WINDOW(gtk_builder_get_object (builder, "gpdfwindow"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (builder, "textbuffer1"));
	gtk_text_buffer_set_text (buffer, "", 0);
	progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (builder, "progressbar"));
	gtk_progress_bar_set_fraction (progressbar, 0.0);
	statusbar = GTK_STATUSBAR(gtk_builder_get_object (builder, "statusbar"));
	id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
	gtk_statusbar_push (statusbar, id, _("new file"));
	gtk_window_set_title (window, _("eBook PDF editor"));
}

/* FIXME: accept a context struct and attach it to signals
 instead of window (or attach a pointer to the struct to the window). */
GtkWidget*
create_window (void)
{
	GtkWidget *window, *open, *save, *cancel, *about, *newbtn;
	GtkWidget *newmenu, *openmenu, *quitmenu, *savemenu, *saveasmenu, *aboutmenu;
	GtkTextBuffer * buffer;
	GtkActionGroup  *action_group;
	GtkUIManager * uimanager;
	GtkBuilder * builder;
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
			G_CALLBACK (destroy), window);
	g_signal_connect (G_OBJECT (aboutmenu), "activate",
			G_CALLBACK (about_show), window);
	g_signal_connect (G_OBJECT (openmenu), "activate",
			G_CALLBACK (open_pdf_cb), window);
	g_signal_connect (G_OBJECT (newmenu), "activate",
			G_CALLBACK (new_pdf_cb), window);
	g_signal_connect (G_OBJECT (quitmenu), "activate",
			G_CALLBACK (destroy), window);
	g_signal_connect (G_OBJECT (savemenu), "activate",
			G_CALLBACK (save_txt_cb), window);
	
	return window;
}

gboolean
open_file (GtkWindow * window, const gchar * filename)
{
	GtkProgressBar * progressbar;
	GtkStatusbar * statusbar;
	guint id;
	PopplerPage * PDFPage;
	PopplerRectangle * rect;
	GError * err;
	gdouble width, height;
	gint pages, c;
	gchar *page, * uri, * msg;
	GVfs * vfs;
	GFile * gfile;
	GFileInfo * ginfo;
	GError * result;

	vfs = g_vfs_get_default ();

	if (g_vfs_is_active(vfs))
	{
		gfile = g_vfs_get_file_for_path (vfs, filename);
	}
	else
	{
		gfile = g_file_new_for_commandline_arg (filename);
	}
	ginfo = g_file_query_info (gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE,
		G_FILE_QUERY_INFO_NONE, NULL, &result);
	if (0 == g_file_info_get_attribute_uint64 (ginfo, 
		G_FILE_ATTRIBUTE_STANDARD_SIZE))
	{
		g_object_unref (gfile);
		g_object_unref (ginfo);
		g_warning ("%s", result->message);
		return FALSE;
	}
	uri = g_file_get_uri (gfile);
	err = NULL;
	pages = 0;
	rect = poppler_rectangle_new ();
	rect->x1 = rect->y1 = 0;
	progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (builder, "progressbar"));
	gtk_progress_bar_set_fraction (progressbar, 0.0);
	statusbar = GTK_STATUSBAR(gtk_builder_get_object (builder, "statusbar"));
	id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
	msg = g_strconcat (_("Loading ebook:"), g_file_get_basename (gfile), NULL);
	gtk_statusbar_push (statusbar, id, msg);
	PDFDoc = poppler_document_new_from_file (uri, NULL, &err);
	gtk_progress_bar_set_fraction (progressbar, 0.2);
	if (POPPLER_IS_DOCUMENT (PDFDoc))
	{
		pages = poppler_document_get_n_pages (PDFDoc);
		for (c = 0; c < pages; c++)
		{
			PDFPage = poppler_document_get_page (PDFDoc, c);
			gtk_progress_bar_set_fraction (progressbar, (c+1)/pages);
			poppler_page_get_size (PDFPage, &width, &height);
			rect->x2 = width;
			rect->y2 = height;
			page = poppler_page_get_text (PDFPage, POPPLER_SELECTION_LINE, rect);
			set_text (page);
			g_free (page);
		}
	}
	else
	{
		g_message ("err: %s", err->message);
		return FALSE;
	}
	gtk_progress_bar_set_fraction (progressbar, 0.0);
	
	msg = g_strconcat (PACKAGE, " - " , g_file_get_basename (gfile), NULL);
	gtk_window_set_title (window, msg);
	gtk_statusbar_push (statusbar, id, _("Done"));
	return TRUE;
}

void
open_pdf_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog, * window;
	GtkFileFilter *filter;

	window = (GtkWidget *)data;
	dialog = gtk_file_chooser_dialog_new (_("Open ebook (PDF)"),
										  GTK_WINDOW (window),
										  GTK_FILE_CHOOSER_ACTION_OPEN,
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
										  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All PDF Files (*.pdf)"));
	gtk_file_filter_add_mime_type (filter, "application/pdf");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		open_file (GTK_WINDOW(window), filename);
		/* this is the PDF filename, so free it. */
		g_free (filename);
		filename = NULL;
	}
	gtk_widget_destroy (dialog);
	return;
}
