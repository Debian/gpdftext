/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ebookui.c
 * Copyright (C) Neil Williams 2009 <linux@codehelp.co.uk>
 * 
 * ebookui.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ebookui.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* main GTK+ interface functions  */

#include <config.h>

#include <glib/gi18n.h>
#include "ebookui.h"
#ifdef HAVE_GTKSPELL
#include <gtkspell/gtkspell.h>
#endif /* HAVE_GTKSPELL */
#include "spell.h"
#include "pdf.h"

Ebook *
new_ebook (void)
{
	Ebook * ebook;
	GError * err;

	err = NULL;
	ebook = g_new0 (Ebook, 1);
	ebook->client = gconf_client_get_default ();
	ebook->line = g_regex_new ("\n(.[^\\s])", 0, 0, &err);
	if (err)
		g_warning ("new line: %s", err->message);
	ebook->page = g_regex_new ("\n\\s+\\d+\\s?\n", 0, 0, &err);
	if (err)
		g_warning ("new para: %s", err->message);
	ebook->hyphen = g_regex_new ("(\\w)-[\\s\n]+", 0, 0, &err);
	if (err)
		g_warning ("new hyphen: %s", err->message);
	gconf_data_fill (ebook);
	return ebook;
}

GtkBuilder*
load_builder_xml (const gchar *root)
{
	gchar *path;
	GError* error = NULL;
	GtkBuilder * builder = NULL;

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

static void
gconf_data_free (Ebook *ebook)
{
	gconf_client_notify_remove (ebook->client, ebook->spell_check.id);
	gconf_client_notify_remove (ebook->client, ebook->paper_size.id);
	gconf_client_notify_remove (ebook->client, ebook->editor_font.id);
	gconf_client_notify_remove (ebook->client, ebook->page_number.id);
	gconf_client_notify_remove (ebook->client, ebook->long_lines.id);
	gconf_client_notify_remove (ebook->client, ebook->join_hyphens.id);
	gconf_client_notify_remove (ebook->client, ebook->language.id);
	g_free (ebook->paper_size.key);
	g_free (ebook->editor_font.key);
	g_free (ebook->spell_check.key);
	g_free (ebook->page_number.key);
	g_free (ebook->long_lines.key);
	g_free (ebook->join_hyphens.key);
	g_free (ebook->language.key);
}

static void
destroy_event_cb (GtkWidget * window, GdkEvent * e, gpointer user_data)
{
	Ebook * ebook;

	ebook = (Ebook *)user_data;
	g_regex_unref (ebook->line);
	g_regex_unref (ebook->page);
	gconf_data_free (ebook);
	g_free (ebook);
	gtk_main_quit();
}

static void
destroy_cb (GtkWidget * window, gpointer user_data)
{
	Ebook * ebook;

	ebook = (Ebook *)user_data;
	g_regex_unref (ebook->line);
	g_regex_unref (ebook->page);
	gconf_data_free (ebook);
	g_free (ebook);
	gtk_main_quit ();
}

/** support dropping line-wrapping to make it easier to
 * spot shortened lines. */

/** optional regexp for lines that do not start with spaces
 * - replace newline with a space. */

/** FIXME: Needs to be able to open text files too. */

/** FIXME: build a regexp to convert inverted commas? 
 * might need to cope with '" types. */

typedef struct _udata
{
	GTrashStack * next;
	gint command;
	gint start;
	gint end;
	gssize len;
	gboolean add;
	gchar * str;
} UData;

static void
delete_range_cb (GtkTextBuffer *textbuffer, GtkTextIter *start,
    GtkTextIter *end, gpointer user_data)
{
	GtkWidget * undo, * undomenu;
	gchar * str;
	Ebook * ebook;
	UData * ud;

	ebook = (Ebook *)user_data;
	undo = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undo_button"));
	undomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undomenuitem"));
	ud = g_new0(UData, 1);
	str = gtk_text_iter_get_text (start, end);
	ud->str = g_strdup(str);
	ud->add = FALSE;
	ud->start = gtk_text_iter_get_offset (start);
	ud->end = gtk_text_iter_get_offset(end);
	ud->len = strlen (str);
	g_trash_stack_push (&ebook->undo_stack, ud);
	gtk_widget_set_sensitive (undo, TRUE);
	gtk_widget_set_sensitive (undomenu, TRUE);
}

static void
insert_text_cb (GtkTextBuffer *textbuffer, GtkTextIter *location,
    gchar *text, gint len, gpointer user_data)
{
	GtkWidget * undo, * undomenu;
	Ebook * ebook;
	UData * ud;

	ebook = (Ebook *)user_data;
	undo = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undo_button"));
	undomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undomenuitem"));
	ud = g_new0(UData, 1);
	ud->str = g_strdup(text);
	ud->add = TRUE;
	ud->start = gtk_text_iter_get_offset(location);
	ud->len = strlen (text);
	ud->end = gtk_text_iter_get_offset(location) + (gint)len;
	g_trash_stack_push (&ebook->undo_stack, ud);
	gtk_widget_set_sensitive (undo, TRUE);
	gtk_widget_set_sensitive (undomenu, TRUE);
}

static void
undo_cb (GtkWidget * widget, gpointer user_data)
{
	GtkWidget * redo, * undo, * redomenu, * undomenu;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	Ebook * ebook;
	UData * ud;

	ebook = (Ebook *)user_data;
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	redo = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "redo_button"));
	undo = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undo_button"));
	redomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "redomenuitem"));
	undomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undomenuitem"));
	// to undo, reverse the action from the first item in the stack.
	ud = g_trash_stack_pop (&ebook->undo_stack);
	g_return_if_fail (ud);
	g_trash_stack_push (&ebook->redo_stack, ud);
	if (ud->add)
	{
		gtk_text_buffer_get_iter_at_offset (buffer, &start, ud->start);
		gtk_text_buffer_get_iter_at_offset (buffer, &end, ud->end);
		gtk_text_buffer_delete (buffer, &start, &end);
		g_trash_stack_pop (&ebook->undo_stack);
	}
	else
	{
		gtk_text_buffer_get_iter_at_offset (buffer, &start, ud->start);
		gtk_text_buffer_insert (buffer, &start, ud->str, ud->len);
		g_trash_stack_pop (&ebook->undo_stack);
	}
	gtk_widget_set_sensitive (redo, TRUE);
	gtk_widget_set_sensitive (redomenu, TRUE);
	if (!ebook->undo_stack)
	{
		gtk_widget_set_sensitive (undo, FALSE);
		gtk_widget_set_sensitive (undomenu, FALSE);
	}
}

