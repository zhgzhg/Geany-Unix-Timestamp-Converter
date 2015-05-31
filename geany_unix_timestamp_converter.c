/*
 * geany_unix_timestamp_converter.c - a Geany plugin to convert unix
 *                                    timestamps to a readable string
 *
 *  Copyright 2015 zhgzhg @ github.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <geanyplugin.h>

#ifdef HAVE_LOCALE_H
	#include <locale.h>
#endif

GeanyPlugin *geany_plugin;
GeanyData *geany_data;
GeanyFunctions *geany_functions;

PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR,
	GETTEXT_PACKAGE,
	_("Unix Timestamp Converter"),

	_("Unix timestamp to readable string converter.\n\
Converts the value in the clipboard to a readable string.\n\
https://github.com/zhgzhg/Geany-Unix-Timestamp-Converter"),

	"1.0",

	"zhgzhg @@ github.com\n\
https://github.com/zhgzhg/Geany-Unix-Timestamp-Converter"
);

static GtkWidget *main_menu_item = NULL;
static GtkWidget *result_in_msgwin_btn = NULL;
static GtkWidget *show_failure_msgs_btn = NULL;
static GtkWidget *work_with_clipbrd_btn = NULL;

static void receiveAndConvertData(GtkClipboard *clipboard,
									const gchar *text,
									gpointer document)
{
	gchar output[80] = "\0";
	struct tm *timeinfo;
	time_t timestamp = 0;
	int r = 0;
	
	if (text != NULL)
	{
		r = sscanf(text, "%d.", &timestamp);
		if (r == 0)
			return;
			
		timeinfo = gmtime((const time_t*)&timestamp);
		
		if (strftime(output, sizeof(output), "%c", timeinfo))
		{
			msgwin_msg_add(COLOR_BLUE, -1, (GeanyDocument*) document,
			"%d is equal to %s", timestamp, output);
			
			dialogs_show_msgbox(GTK_MESSAGE_INFO, 
								(const gchar*) output);
		}
	}
}

static void unixts_to_string(GeanyDocument *doc)
{
	GtkClipboard *clipboard = NULL;
	gchar *selectedText;
	
	if (!sci_has_selection(doc->editor->sci)) // use the text in the clipboard
	{
		clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	
		if (clipboard)
			gtk_clipboard_request_text(clipboard,
				(GtkClipboardTextReceivedFunc) receiveAndConvertData,
					doc);
	}
	else // work with selected text
	{
		selectedText = sci_get_selection_contents(doc->editor->sci);	
		receiveAndConvertData(NULL, selectedText, doc);
		g_free(selectedText);	
	}					
}


/* Geany plugin EP code */

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata)
{
	unixts_to_string(document_get_current());
}


static void on_configure_response(GtkDialog* dialog, gint response, 
									gpointer user_data)
{
}


void plugin_init(GeanyData *data)
{
	main_menu_item = gtk_menu_item_new_with_mnemonic("Unix Timestamp Converter");
	gtk_widget_show(main_menu_item);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
						main_menu_item);
	g_signal_connect(main_menu_item, "activate",
						G_CALLBACK(item_activate_cb), NULL);
}


GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
	GtkWidget *_hbox1 = gtk_hbox_new(FALSE, 6);
	GtkWidget *_hbox2 = gtk_hbox_new(FALSE, 6);
	GtkWidget *_hbox3 = gtk_hbox_new(FALSE, 6);
	
	result_in_msgwin_btn = gtk_check_button_new_with_label(
		_("Show the converted output in a message window."));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(result_in_msgwin_btn),
									TRUE);
	
	show_failure_msgs_btn = gtk_check_button_new_with_label(
		_("Show the failure messages."));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_failure_msgs_btn),
									FALSE);
	
	work_with_clipbrd_btn = gtk_check_button_new_with_label(
		_("Work with the clipboard if no text was selected."));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(work_with_clipbrd_btn),
									TRUE);

	gtk_box_pack_start(GTK_BOX(_hbox1), result_in_msgwin_btn, TRUE,
						TRUE, 0);	
	gtk_box_pack_start(GTK_BOX(_hbox2), show_failure_msgs_btn, TRUE,
						TRUE, 0);	
	gtk_box_pack_start(GTK_BOX(_hbox3), work_with_clipbrd_btn, TRUE,
						TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), _hbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _hbox2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _hbox3, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	g_signal_connect(dialog, "response", 
						G_CALLBACK(on_configure_response), NULL);

	return vbox;
}


void plugin_cleanup(void)
{
	gtk_widget_destroy(main_menu_item);
}
