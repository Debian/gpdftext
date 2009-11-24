/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ebookui.h
 * Copyright (C) Neil Williams 2009 <linux@codehelp.co.uk>
 * 
 * ebookui.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ebookui.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _EBOOKUI_H_
#define _EBOOKUI_H_

#include <gtk/gtk.h>
#include <glib/poppler-document.h>
#include <glib/poppler-page.h>
#include <gconf/gconf-client.h>

typedef struct _gconf
{
	guint id;
	gchar * key;
} ebGconf;

/** the gpdftext context struct 

Try to *not* add lots of GtkWidget pointers here, confine things
 to other stuff and get the widgets via builder.
 */
typedef struct _eb
{
	/** preference settings */
	GConfClient *client;
	/** one Gconf for each setting. */
	ebGconf paper_size;
	ebGconf editor_font;
	ebGconf spell_check;
	ebGconf page_number;
	ebGconf long_lines;
	ebGconf join_hyphens;
	ebGconf language;
	PopplerDocument * PDFDoc;
	/** everything else can be found from the one builder */
	GtkBuilder * builder;
	/** returned by the file_chooser as the export filename */
	gchar * filename;
	/** built from the PDF filename opened */
	GFile * gfile;
	/** the regular expressions - compiled once per app */
	GRegex * line, * page, *hyphen;
	/** the language for spell checking */
	gchar * lang;
	/** the undo stack - gets all user events */
	GTrashStack * undo_stack;
	/** the redo stack - only gets events popped off undo_stack */
	GTrashStack * redo_stack;
	/** boolean to save as PDF */
	gboolean save_pdf;
} Ebook;

void save_txt_cb (GtkWidget * widget, gpointer data);

GtkWidget* create_window (Ebook * ebook);

GtkBuilder* load_builder_xml (const gchar *root);

void about_show (void);

gboolean open_file (Ebook * ebook, const gchar * filename);

Ebook * new_ebook (void);

void gconf_data_fill (Ebook *ebook);

#endif