static void
redo_cb (GtkWidget * widget, gpointer user_data)
{
	GtkWidget * redo, * undo, * redomenu, * undomenu;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	Ebook * ebook;
	UData * ud;

	ebook = (Ebook *)user_data;
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	undo = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undo_button"));
	redo = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "redo_button"));
	redomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "redomenuitem"));
	undomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undomenuitem"));
	// to redo, replay the action from the next item in the stack.
	ud = g_trash_stack_pop (&ebook->redo_stack);
	g_return_if_fail (ud);
	if (ud->add)
	{
		gtk_text_buffer_get_iter_at_offset (buffer, &start, ud->start);
		gtk_text_buffer_insert (buffer, &start, ud->str, ud->len);
	}
	else
	{
		gtk_text_buffer_get_iter_at_offset (buffer, &start, ud->start);
		gtk_text_buffer_get_iter_at_offset (buffer, &end, ud->end);
		gtk_text_buffer_delete (buffer, &start, &end);
	}
	gtk_widget_set_sensitive (undo, TRUE);
	gtk_widget_set_sensitive (undomenu, TRUE);
	if (!ebook->redo_stack)
	{
		gtk_widget_set_sensitive (redo, FALSE);
		gtk_widget_set_sensitive (redomenu, FALSE);
	}
}

static void
save_sensitive_cb (GtkTextBuffer *textbuffer, gpointer user_data)
{
	GtkWidget * button, * menu;
	gboolean modified;
	Ebook * ebook;

	ebook = (Ebook *)user_data;
	menu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "savemenuitem"));
	button = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "save_button"));
	modified = gtk_text_buffer_get_modified (textbuffer);
	gtk_widget_set_sensitive (button, modified);
	gtk_widget_set_sensitive (menu, modified);
}

