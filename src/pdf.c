/***************************************************************************
 *            pdf.c
 *
 *  Wed Oct 14 14:50:35 2009
 *  Copyright  2009  Neil Williams
 *  <linux@codehelp.co.uk>
 ****************************************************************************/

/*
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
 
/** general PDF functions. */
/**

Idea: file_selector SaveAs to get a filename, then create a
 new PDF - without preview support - and choice of font.

Need to add a new window to the GTKBuilder XML using glade-3
 that just shows a GdkPixbuf, also with controls for next page
 first page etc. or just use libevview? (nah, part of libevince1,
 too big.)

The problem (and the goal) is to harness poppler to change the
paper size from A4 to something more like a book.
Something like A5 portrait or B5 portrait.
http://www.cl.cam.ac.uk/~mgk25/iso-paper.html
http://www.hintsandthings.co.uk/office/paper.htm

   width   height
A4 210mm x 297mm   8.3 x 11.7 inches
A5 148mm × 210mm   5.8 x  8.3 inches
B5 176mm × 250mm   6.9 x  9.8 inches

Ratio of 0.7:1
Hence the unusual shape of the main gpdftext window.
(440x630, same proportions as A5.)

Use cairo-pdf.
 Creating a PDF of a non-standard size in poppler is
 (or at least appears to be) more trouble than it is worth. (The
 functions to do this are private within poppler and are not
 exported at all in poppler-glib.)

The main window size should also be configurable via GConf, with the
default values retained.

This will also probably need some kind of context object
that can be passed around between functions to provide the
pointer to the builder: GPContext * context; The context could
then have other bits that may become necessary, like
the preferences values. (Add to Ebook).

(See spell.c : editor_update_font )

*/

#include <glib.h>
#include <glib/gi18n.h>
#include "ebookui.h"
#include <config.h>
#ifdef HAVE_GTKSPELL
#include <gtkspell/gtkspell.h>
#endif /* HAVE_GTKSPELL */
#include <cairo/cairo-pdf.h>
#include "spell.h"
#include "pdf.h"

/** width for an A5 PDF file in mm. */
static gdouble a5_width = 148.0;
/** height for an A5 PDF file in mm. */
static gdouble a5_height = 210.0;
/** width for a B5 PDF file in mm. */
static gdouble b5_width = 176.0;
/** height for a B5 PDF file in mm. */
static gdouble b5_height = 250.0;

void buffer_to_ps (Ebook * ebook)
{
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	cairo_t *cr;
	cairo_surface_t *surface;
	PangoLayout *layout;
	PangoFontDescription *desc;
	gchar * size, * text, *editor_font;
	gdouble width, height;

	width = 210.0;
	height = 297.0;
	g_return_if_fail (ebook);
	g_return_if_fail (ebook->filename);
	size = gconf_client_get_string (ebook->client, ebook->paper_size.key, NULL);
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
	editor_font = gconf_client_get_string(ebook->client, ebook->editor_font.key, NULL);
	if (0 == g_strcmp0 (size, "A5"))
	{
		width = a5_width;
		height = a5_height;
	}
	if (0 == g_strcmp0 (size, "B5"))
	{
		width = b5_width;
		height = b5_height;
	}
	surface = cairo_pdf_surface_create (ebook->filename, width, height);
	cr = cairo_create (surface);
	cairo_surface_destroy(surface);
	layout = pango_cairo_create_layout (cr);
	pango_layout_set_text (layout, text, -1);
	cairo_move_to (cr, 10, 30);
	cairo_set_font_size (cr, 90.0);
	/* editor_font contains font name and font size. */
	desc = pango_font_description_from_string (editor_font);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);
	pango_cairo_show_layout (cr, layout);
	g_object_unref (layout);
	cairo_destroy (cr);
}

