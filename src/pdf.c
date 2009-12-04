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

The problem (and the goal) is to harness pango and cairo
to change the paper size from A4 to something more like a book.
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

/** width for an A5 PDF file in inches. */
static gdouble a5_width = 5.8;
/** height for an A5 PDF file in inches. */
static gdouble a5_height = 8.3;
/** width for a B5 PDF file in inches. */
static gdouble b5_width = 6.9;
/** height for a B5 PDF file in inches. */
static gdouble b5_height = 9.8;

#define POINTS 72.0
/** FIXME: make margins configurable via GConf
 (means making the preferences dialog use a notebook and tabs. */
#define SIDE_MARGIN 20
#define EDGE_MARGIN 40

typedef struct _psync
{
	Ebook * ebook;
	PangoContext * context;
	PangoLayout * layout;
	PangoLayoutIter * iter;
	PangoFontDescription * desc;
	gdouble height;
	gdouble width;
	gdouble page_height;
	guint lines_per_page;
	cairo_t *cr;
	cairo_surface_t *surface;
	gchar * text;
	gsize pos;
	GtkProgressBar * progressbar;
	GtkStatusbar * statusbar;
} Pqueue;

static PangoLayout *
make_new_page (PangoContext * context, PangoFontDescription * desc,
				gdouble height, gdouble width)
{
	PangoLayout *layout;

	layout = pango_layout_new (context);
	pango_layout_set_justify (layout, TRUE);
	pango_layout_set_spacing (layout, 1.5);
	pango_layout_set_width (layout, pango_units_from_double(width - SIDE_MARGIN));
	pango_layout_set_height (layout, pango_units_from_double(height - EDGE_MARGIN));
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_font_description (layout, desc);
	return layout;
}

static void
create_pages (Pqueue * queue)
{
	gdouble line_height;
	guint i, index, id;
	PangoLayoutLine * line;
	PangoRectangle ink_rect, logical_rect;

	g_return_if_fail (queue);
	g_return_if_fail (queue->pos < strlen(queue->text));
	while (queue->pos < strlen (queue->text))
	{
		while (gtk_events_pending ())
			gtk_main_iteration ();
		for (i = 0; i < queue->lines_per_page; i++)
		{
			line = pango_layout_iter_get_line (queue->iter);
			pango_layout_iter_next_line (queue->iter);
			pango_layout_iter_get_line_extents (queue->iter, &ink_rect, &logical_rect);
			index = pango_layout_iter_get_index (queue->iter);
			line_height = logical_rect.height / PANGO_SCALE;
			if ((queue->page_height + line_height) > (queue->height - (EDGE_MARGIN/2)))
			{
				queue->pos += index;
				queue->page_height = EDGE_MARGIN;
				gtk_progress_bar_pulse (queue->progressbar);
				pango_cairo_update_layout (queue->cr, queue->layout);
				queue->layout = make_new_page (queue->context, queue->desc, queue->height, queue->width);
				pango_layout_set_text (queue->layout, (queue->text+queue->pos), -1);
				queue->iter = pango_layout_get_iter (queue->layout);
				pango_cairo_show_layout_line (queue->cr, line);
				pango_cairo_update_layout (queue->cr, queue->layout);
				cairo_show_page (queue->cr);
			}
			else
				pango_cairo_show_layout_line (queue->cr, line);
			queue->page_height += line_height;
			cairo_move_to (queue->cr, SIDE_MARGIN / 2, queue->page_height);
		}
	}
	pango_layout_iter_free (queue->iter);
	gtk_progress_bar_set_fraction (queue->progressbar, 0.0);
	cairo_surface_destroy(queue->surface);
	pango_font_description_free (queue->desc);
	g_object_unref (queue->context);
	g_object_unref (queue->layout);
	cairo_destroy (queue->cr);
	id = gtk_statusbar_get_context_id (queue->statusbar, PACKAGE);
	gtk_statusbar_push (queue->statusbar, id, _("Saved PDF file."));
}

