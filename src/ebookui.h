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

#include <gtk/gtk.h>

typedef struct _eb Ebook;

void save_txt_cb (GtkWidget * widget, gpointer data);

void open_pdf_cb (GtkWidget *widget, gpointer data);

GtkWidget* create_window (Ebook * ebook);

GtkBuilder* load_builder_xml (const gchar *root);

void about_show (void);

gboolean open_file (Ebook * ebook, const gchar * filename);

Ebook * new_ebook (void);

void gconf_data_fill (Ebook *ebook);
