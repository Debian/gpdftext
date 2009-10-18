/*************************************************************************
 *            spell.c
 *
 *  Sun Oct 18 11:41:57 2009
 *  Copyright  2009  Neil Williams
 *  <linux@codehelp.co.uk>
 ************************************************************************/

/*
 * spell.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * spell.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include "ebookui.h"
#include "spell.h"

/** spell checking and editor font functionality */

/* realname, the name that aspell understands, i.e. en_US, en_GB etc.
   label, the name displayed to the user, fallbacks to realname */
typedef struct _sl
{
	gchar *realname;
	gchar *label;
} eSpellLanguage;

void
view_misspelled_words_cb (GtkWidget *w, gpointer data)
{
	GtkWidget * spell_menu;
	gboolean state;
	Ebook *ebook = (Ebook *)data;

	spell_menu = GTK_WIDGET(gtk_builder_get_object (ebook->builder,"spellcheckmenuitem"));
	state = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (spell_menu));
	gconf_client_set_bool (ebook->client, ebook->spell_check.key, state, NULL);
}

void
spell_language_select_menuitem (Ebook *ebook, const gchar *lang)
{
#ifdef HAVE_GTKSPELL
	GtkComboBox *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *tmp_lang;
	gint i = 0, found = -1;

	combo = GTK_COMBO_BOX(gtk_builder_get_object (ebook->builder, "langboxentry"));
	if (lang == NULL)
	{
		gtk_combo_box_set_active (combo, 0);
		return;
	}
	model = gtk_combo_box_get_model (combo);

	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;

	do
	{
		gtk_tree_model_get (model, &iter, 0, &tmp_lang, -1);
		if (g_str_equal (tmp_lang, lang))
			found = i;
		g_free (tmp_lang);
		i++;
	} while (gtk_tree_model_iter_next (model, &iter) && found < 0);


	if (found >= 0)
		gtk_combo_box_set_active (combo, found);
	else
		g_warning (_("Language '%s' from GConf isn't in the list of available languages\n"), lang);

#endif /* HAVE_GTKSPELL */
}

void
spell_language_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer user_data)
{
#ifdef HAVE_GTKSPELL
	Ebook *ebook;
	GConfValue *value;
	GtkSpell *spell;
	const gchar *gconf_lang;
	gchar *lang;
	GtkWidget * window, * spell_check;
	GtkTextView * text_view;
	gboolean spellcheck_wanted;

	g_return_if_fail (user_data);

	ebook = (Ebook *) user_data;
	value = gconf_entry_get_value (entry);
	gconf_lang = gconf_value_get_string (value);
	window = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "gpdfwindow"));
	spell_check = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "spellcheckmenuitem"));
	text_view = GTK_TEXT_VIEW(gtk_builder_get_object (ebook->builder, "textview"));

	if (*gconf_lang == '\0' || gconf_lang == NULL)
		lang = NULL;
	else
		lang = g_strdup (gconf_lang);

	if (window)
	{
		spellcheck_wanted = gconf_client_get_bool (ebook->client, ebook->spell_check.key, NULL);
		spell = gtkspell_get_from_text_view (text_view);

		if (spellcheck_wanted)
		{
			if (spell && lang)
				/* Only if we have both spell and lang non-null we can use _set_language() */
				gtkspell_set_language (spell, lang, NULL);
			else
			{
				/* We need to create a new spell widget if we want to use lang == NULL (use default lang)
				 * or if the spell isn't initialized */
				if (spell)
					gtkspell_detach (spell);
				spell = gtkspell_new_attach (text_view, lang, NULL);
			}
			gtkspell_recheck_all (spell);

		}
	}

	spell_language_select_menuitem (ebook, lang);

	g_free (lang);
#endif /* HAVE_GTKSPELL */
}

