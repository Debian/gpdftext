/************************************************************************
 *            spell.h
 *
 *  Sun Oct 18 11:41:57 2009
 *  Copyright  2009  Neil Williams
 *  <linux@codehelp.co.uk>
 ***********************************************************************/

/*
 * spell.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * spell.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _SPELL_H_
#define _SPELL_H_

#include <gtk/gtk.h>
#include "ebookui.h"
#ifdef HAVE_GTKSPELL
#include <gtkspell/gtkspell.h>
#endif

void view_misspelled_words_cb (GtkWidget *w, gpointer data);

void spell_language_select_menuitem (Ebook *ebook, const gchar *lang);

void spell_language_changed_cb (GConfClient *client, guint id,
                                GConfEntry *entry, gpointer user_data);

void setup_languages (Ebook * ebook);

void spellcheck_changed_cb (GConfClient *client, guint id,
                            GConfEntry *entry, gpointer user_data);

void editor_font_changed_cb (GConfClient *client, guint id,
                             GConfEntry *entry, gpointer user_data);

void editor_font_cb(GtkWidget *button, gpointer data);

void editor_update_font(Ebook * ebook);

#endif