static void
save_file (Ebook * ebook)
{
	GtkProgressBar * progressbar;
	GtkStatusbar * statusbar;
	GtkTextView * textview;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	GtkWindow * window;
	gchar * text, * msg;
	GError * err;
	guint id;

	if (!ebook->builder)
		return;
	if (!ebook->filename)
		return;
	err = NULL;
	window = GTK_WINDOW(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (ebook->builder, "progressbar"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	gtk_progress_bar_set_fraction (progressbar, 0.0);
	statusbar = GTK_STATUSBAR(gtk_builder_get_object (ebook->builder, "statusbar"));
	id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
	if (ebook->save_pdf)
	{
		gtk_statusbar_push (statusbar, id, _("Saving PDF file"));
		buffer_to_pdf (ebook);
		id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
		gtk_statusbar_push (statusbar, id, _("Saved PDF file."));
	}
	else
	{
		gtk_statusbar_push (statusbar, id, _("Saving text file"));
		textview = GTK_TEXT_VIEW(gtk_builder_get_object (ebook->builder, "textview"));
		gtk_text_buffer_get_bounds (buffer, &start, &end);
		text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
		g_file_set_contents (ebook->filename, text, -1, &err);
		id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
		gtk_statusbar_push (statusbar, id, _("Saved text file."));
	}
	msg = g_strconcat (PACKAGE, " - " , g_path_get_basename (ebook->filename), NULL);
	gtk_window_set_title (GTK_WINDOW(window), msg);
	gtk_text_buffer_set_modified (buffer, FALSE);
}

static void
saveas_cb (GtkWidget * widget, gpointer user_data)
{
	Ebook * ebook;

	ebook = (Ebook *)user_data;
	g_return_if_fail (ebook);
	if (ebook->filename)
	{
		g_free (ebook->filename);
		ebook->filename = NULL;
	}
	ebook->save_pdf = FALSE;
	save_txt_cb (widget, user_data);
}

void
save_txt_cb (GtkWidget * widget, gpointer user_data)
{
	GtkWidget *dialog, *window;
	GtkFileFilter *txt_filter, *pdf_filter;
	Ebook * ebook;

	ebook = (Ebook *)user_data;
	g_return_if_fail (ebook);
	/* for a save_as_txt_cb, just omit this check */
	if (ebook->filename)
	{
		save_file (ebook);
		return;
	}
	window = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	dialog = gtk_file_chooser_dialog_new (_("Save file as"),
		GTK_WINDOW (window),GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE,
		GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	txt_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (txt_filter, _("All text files"));
	gtk_file_filter_add_mime_type (txt_filter, "text/plain");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), txt_filter);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), txt_filter);
	pdf_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (pdf_filter, _("PDF files"));
	gtk_file_filter_add_mime_type (pdf_filter, "application/x-pdf");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), pdf_filter);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkFileFilter * f;
		ebook->filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		// read the filter property.
		g_object_get (G_OBJECT(GTK_FILE_CHOOSER (dialog)), "filter", &f, NULL);
		if (f == pdf_filter)
			ebook->save_pdf = TRUE;
		gtk_widget_destroy (dialog);
		save_file (ebook);
	}
	else
		gtk_widget_destroy (dialog);
}

static void
new_pdf_cb (GtkImageMenuItem *self, gpointer user_data)
{
	guint id;
	GtkWidget * redo, * undo, * redomenu, * undomenu;
	GtkTextBuffer * buffer;
	GtkProgressBar * progressbar;
	GtkStatusbar * statusbar;
	Ebook * ebook;
	GtkWindow * window;

	ebook = (Ebook *)user_data;
	/** FIXME: this is messy and repetitive - wrap in a foo_valid func. */
	if (!ebook->builder)
		ebook->builder = load_builder_xml (NULL);
	if (!ebook->builder)
		return;
	if (ebook->filename)
	{
		g_free (ebook->filename);
		ebook->filename = NULL;
	}
	window = GTK_WINDOW(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	undo = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undo_button"));
	redo = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "redo_button"));
	redomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "redomenuitem"));
	undomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undomenuitem"));
	gtk_text_buffer_set_text (buffer, "", 0);
	progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object (ebook->builder, "progressbar"));
	gtk_progress_bar_set_fraction (progressbar, 0.0);
	statusbar = GTK_STATUSBAR(gtk_builder_get_object (ebook->builder, "statusbar"));
	id = gtk_statusbar_get_context_id (statusbar, PACKAGE);
	gtk_statusbar_push (statusbar, id, _("new file"));
	gtk_window_set_title (window, _("eBook PDF editor"));
	gtk_text_buffer_set_modified (buffer, FALSE);
	gtk_widget_set_sensitive (redo, FALSE);
	gtk_widget_set_sensitive (undo, FALSE);
	gtk_widget_set_sensitive (redomenu, FALSE);
	gtk_widget_set_sensitive (undomenu, FALSE);
}