void
spellcheck_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer user_data)
{
#ifdef HAVE_GTKSPELL
	GConfValue *value;
	GtkSpell *spell;
	GtkWidget * dict, * spell_check;
	GtkTextView * text_view;
	gboolean state;
	gchar *lang;
	Ebook *ebook;

	ebook = (Ebook *) user_data;
	value = gconf_entry_get_value (entry);
	state = gconf_value_get_bool (value);
	dict = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "langboxentry"));

	gtk_widget_set_sensitive (dict, state);

	text_view = GTK_TEXT_VIEW(gtk_builder_get_object (ebook->builder, "textview"));
	spell_check = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "spellcheckmenuitem"));
	spell = gtkspell_get_from_text_view (text_view);
	lang = gconf_client_get_string (ebook->client, ebook->language.key, NULL);

	if (state)
	{
		if (!spell)
			gtkspell_new_attach (text_view,
					     (lang == NULL || *lang == '\0') ? NULL : lang,
					     NULL);
	}
	else
	{
		if (spell)
			gtkspell_detach (spell);
	}

	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (spell_check), state);

#endif /* HAVE_GTKSPELL */
}

static G_GNUC_UNUSED GSList *
get_available_spell_languages (void)
{
#ifdef HAVE_GTKSPELL
	GSList *rv = NULL;
	gchar *prgm, *cmd;
	gchar *prgm_err, *prgm_out;
	gint exit_status, i;
	GError *err = NULL;
	eSpellLanguage *lang;
	gchar **lang_arr;

	if ((prgm = g_find_program_in_path ("aspell")) == NULL)
		return NULL;
	cmd = g_strdup_printf ("%s dump dicts", prgm);
	g_spawn_command_line_sync (cmd, &prgm_out, &prgm_err, &exit_status, &err);
	g_free (cmd);
	g_free (prgm);
	if (err)
	{
		g_warning ("Failed to get language list: %s", err->message);
		g_error_free (err);
		return NULL;
	}
	if (exit_status != 0)
	{
		g_warning ("Failed to get language list, program output was: %s", prgm_err);
		g_free (prgm_out);
		g_free (prgm_err);
		return NULL;
	}
	lang_arr = g_strsplit (prgm_out, "\n", -1);

	i = 0;
	while (lang_arr[i])
	{
		g_strstrip (lang_arr[i]);
		if (*(lang_arr[i]) != '\0')
		{
			lang = g_new0 (eSpellLanguage, 1);
			/* For now, set realname == label */
			lang->realname = g_strdup (lang_arr[i]);
			lang->label = g_strdup (lang_arr[i]);
//			rv = g_slist_insert_sorted (rv, lang, (GCompareFunc) du_str_cmp);
			rv = g_slist_insert_sorted (rv, lang, (GCompareFunc) g_str_equal);
		}
		i++;
	}
	g_strfreev (lang_arr);

	lang = g_new0 (eSpellLanguage, 1);
	/* Context: Spell check dictionary */
	lang->label = g_strdup (_("System default"));
	lang->realname = g_strdup ("");
	rv = g_slist_prepend (rv, lang);

	return rv;
#else
	return NULL;
#endif /* HAVE_GTKSPELL */
}

static G_GNUC_UNUSED gchar *
get_default_text (GConfClient *client, const gchar *key, const gchar *standard_text)
{
	gchar *string;

	string = gconf_client_get_string (client, key, NULL);

	if (!string)
		string = g_strdup (standard_text);

	return string;
}

static G_GNUC_UNUSED void
spellcheck_language_cb (GtkComboBox *combobox, gpointer user_data)
{
#ifdef HAVE_GTKSPELL
	gchar *language;
	Ebook *ebook = (Ebook *) user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint sel;

	model = gtk_combo_box_get_model (combobox);
	sel = gtk_combo_box_get_active (combobox);

	if (gtk_tree_model_iter_nth_child (model, &iter, NULL, sel))
	{
		gtk_tree_model_get (model, &iter, 0, &language, -1);
		gconf_client_set_string (ebook->client, ebook->language.key, language, NULL);
		g_free (language);
	}
#endif /* HAVE_GTKSPELL */
}