void buffer_to_pdf (Ebook * ebook)
{
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	Pqueue queue;
	gchar * size, *editor_font;

	/* A4 initial */
	queue.ebook = ebook;
	queue.pos = 0;
	queue.width = 8.3 * POINTS;
	queue.height = 11.7 * POINTS;
	queue.page_height = 40.0;
	g_return_if_fail (ebook);
	g_return_if_fail (ebook->filename);
	size = gconf_client_get_string (ebook->client, ebook->paper_size.key, NULL);
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	queue.progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (ebook->builder, "progressbar"));
	queue.statusbar = GTK_STATUSBAR(gtk_builder_get_object (ebook->builder, "statusbar"));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	queue.text = g_strdup(gtk_text_buffer_get_text (buffer, &start, &end, TRUE));
	editor_font = gconf_client_get_string(ebook->client, ebook->editor_font.key, NULL);
	if (0 == g_strcmp0 (size, "A5"))
	{
		queue.width = a5_width * POINTS;
		queue.height = a5_height * POINTS;
	}
	if (0 == g_strcmp0 (size, "B5"))
	{
		queue.width = b5_width * POINTS;
		queue.height = b5_height * POINTS;
	}
	queue.surface = cairo_pdf_surface_create (ebook->filename, queue.width, queue.height);
	queue.cr = cairo_create (queue.surface);
	queue.context = pango_cairo_create_context (queue.cr);
	/* pango_cairo_create_layout is wasteful with a lot of text. */
	queue.desc = pango_font_description_from_string (editor_font);
	queue.layout = make_new_page (queue.context, queue.desc, queue.height, queue.width);
	pango_layout_set_text (queue.layout, queue.text, -1);
	cairo_move_to (queue.cr, SIDE_MARGIN / 2, EDGE_MARGIN / 2);
	gtk_progress_bar_set_fraction (queue.progressbar, 0.0);
	queue.lines_per_page = pango_layout_get_line_count (queue.layout);
	queue.iter = pango_layout_get_iter (queue.layout);
	create_pages (&queue);
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
	gssize size, old;
	GError * err;

	err = NULL;
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

	if (!ebook->builder)
		ebook->builder = load_builder_xml (NULL);
	if (!ebook->builder)
		return;
	old = strlen (text);
	text = g_utf8_normalize (text, old, G_NORMALIZE_ALL);
	size = strlen (text);
	if (size < old)
		ebook->utf8_count += (old - size);
	textview = GTK_TEXT_VIEW(gtk_builder_get_object (ebook->builder, "textview"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	if ((text != NULL) && (g_utf8_validate (text, size, NULL)))
	{
		gtk_text_buffer_insert (buffer, &end, text, size);
		gtk_text_buffer_set_modified (buffer, TRUE);
	}
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
	const gchar * str;
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
		if (queue->ebook->utf8_count > 0)
		{
			/* Translators: Please try to keep this string brief,
			 there often isn't a lot of room in the statusbar. */
			str = ngettext ("%ld non-UTF8 character was removed",
				"%ld non-UTF-8 characters were removed", queue->ebook->utf8_count);
			msg = g_strdup_printf (str, queue->ebook->utf8_count);
			id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
			gtk_statusbar_push (statusbar, id, msg);
			g_free (msg);
		}
		else
		{
			gtk_statusbar_push (statusbar, id, _("Done"));
		}
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
	GtkTextView * text_view;
	GtkFileFilter *pdffilter, *txtfilter;
	Ebook * ebook;

	ebook = (Ebook *)data;
	window = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	text_view = GTK_TEXT_VIEW(gtk_builder_get_object (ebook->builder, "textview"));
	dialog = gtk_file_chooser_dialog_new (_("Open file"),
		GTK_WINDOW (window), GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
		GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	pdffilter = gtk_file_filter_new ();
	gtk_file_filter_set_name (pdffilter, _("All PDF Files (*.pdf)"));
	gtk_file_filter_add_mime_type (pdffilter, "application/pdf");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), pdffilter);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), pdffilter);
	txtfilter = gtk_file_filter_new ();
	gtk_file_filter_set_name (txtfilter, _("ASCII text files (*.txt)"));
	gtk_file_filter_add_mime_type (txtfilter, "text/plain");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), txtfilter);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename;
		GtkFileFilter * f;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		// read the filter property.
		g_object_get (G_OBJECT(GTK_FILE_CHOOSER (dialog)), "filter", &f, NULL);
		if (f == pdffilter)
		{
			/* get the dialog out of the way before the work starts. */
			gtk_widget_destroy (dialog);
			open_file (ebook, filename);
		}
		else
		{
			gchar * contents, * msg;
			GtkTextBuffer * buffer;
			GtkStatusbar * statusbar;
			GtkProgressBar * progressbar;
			guint id;

			/* get the dialog out of the way before the work starts. */
			gtk_widget_destroy (dialog);
			progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (ebook->builder, "progressbar"));
			statusbar = GTK_STATUSBAR(gtk_builder_get_object (ebook->builder, "statusbar"));
			id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
			/* typical text files are v.large and this can block. FIXME. */
			g_file_get_contents (filename, &contents, NULL, NULL);
			buffer = gtk_text_view_get_buffer (text_view);
			gtk_text_buffer_set_text (buffer, contents, -1);
			msg = g_strconcat (PACKAGE, " - ", g_filename_display_basename (filename), NULL);
			gtk_window_set_title (GTK_WINDOW(window), msg);
			gtk_progress_bar_set_text (progressbar, "");
			gtk_progress_bar_set_fraction (progressbar, 0.0);
			gtk_statusbar_push (statusbar, id, _("Done"));
		}
		/* this is the PDF filename, so free it. */
		g_free (filename);
		filename = NULL;
		return;
	}
	gtk_widget_destroy (dialog);
}