static void
preferences_close_cb (GtkWidget *widget, gint arg, gpointer data)
{
	gtk_widget_hide (widget);
}

static void
paper_radio_cb (GtkWidget *w, gpointer data)
{
	GtkWidget * a4, * a5, * b5;
	gboolean a4state, a5state, b5state;
	Ebook * ebook;

	ebook = (Ebook *)data;
	a5 = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "a5radiobutton"));
	b5 = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "b5radiobutton"));
	a4 = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "a4radiobutton"));
	a5state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (a5));
	if (a5state)
	{
		gconf_client_set_string (ebook->client, ebook->paper_size.key, "A5", NULL);
		return;
	}
	b5state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b5));
	if (b5state)
	{
		gconf_client_set_string (ebook->client, ebook->paper_size.key, "B5", NULL);
		return;
	}
	a4state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (a4));
	if (a4state)
		gconf_client_set_string (ebook->client, ebook->paper_size.key, "A4", NULL);
}

static void
page_check_cb (GtkWidget *w, gpointer data)
{
	GtkWidget * check;
	Ebook * ebook;
	gboolean state;

	ebook = (Ebook *)data;
	check = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "pagecheckbutton"));
	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check));
	gconf_client_set_bool (ebook->client, ebook->page_number.key, state, NULL);
}

static void
line_check_cb (GtkWidget *w, gpointer data)
{
	GtkWidget * check;
	gboolean state;
	Ebook * ebook;

	ebook = (Ebook *)data;
	check = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "linecheckbutton"));
	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check));
	gconf_client_set_bool (ebook->client, ebook->long_lines.key, state, NULL);
}

static void
hyphen_check_cb (GtkWidget *w, gpointer data)
{
	GtkWidget * check;
	Ebook * ebook;
	gboolean state;

	ebook = (Ebook *)data;
	check = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "hyphencheckbutton"));
	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check));
	gconf_client_set_bool (ebook->client, ebook->join_hyphens.key, state, NULL);
}

/**
 GConf will make these effective immediately, hence no
 need to set or allow the user to "cancel" pref settings. 
*/
static void
pref_cb (GtkWidget *menu, gpointer data)
{
	static GtkWidget *dialog;
	GtkWidget * window, * a5, *b5, *a4, * pages, * lines, *hyphens;
	GtkWidget * fontbut, * buffer;
	GdkPixbuf *logo;
	gchar * path, * page_size;
	gboolean state;
	Ebook * ebook;

	if (dialog)
	{
		gtk_window_present (GTK_WINDOW (dialog));
		return;
	}
	ebook = (Ebook *)data;
	if (!ebook->builder)
		return;
	path = g_build_filename (DATADIR, "pixmaps", "gpdftext.png", NULL);
	logo = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);
	window = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	dialog = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "prefdialog"));
	a5 = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "a5radiobutton"));
	b5 = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "b5radiobutton"));
	a4 = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "a4radiobutton"));
	pages = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "pagecheckbutton"));
	lines = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "linecheckbutton"));
	hyphens = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "hyphencheckbutton"));
	fontbut = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "fontbutton"));
	buffer = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "texview"));
	gtk_window_set_icon (GTK_WINDOW(dialog), logo);
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
	/* set the widgets from the gconf data */
	page_size = gconf_client_get_string (ebook->client, ebook->paper_size.key, NULL);
	if (0 == g_strcmp0 (page_size, "B5"))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b5), TRUE);
	else if (0 == g_strcmp0 (page_size, "A4"))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (a4), TRUE);
	else /* A5 default */
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (a5), TRUE);

	state = gconf_client_get_bool (ebook->client, ebook->page_number.key, NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pages), state);

	state = gconf_client_get_bool (ebook->client, ebook->long_lines.key, NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pages), state);

	state = gconf_client_get_bool (ebook->client, ebook->join_hyphens.key, NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pages), state);