static G_GNUC_UNUSED void
free_store (GtkWidget *widget, GObject *store)
{
	g_object_unref (store);
}

void
setup_languages (Ebook * ebook)
{
#ifdef HAVE_GTKSPELL
	GtkTreeIter iter;
	GtkListStore *language_store;
	GtkComboBox * combo;
	GtkWidget * pref_window;
	GSList *list;
	gchar *string;
	GtkCellRenderer *renderer;
	gint sel;

	/* spellcheck dictionaries */

	sel = -1;
	string = get_default_text (ebook->client, ebook->language.key, "");
	language_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	list = get_available_spell_languages ();
	combo = GTK_COMBO_BOX(gtk_builder_get_object (ebook->builder, "langboxentry"));
	gtk_combo_box_set_model (combo, GTK_TREE_MODEL (language_store));
	if (list)
	{
		eSpellLanguage *lang_struct;
		gint i;
		GSList *head = list;

		for (i = 0, list = head; list != NULL; list = g_slist_next (list), i++)
		{
			lang_struct = list->data;
			gtk_list_store_append (language_store, &iter);
			gtk_list_store_set (language_store, &iter,
					    0, lang_struct->realname,
					    1, lang_struct->label,
					    -1);
			if (!g_strcmp0 (lang_struct->realname, string))
			{
				gtk_combo_box_set_active_iter (combo, &iter);
				sel = i;
			}
			g_free (lang_struct->realname);
			g_free (lang_struct->label);
		}
		g_slist_free (head);
	}
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
					"text", 1,
					NULL);
	if (sel < 0)
	{
		if (!g_strcmp0 (string, ""))
			g_warning (_("Language '%s' from GConf isn't in the list of available languages\n"), string);
		sel = 0;
	}

	gtk_combo_box_set_active (combo, sel);

	pref_window = GTK_WIDGET(gtk_builder_get_object (ebook->builder, "prefdialog"));
	g_signal_connect (G_OBJECT (combo), "changed",
			  G_CALLBACK (spellcheck_language_cb), ebook);
	g_signal_connect (G_OBJECT (pref_window), "destroy",
			  G_CALLBACK (free_store), G_OBJECT (language_store));
#endif /* HAVE_GTKSPELL */
}

static G_GNUC_UNUSED void
editor_set_font (GtkWidget   * textview, const gchar *font_name)
{
	PangoFontDescription *font_desc = NULL;

	g_return_if_fail (font_name != NULL);
	font_desc = pango_font_description_from_string (font_name);
	g_return_if_fail (font_desc != NULL);
	gtk_widget_modify_font (GTK_WIDGET (textview), font_desc);
	pango_font_description_free (font_desc);
}

void
editor_update_font(Ebook * ebook)
{
	GtkTextView * textview;
	gchar *editor_font;

	editor_font = gconf_client_get_string(ebook->client, ebook->editor_font.key, NULL);
	textview = GTK_TEXT_VIEW(gtk_builder_get_object (ebook->builder, "textview"));
	editor_set_font( GTK_WIDGET(textview), 
			(editor_font == NULL || *editor_font=='\0') ? NULL : editor_font);
	g_free (editor_font);
}

void
editor_font_cb(GtkWidget *button, gpointer data)
{
	const gchar *font_name;
	Ebook *ebook = (Ebook *) data;

	font_name = gtk_font_button_get_font_name (GTK_FONT_BUTTON (button));
	if (!font_name)
	{
		g_warning ("Could not get font name");
		return;
	}
	gconf_client_set_string(ebook->client, ebook->editor_font.key, font_name, NULL);
}

void
editor_font_changed_cb (GConfClient *client, guint id, GConfEntry *entry, gpointer user_data)
{
	Ebook *ebook = (Ebook *) user_data;
	editor_update_font(ebook);
}
