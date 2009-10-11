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

#include <config.h>


#include "callbacks.h"
#include <glib/gi18n.h>
#include <glib/poppler-document.h>
#include <glib/poppler-page.h>

static PopplerDocument * PDFDoc;

GtkBuilder*
load_builder_xml (const gchar *root)
{
	gchar *path;
	GError* error = NULL;
	GtkBuilder* builder = gtk_builder_new ();
	
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

void
save_txt_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog, *window;
	GtkFileFilter *filter;

	window = (GtkWidget *)data;
	dialog = gtk_file_chooser_dialog_new (_("Save as text file"),
										  GTK_WINDOW (window),
										  GTK_FILE_CHOOSER_ACTION_SAVE,
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
										  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	/* Add filters for all XML files all XML files which appear to be Drivel
	   Drafts */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All text files"));
	gtk_file_filter_add_mime_type (filter, "text/plain");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
//		g_file_set_contents
	}
	else
	gtk_widget_destroy (dialog);
}

static G_GNUC_UNUSED void
text_changed (void)
{
	g_message ("text_changed");
}

#ifdef HAVE_CONFIG
static void
set_text (const gchar * text)
{
	GtkBuilder *builder;
//	GtkTextView * textview;
	GtkTextBuffer * buffer;
//	GtkTextTagTable * tt;
//	GtkWidget * box;
//	GtkTextIter start, end;
//	gint size;

	builder = load_builder_xml (NULL);
	if (!builder)
		return;
	return;
	/*	textview = GTK_TEXT_VIEW(gtk_builder_get_object (builder, "textview"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (builder, "textbuffer1"));
	box = GTK_WIDGET(gtk_builder_get_object (builder, "main_vbox"));
	if (!GTK_IS_BOX(box))
		g_message ("GtkBuilder is foobar");
	g_object_unref (buffer);
	gtk_widget_destroy (GTK_WIDGET(textview));
	tt = gtk_text_tag_table_new ();
	buffer = gtk_text_buffer_new (tt);
	textview = GTK_TEXT_VIEW(gtk_text_view_new_with_buffer (buffer));
	gtk_text_buffer_set_text (buffer, text, -1);
	gtk_box_pack_start (GTK_BOX(box), GTK_WIDGET(textview), FALSE, TRUE, 0);
	return;*/
//	buffer = gtk_text_view_get_buffer (textview);
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (builder, "textbuffer1"));
/*	buffer = gtk_text_view_
	buffer = gtk_text_view_get_buffer (textview);*/
	g_signal_connect (G_OBJECT (buffer), "changed",
		G_CALLBACK (text_changed), NULL);
	gtk_text_buffer_set_text (buffer, text, -1);
/*	gtk_text_view_set_buffer (textview, buffer);
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	size = strlen (text);
	gtk_text_buffer_insert (buffer, &start, text, size);
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);*/
//	g_message ("%s", text);
}
#endif
void
open_pdf_cb (GtkWidget *widget, gpointer data)
{
	gdouble width, height;
	gint pages, c;
	PopplerPage * PDFPage;
	PopplerRectangle * rect;
	gchar *page;
	GtkWidget *dialog, * window;
	GtkFileFilter *filter;

	pages = 0;
	rect = poppler_rectangle_new ();
	rect->x1 = rect->y1 = 0;
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
		gchar *filename, *uri;
		GError * err = NULL;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		uri = g_strconcat ("file://", filename, NULL);
		PDFDoc = poppler_document_new_from_file (uri, NULL, &err);
		if (POPPLER_IS_DOCUMENT (PDFDoc))
		{
			pages = poppler_document_get_n_pages (PDFDoc);
			for (c = 0; c < pages; c++)
			{
				PDFPage = poppler_document_get_page (PDFDoc, c);
				poppler_page_get_size (PDFPage, &width, &height);
				rect->x2 = width;
				rect->y2 = height;
				page = poppler_page_get_text (PDFPage, POPPLER_SELECTION_LINE, rect);
				set_text (page);
				g_free (page);
			}
		}
		else
			g_message ("err: %s", err->message);
		g_free (filename);
		g_free (uri);
	}
	gtk_widget_destroy (dialog);
	return;
}