#ifdef HAVE_GTKSPELL
	setup_languages (ebook);
#endif
	g_signal_connect (G_OBJECT (dialog), "destroy",
			G_CALLBACK (gtk_widget_destroyed), &dialog);
	g_signal_connect (G_OBJECT (dialog), "response",
			G_CALLBACK (preferences_close_cb), ebook);
	g_signal_connect (G_OBJECT (a5), "toggled",
			G_CALLBACK (paper_radio_cb), ebook);
	g_signal_connect (G_OBJECT (b5), "toggled",
			G_CALLBACK (paper_radio_cb), ebook);
	g_signal_connect (G_OBJECT (a4), "toggled",
			G_CALLBACK (paper_radio_cb), ebook);
	g_signal_connect (G_OBJECT (pages), "toggled",
			G_CALLBACK (page_check_cb), ebook);
	g_signal_connect (G_OBJECT (lines), "toggled",
			G_CALLBACK (line_check_cb), ebook);
	g_signal_connect (G_OBJECT (hyphens), "toggled",
			G_CALLBACK (hyphen_check_cb), ebook);
	g_signal_connect (G_OBJECT (fontbut), "font-set",
			G_CALLBACK (editor_font_cb), ebook);
	gtk_widget_show_all (dialog);
}

static void
help_cb (GtkWidget *menu, gpointer data)
{
	g_app_info_launch_default_for_uri ("ghelp:" PACKAGE, NULL, NULL);
}

GtkWidget*
create_window (Ebook * ebook)
{
	GtkWidget *window, *open, *save, *cancel, *about, *newbtn;
	GtkWidget *pref_btn, *manualbtn, *langbox, * textview;
	GtkWidget *newmenu, *openmenu, *quitmenu, *savemenu, *spellmenu;
	GtkWidget *saveasmenu, *aboutmenu, *manualmenu, *prefmenu;
	GtkWidget *undobutton, *redobutton, *undomenu, *redomenu;
	GtkTextBuffer * buffer;
	GtkActionGroup  *action_group;
	GtkUIManager * uimanager;
	GdkPixbuf *logo;
	gchar *path;

	if (!ebook->builder)
		ebook->builder = load_builder_xml (NULL);
	if (!ebook->builder)
		return NULL;
	path = g_build_filename (DATADIR, "pixmaps", "gpdftext.png", NULL);
	logo = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);
	window = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	gtk_window_set_icon (GTK_WINDOW(window), logo);
	newbtn = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "new_button"));
	open = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "open_button"));
	save = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "save_button"));
	cancel = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "quit_button"));
	about = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "about_button"));
	pref_btn = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "pref_button"));
	manualbtn = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "help_button"));
	buffer = GTK_TEXT_BUFFER(gtk_builder_get_object (ebook->builder, "textbuffer1"));
	action_group = GTK_ACTION_GROUP(gtk_builder_get_object (ebook->builder, "actiongroup"));
	uimanager = GTK_UI_MANAGER(gtk_builder_get_object (ebook->builder, "uimanager"));
	aboutmenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "aboutmenuitem"));
	openmenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "openmenuitem"));
	newmenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "newmenuitem"));
	quitmenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "quitmenuitem"));
	savemenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "savemenuitem"));
	saveasmenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "saveasmenuitem"));
	manualmenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "manualmenuitem"));
	spellmenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "spellcheckmenuitem"));
	prefmenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "prefmenuitem"));
	langbox = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "langboxentry"));
	textview = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "textview"));
	undobutton = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undo_button"));
	redobutton = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "redo_button"));
	undomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "undomenuitem"));
	redomenu = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "redomenuitem"));
#ifndef HAVE_GTKSPELL
	gtk_widget_set_sensitive (spellmenu, FALSE);
	gtk_widget_set_sensitive (langbox, FALSE);