/** 
 runs once per page
*/
static void
set_text (Ebook * ebook, gchar * text, 
			gboolean lines_state, gboolean page_state, gboolean hyphens_state)
{
	GtkTextView * textview;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	gssize size;
	GError * err;

	err = NULL;
	size = strlen (text);

	if (lines_state)
		text = g_regex_replace (ebook->line, text, -1, 0, " \\1",0 , &err);
	if (err)
		g_warning ("line replace: %s", err->message);

	if (page_state)
		text = g_regex_replace_literal (ebook->page, text, -1, 0, " ",0 , &err);
	if (err)
		g_warning ("page replace: %s", err->message);

	if (hyphens_state)
		text = g_regex_replace (ebook->hyphen, text, -1, 0, "\\1",0 , &err);
	if (err)
		g_warning ("hyphen replace: %s", err->message);

	if (!g_utf8_validate (text, -1, NULL))
	{
		/** FIXME: this should be a user-level warning. 
		 Needs a dialog. */
		g_warning ("validate %s", text);
		return;
	}
	if (!text)
		return;
	if (!ebook->builder)
		ebook->builder = load_builder_xml (NULL);
	if (!ebook->builder)
		return;
	size = strlen (text);
	text = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
	textview = GTK_TEXT_VIEW(gtk_builder_get_object (ebook->builder, "textview"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_insert (buffer, &end, text, size);
	gtk_text_buffer_set_modified (buffer, TRUE);
	gtk_widget_show (GTK_WIDGET(textview));
}

typedef struct _async
{
	Ebook * ebook;
	PopplerRectangle * rect;
	gint c;
	gboolean lines;
	gboolean hyphens;
	gboolean pagenums;
	/* whether to re-enable spell checking once loading is complete. */
	gboolean spell_state;
} Equeue;

/**
 called repeatedly to add more text to the editor as each
 page is read from the PDF file, via g_timeout_add.

 Spelling needs to be turned off during loading, re-attached it at the end
 if GConf says spelling should be enabled.

 This function is asynchronous - a background task.
 Don't do things with the progress bar or status bar whilst a load
 task could be running.
 */
static gboolean
load_pdf (gpointer data)
{
	GtkProgressBar * progressbar;
	GtkStatusbar * statusbar;
	GtkTextView * text_view;
	GtkTextBuffer * buffer;
	gchar *page, * msg, * lang;
	gdouble fraction, step, width, height;
	PopplerPage * PDFPage;
	gint pages;
	guint id;
	Equeue * queue;

	queue= (Equeue *)data;
	text_view = GTK_TEXT_VIEW(gtk_builder_get_object (queue->ebook->builder, "textview"));
	buffer = gtk_text_view_get_buffer (text_view);
	if (!queue)
	{
		gtk_text_buffer_end_user_action (buffer);
		return FALSE;
	}
	if (!queue->ebook)
	{
		gtk_text_buffer_end_user_action (buffer);
		return FALSE;
	}
	progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (queue->ebook->builder, "progressbar"));
	statusbar = GTK_STATUSBAR(gtk_builder_get_object (queue->ebook->builder, "statusbar"));
	id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
	pages = poppler_document_get_n_pages (queue->ebook->PDFDoc);
	if (queue->c >= pages)
	{
		lang = gconf_client_get_string (queue->ebook->client,
			queue->ebook->language.key, NULL);
#ifdef HAVE_GTKSPELL
		/* spell_attach is already a background task. */
		if (queue->spell_state)
			gtkspell_new_attach (text_view,
				(lang == NULL || *lang == '\0') ? NULL : lang, NULL);
#endif
		gtk_progress_bar_set_text (progressbar, "");
		gtk_progress_bar_set_fraction (progressbar, 0.0);
		gtk_statusbar_push (statusbar, id, _("Done"));
		return FALSE;
	}
	PDFPage = poppler_document_get_page (queue->ebook->PDFDoc, queue->c);
	fraction = 0.0;
	/* fraction never reaches 1.0 - allow room for spelling attachment. */
	if (queue->spell_state)
		step = 0.90/(gdouble)pages;
	else
		step = 0.99/(gdouble)pages;
	fraction += step * queue->c;
	/* update progress bar as we go */
	gtk_progress_bar_set_fraction (progressbar, fraction);
	poppler_page_get_size (PDFPage, &width, &height);
	queue->rect->x2 = width;
	queue->rect->y2 = height;
	page = poppler_page_get_text (PDFPage, POPPLER_SELECTION_LINE, queue->rect);
	set_text (queue->ebook, page, queue->lines, queue->pagenums, queue->hyphens);
	g_free (page);
	queue->c++;
	/* add the page to the progressbar count. */
	msg = g_strdup_printf ("%d/%d", queue->c, pages);
	gtk_progress_bar_set_text (progressbar, msg);
	g_free (msg);
	/* more to do yet, so return TRUE to call me again. */
	return TRUE;
}

gboolean
open_file (Ebook * ebook, const gchar * filename)
{
	GtkProgressBar * progressbar;
	GtkStatusbar * statusbar;
	guint id;
	GtkWidget * window;
	PopplerRectangle * rect;
	GError * err;
	gint pages;
	gchar * uri, * msg;
	GVfs * vfs;
	GFileInfo * ginfo;
	GError * result;
	GConfValue *value;
	gboolean lines, hyphens, pagenums;

	vfs = g_vfs_get_default ();

	if (g_vfs_is_active(vfs))
		ebook->gfile = g_vfs_get_file_for_path (vfs, filename);
	else
		ebook->gfile = g_file_new_for_commandline_arg (filename);
	ginfo = g_file_query_info (ebook->gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE,
		G_FILE_QUERY_INFO_NONE, NULL, &result);
	if (0 == g_file_info_get_attribute_uint64 (ginfo, 
		G_FILE_ATTRIBUTE_STANDARD_SIZE))
	{
		g_object_unref (ebook->gfile);
		g_object_unref (ginfo);
		g_warning ("%s", result->message);
		return FALSE;
	}
	uri = g_file_get_uri (ebook->gfile);
	err = NULL;
	pages = 0;
	rect = poppler_rectangle_new ();
	rect->x1 = rect->y1 = 0;
	window = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (ebook->builder, "progressbar"));
	gtk_progress_bar_set_fraction (progressbar, 0.0);
	statusbar = GTK_STATUSBAR(gtk_builder_get_object (ebook->builder, "statusbar"));
	id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
	msg = g_strconcat (_("Loading ebook:"), g_file_get_basename (ebook->gfile), NULL);
	gtk_statusbar_push (statusbar, id, msg);
	ebook->PDFDoc = poppler_document_new_from_file (uri, NULL, &err);
	gtk_progress_bar_set_fraction (progressbar, 0.0);

	/* long lines support */
	value = gconf_client_get(ebook->client, ebook->long_lines.key, NULL);
	if (value)
		lines = gconf_value_get_bool(value);
	else
		lines = TRUE;

	/* page numbers support */
	value = gconf_client_get(ebook->client, ebook->page_number.key, NULL);
	if (value)
		pagenums = gconf_value_get_bool(value);
	else
		pagenums = TRUE;

	/* join hyphens support */
	value = gconf_client_get(ebook->client, ebook->join_hyphens.key, NULL);
	if (value)
		hyphens = gconf_value_get_bool(value);
	else
		hyphens = TRUE;

	if (POPPLER_IS_DOCUMENT (ebook->PDFDoc))
	{
#ifdef HAVE_GTKSPELL
		GtkSpell *spell;
		gchar *lang;
#endif
		GtkWidget * spell_check;
		GtkTextView * text_view;
		GtkTextBuffer * buffer;
		gboolean state;
		gdouble fraction, step;
		static Equeue queue;

		spell_check = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "spellcheckmenuitem"));
		text_view = GTK_TEXT_VIEW(gtk_builder_get_object (ebook->builder, "textview"));
		buffer = gtk_text_view_get_buffer (text_view);
		state = gconf_client_get_bool (ebook->client, ebook->spell_check.key, NULL);