#else
	gtk_text_buffer_set_modified (buffer, FALSE);
	if (gconf_client_get_bool (ebook->client, ebook->spell_check.key, NULL))
	{
		const gchar *spell_lang;
		spell_lang = gconf_client_get_string (ebook->client, ebook->language.key, NULL);
		gtkspell_new_attach (GTK_TEXT_VIEW (textview), 
			(spell_lang == NULL || *spell_lang == '\0') ? NULL : spell_lang, NULL);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(spellmenu), TRUE);
	}
	editor_update_font (ebook);

#endif /* HAVE_GTKSPELL */

	gtk_widget_set_sensitive (savemenu, FALSE);
	gtk_widget_set_sensitive (save, FALSE);
	gtk_widget_set_sensitive (redobutton, FALSE);
	gtk_widget_set_sensitive (undobutton, FALSE);
	gtk_widget_set_sensitive (redomenu, FALSE);
	gtk_widget_set_sensitive (undomenu, FALSE);

	gtk_text_buffer_set_text (buffer, "", 0);
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_ui_manager_insert_action_group (uimanager, action_group, 0);
	g_signal_connect (G_OBJECT (newbtn), "clicked", 
			G_CALLBACK (new_pdf_cb), ebook);
	g_signal_connect (G_OBJECT (open), "clicked", 
			G_CALLBACK (open_pdf_cb), ebook);
	g_signal_connect (G_OBJECT (save), "clicked", 
			G_CALLBACK (save_txt_cb), ebook);
	g_signal_connect (G_OBJECT (cancel), "clicked", 
			G_CALLBACK (destroy_cb), ebook);
	g_signal_connect (G_OBJECT (about), "clicked", 
			G_CALLBACK (about_show), ebook);
	g_signal_connect (G_OBJECT (manualbtn), "clicked", 
			G_CALLBACK (help_cb), ebook);
	g_signal_connect (G_OBJECT (pref_btn), "clicked", 
			G_CALLBACK (pref_cb), ebook);
	g_signal_connect (G_OBJECT (undobutton), "clicked", 
			G_CALLBACK (undo_cb), ebook);
	g_signal_connect (G_OBJECT (redobutton), "clicked", 
			G_CALLBACK (redo_cb), ebook);
	g_signal_connect (G_OBJECT (undomenu), "activate", 
			G_CALLBACK (undo_cb), ebook);
	g_signal_connect (G_OBJECT (redomenu), "activate", 
			G_CALLBACK (redo_cb), ebook);
	g_signal_connect (G_OBJECT (window), "delete_event",
			G_CALLBACK (destroy_event_cb), ebook);
	g_signal_connect (G_OBJECT (aboutmenu), "activate",
			G_CALLBACK (about_show), ebook);
	g_signal_connect (G_OBJECT (openmenu), "activate",
			G_CALLBACK (open_pdf_cb), ebook);
	g_signal_connect (G_OBJECT (newmenu), "activate",
			G_CALLBACK (new_pdf_cb), ebook);
	g_signal_connect (G_OBJECT (prefmenu), "activate",
			G_CALLBACK (pref_cb), ebook);
	g_signal_connect (G_OBJECT (quitmenu), "activate",
			G_CALLBACK (destroy_cb), ebook);
	g_signal_connect (G_OBJECT (savemenu), "activate",
			G_CALLBACK (save_txt_cb), ebook);
	g_signal_connect (G_OBJECT (manualmenu), "activate",
			G_CALLBACK (help_cb), ebook);
	g_signal_connect (G_OBJECT (saveasmenu), "activate",
			G_CALLBACK (saveas_cb), ebook);
#ifdef HAVE_GTKSPELL
	g_signal_connect (G_OBJECT (spellmenu), "activate",
			G_CALLBACK (view_misspelled_words_cb), ebook);
#endif
	g_signal_connect (G_OBJECT (buffer), "modified-changed",
			G_CALLBACK (save_sensitive_cb), ebook);
	g_signal_connect (G_OBJECT (buffer), "delete_range",
			G_CALLBACK (delete_range_cb), ebook);
	g_signal_connect (G_OBJECT (buffer), "insert_text",
			G_CALLBACK (insert_text_cb), ebook);
	return window;
}

static void
font_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	GConfValue *value;
	const gchar *string;
	GtkWidget * editor;
	Ebook *ebook = (Ebook *) data;

	value = gconf_entry_get_value (entry);
	string = gconf_value_get_string (value);
	editor = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "textview"));
	editor_update_font (ebook);
}