#ifdef HAVE_GTKSPELL
		spell = gtkspell_get_from_text_view (text_view);
		lang = gconf_client_get_string (ebook->client, ebook->language.key, NULL);
		/* updating the text area with spell enabled is very slow */
		if (state)
			gtkspell_detach (spell);
#endif
		pages = poppler_document_get_n_pages (ebook->PDFDoc);
		fraction = 0.0;
		step = 0.99/(gdouble)pages;
		queue.ebook = ebook;
		queue.c = 0;
		queue.lines = lines;
		queue.hyphens = hyphens;
		queue.pagenums = pagenums;
		queue.rect = rect;
		/* whether to enable spell once all pages are loaded. */
		queue.spell_state = state;
		/* loading a file is a single user action */
		gtk_text_buffer_begin_user_action (buffer);
		g_timeout_add (30, load_pdf, &queue);
	}
	else
	{
		g_message ("err: %s", err->message);
		return FALSE;
	}
	msg = g_strconcat (PACKAGE, " - ", g_file_get_basename (ebook->gfile), NULL);
	gtk_window_set_title (GTK_WINDOW(window), msg);
	return TRUE;
}

void
open_pdf_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog, * window;
	GtkFileFilter *filter;
	Ebook * ebook;

	ebook = (Ebook *)data;
	window = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	dialog = gtk_file_chooser_dialog_new (_("Open ebook (PDF)"),
		GTK_WINDOW (window), GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
		GTK_RESPONSE_ACCEPT, NULL);
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
		/* get the dialog out of the way before the work starts. */
		gtk_widget_destroy (dialog);
		open_file (ebook, filename);
		/* this is the PDF filename, so free it. */
		g_free (filename);
		filename = NULL;
		return;
	}
	gtk_widget_destroy (dialog);
}