static void
number_regexp_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	GConfValue *value;
	gboolean state;
	GtkWidget * pagecheck;
	Ebook *ebook = (Ebook *) data;

	value = gconf_entry_get_value (entry);
	state = gconf_value_get_bool (value);
	pagecheck = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "pagecheckbutton"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pagecheck), state);
}

static void
lines_regexp_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	GConfValue *value;
	gboolean state;
	GtkWidget * linecheck;
	Ebook *ebook = (Ebook *) data;

	value = gconf_entry_get_value (entry);
	state = gconf_value_get_bool (value);
	linecheck = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "linecheckbutton"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (linecheck), state);
}

static void
hyphens_regexp_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	GConfValue *value;
	gboolean state;
	GtkWidget * hyphens;
	Ebook *ebook = (Ebook *) data;

	value = gconf_entry_get_value (entry);
	state = gconf_value_get_bool (value);
	hyphens = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "hyphencheckbutton"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (hyphens), state);
}

static void
paper_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer data)
{
	GConfValue *value;
	GtkWidget * paper;
	const gchar *string;
	Ebook *ebook = (Ebook *) data;

	value = gconf_entry_get_value (entry);
	string = gconf_value_get_string (value);

	/* A5 is the default */
	paper = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "a5radiobutton"));
	if (0 == g_strcmp0 (string, "B5"))
		paper = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "b5radiobutton"));
	if (0 == g_strcmp0 (string, "A4"))
		paper = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "a4radiobutton"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (paper), TRUE);
}

void
gconf_data_fill (Ebook *ebook)
{
	gchar *base;
	GError * err;

	g_return_if_fail (ebook);
	base = g_strdup_printf ("/apps/%s", PACKAGE);
	err = NULL;
	ebook->paper_size.key = g_strdup_printf ("%s/paper_size", base);
	ebook->editor_font.key = g_strdup_printf ("%s/editor_font", base);
	ebook->spell_check.key = g_strdup_printf ("%s/spell_check", base);
	ebook->page_number.key = g_strdup_printf ("%s/page_number", base);
	ebook->long_lines.key = g_strdup_printf ("%s/long_lines", base);
	ebook->join_hyphens.key = g_strdup_printf ("%s/join_hyphens", base);
	ebook->language.key = g_strdup_printf ("%s/language", base);

	gconf_client_add_dir (ebook->client, base, GCONF_CLIENT_PRELOAD_NONE, &err);
	if (err)
		g_message ("%s", err->message);

	ebook->paper_size.id = gconf_client_notify_add (ebook->client,
		ebook->paper_size.key, paper_changed_cb, ebook, NULL, &err);
	if (err)
		g_message ("%s", err->message);
	ebook->editor_font.id = gconf_client_notify_add (ebook->client,
		ebook->editor_font.key, font_changed_cb, ebook, NULL, &err);
	if (err)
		g_message ("%s", err->message);
	ebook->spell_check.id = gconf_client_notify_add (ebook->client,
		ebook->spell_check.key, spellcheck_changed_cb, ebook, NULL, &err);
	if (err)
		g_message ("%s", err->message);
	ebook->page_number.id = gconf_client_notify_add (ebook->client,
		ebook->page_number.key, number_regexp_changed_cb, ebook, NULL, &err);
	if (err)
		g_message ("%s", err->message);
	ebook->long_lines.id = gconf_client_notify_add (ebook->client,
		ebook->long_lines.key, lines_regexp_changed_cb, ebook, NULL, &err);
	if (err)
		g_message ("%s", err->message);
	ebook->join_hyphens.id = gconf_client_notify_add (ebook->client,
		ebook->join_hyphens.key, hyphens_regexp_changed_cb, ebook, NULL, &err);
	if (err)
		g_message ("%s", err->message);
	ebook->editor_font.id = gconf_client_notify_add (ebook->client,
		ebook->editor_font.key, editor_font_changed_cb, ebook, NULL, &err);
	if (err)
		g_message ("%s", err->message);
#ifdef HAVE_GTKSPELL
	ebook->language.id = gconf_client_notify_add (ebook->client,
		ebook->language.key, spell_language_changed_cb, ebook, NULL, &err);
	if (err)
		g_message ("%s", err->message);
#endif
	g_free (base);
}
