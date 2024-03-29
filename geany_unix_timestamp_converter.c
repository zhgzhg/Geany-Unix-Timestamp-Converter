/*
 * geany_unix_timestamp_converter.c
 *
 * Geany plugin converting unix epoch timestamps to human-readable
 * strings, and their GPS timestamp equivalents.
 *
 * Copyright 2022 zhgzhg @ github.com
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h> /* for the key bindings */

#ifdef HAVE_LOCALE_H
	#include <locale.h>
#endif

GeanyPlugin *geany_plugin;
struct GeanyKeyGroup *geany_key_group;
GeanyData *geany_data;

static gchar *plugin_config_path = NULL;
static GKeyFile *keyfile_plugin = NULL;

PLUGIN_VERSION_CHECK(231)

PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR,
	GETTEXT_PACKAGE,
	_("Unix Timestamp Converter"),

	_("Unix timestamp to readable string converter.\n\
Converts the value in selection or in the clipboard to a readable \
string.\nhttps://github.com/zhgzhg/Geany-Unix-Timestamp-Converter"),

	"1.5.1",

	"zhgzhg @@ github.com\n\
https://github.com/zhgzhg/Geany-Unix-Timestamp-Converter"
);

static GtkWidget *main_menu_item = NULL;
static GtkWidget *result_in_msgwin_btn = NULL;
static GtkWidget *show_failure_msgs_btn = NULL;
static GtkWidget *work_with_clipbrd_btn = NULL;
static GtkWidget *autodetect_timestamp_in_milliseconds_btn = NULL;


static gboolean showResultInMsgPopupWindow = TRUE;
static gboolean showErrors = FALSE;
static gboolean useClipboard = TRUE;
static gboolean autodetectTimestampInMsAndUs = TRUE;

static time_t unixTs2GPSTs(time_t unixTs, gboolean calcLeapSeconds)
{
	static const time_t GPS_LEAPS[] = {
		46828800, 78364801, 109900802, 173059203, 252028804, 315187205,
		346723206, 393984007, 425520008, 457056009, 504489610,
		551750411, 599184012, 820108813, 914803214, 1025136015,
		1119744016, 1167264017
	};
	time_t gpsTs = unixTs - 315964800;
	unsigned char leapSecondsPassed = 0, isLeap = 0, ctr;
	
	if (!calcLeapSeconds) return gpsTs;

	for (ctr = 0; ctr < sizeof(GPS_LEAPS) / sizeof(GPS_LEAPS[0]); ++ctr)
	{
		if (gpsTs + ctr >= GPS_LEAPS[ctr])
		{ ++leapSecondsPassed; }
	}

	return gpsTs + leapSecondsPassed;
}

static void receiveAndConvertData(GtkClipboard *clipboard,
									const gchar *text,
									gpointer document)
{
	const gchar *noDataMsg = "No data to convert!";
	const gchar *noNumDataMsg = "No numerical data %s was recognized!";
	const gchar *tsConvFailMsg = "Conversion of %d timestamp failed!";

	int r = 0;
	gchar output[81] = "\0";
	gchar finalOutputUtc[91] = "\0";
	gchar finalOutputLocal[91] = "\0";
	gchar finalOutputGPSTs[71] = "\0";
	gchar finalOutput[251] = "\0";
	unsigned long long timestamp = 0;
	unsigned long remainder = 0;
	time_t realTimestamp;

	struct tm *timeinfo;

	if (text != NULL)
	{
		r = sscanf(text, "%llu.%lu", &timestamp, &remainder);
		if (r == 0)
		{
			if (showErrors)
			{
				if (0 <= snprintf(output, sizeof(output), noNumDataMsg,
						timestamp))
					strcpy(output, "No numerical data was recognized!");

				msgwin_msg_add(COLOR_RED, -1, (GeanyDocument*) document,
								"%s", output);

				if (showResultInMsgPopupWindow)
					dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s", output);
			}
			return;
		}

		if (autodetectTimestampInMsAndUs && remainder == 0)
		{
			if (timestamp > 9999999999999ULL)
			{
				remainder = (timestamp/1000) % 1000;
				timestamp /= 1000000;
			}
			else if (timestamp > 9999999999ULL)
			{
				remainder = timestamp % 1000;
				timestamp /= 1000;
			}
		}

		realTimestamp = (time_t) timestamp;

		timeinfo = gmtime((const time_t*) &realTimestamp);
		while (remainder > 1000) remainder /= 1000;

		if (strftime(output, sizeof(output),
				"%a %d %b %Y %T.%%03u %p %Z", timeinfo) != 0)
		{
			snprintf(finalOutputUtc, sizeof(finalOutputUtc), output,
					 remainder);

			timeinfo = localtime(&realTimestamp);
			strftime(output, sizeof(output),
				"\n%a %d %b %Y %T.%%03u %p (%Z) %z", timeinfo);

			snprintf(finalOutputLocal, sizeof(finalOutputLocal), output,
					 remainder);

			snprintf(finalOutputGPSTs, sizeof(finalOutputGPSTs),
					"\nGPS Timestamp Equivalent: %ld; Without leaps: %ld",
					unixTs2GPSTs(realTimestamp, TRUE),
					unixTs2GPSTs(realTimestamp, FALSE)
			);

			strcpy(finalOutput, finalOutputUtc);
			strcat(finalOutput, finalOutputLocal);
			if (timeinfo->tm_isdst) strcat(finalOutput, " DST");
			strcat(finalOutput, finalOutputGPSTs);

			msgwin_msg_add(COLOR_BLUE, -1, (GeanyDocument*) document,
							"%llu.%lu is equal to\n%s", timestamp,
							remainder, finalOutput);

			if (showResultInMsgPopupWindow)
				dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s",
									finalOutput);
		}
		else
		{
			if (showErrors)
			{
				msgwin_msg_add(COLOR_RED, -1,
								(GeanyDocument*) document,
								tsConvFailMsg, timestamp);

				if (showResultInMsgPopupWindow)
				{
					if (0 > snprintf(output, sizeof(output), noDataMsg,
						timestamp))
						dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s",
											output);
					else
						dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s",
											noDataMsg);
				}
			}
		}
	}
	else
	{
		if (showErrors)
		{
			msgwin_msg_add(COLOR_RED, -1, (GeanyDocument*) document,
							"%s", noDataMsg);

			if (showResultInMsgPopupWindow)
				dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s", noDataMsg);
		}
	}
}

static void unixts_to_string(GeanyDocument *doc)
{
	const gchar *noDataMsg = "No data to convert!";
	GtkClipboard *clipboard = NULL;
	gchar *selectedText;

	/* use the text in the clipboard */

	if (!doc || !sci_has_selection(doc->editor->sci))
	{
		if (useClipboard)
		{
			clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

			if (clipboard)
			{
				gtk_clipboard_request_text(clipboard,
				(GtkClipboardTextReceivedFunc) receiveAndConvertData,
					doc);
				return;
			}
		}

		if (showErrors)
		{
			msgwin_msg_add(COLOR_RED, -1, doc, "%s", noDataMsg);

			if (showResultInMsgPopupWindow)
				dialogs_show_msgbox(GTK_MESSAGE_INFO, "%s", noDataMsg);
		}
	}
	else /* work with selected text */
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


static void kb_run_unix_ts_converter(G_GNUC_UNUSED guint key_id)
{
	unixts_to_string(document_get_current());
}


static void config_save_setting(GKeyFile *keyfile, const gchar *filePath)
{
	if (keyfile && filePath)
		g_key_file_save_to_file(keyfile, filePath, NULL);
}


static gboolean config_get_setting(GKeyFile *keyfile, const gchar *name)
{
	if (keyfile)
		return g_key_file_get_boolean(keyfile, "settings", name, NULL);

	return FALSE;
}


static void config_set_setting(GKeyFile *keyfile, const gchar *name,
								gboolean value)
{
	if (keyfile)
		g_key_file_set_boolean(keyfile, "settings", name, value);
}


static void on_configure_response(GtkDialog* dialog, gint response,
									gpointer user_data)
{
	gboolean value = FALSE;

	if (keyfile_plugin &&
		(response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY))
	{
		value = gtk_toggle_button_get_active(
							GTK_TOGGLE_BUTTON(result_in_msgwin_btn));
		showResultInMsgPopupWindow = value;
		config_set_setting(keyfile_plugin,
							"show_result_in_message_window", value);


		value = gtk_toggle_button_get_active(
							GTK_TOGGLE_BUTTON(show_failure_msgs_btn));
		showErrors = value;
		config_set_setting(keyfile_plugin, "show_failure_messages",
							value);


		value = gtk_toggle_button_get_active(
							GTK_TOGGLE_BUTTON(work_with_clipbrd_btn));
		useClipboard = value;
		config_set_setting(keyfile_plugin, "use_clipboard_too", value);


		value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				autodetect_timestamp_in_milliseconds_btn));
		autodetectTimestampInMsAndUs = value;
		config_set_setting(keyfile_plugin,
							"autodetect_timestamp_in_ms_us", value);

		config_save_setting(keyfile_plugin, plugin_config_path);
	}
}

static void config_set_defaults(GKeyFile *keyfile)
{
	if (!keyfile) return;
	config_set_setting(keyfile,	"show_result_in_message_window", TRUE);
	showResultInMsgPopupWindow = TRUE;
	config_set_setting(keyfile, "show_failure_messages", FALSE);
	showErrors = FALSE;
	config_set_setting(keyfile, "use_clipboard_too", TRUE);
	useClipboard = TRUE;
	config_set_setting(keyfile, "autodetect_timestamp_in_ms_us", TRUE);
	autodetectTimestampInMsAndUs = TRUE;
}


void plugin_init(GeanyData *data)
{
	/* read & prepare configuration */
	gchar *config_dir = g_build_path(G_DIR_SEPARATOR_S,
		geany_data->app->configdir, "plugins", "unixtsconverter", NULL);
	plugin_config_path = g_build_path(G_DIR_SEPARATOR_S, config_dir,
										"unixtsconverter.conf", NULL);

	g_mkdir_with_parents(config_dir, S_IRUSR | S_IWUSR | S_IXUSR);
	g_free(config_dir);

	keyfile_plugin = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile_plugin, plugin_config_path,
									G_KEY_FILE_NONE, NULL))
	{
		config_set_defaults(keyfile_plugin);
		config_save_setting(keyfile_plugin, plugin_config_path);
	}
	else
	{
		showResultInMsgPopupWindow = config_get_setting(keyfile_plugin,
									"show_result_in_message_window");

		showErrors = config_get_setting(keyfile_plugin,
									"show_failure_messages");

		useClipboard = config_get_setting(keyfile_plugin,
									"use_clipboard_too");

		autodetectTimestampInMsAndUs = config_get_setting(
				keyfile_plugin,	"autodetect_timestamp_in_ms_us");
	}

	/* ---------------------------- */

	main_menu_item = gtk_menu_item_new_with_mnemonic(
											"Unix Timestamp Converter");
	gtk_widget_show(main_menu_item);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
						main_menu_item);
	g_signal_connect(main_menu_item, "activate",
						G_CALLBACK(item_activate_cb), NULL);

	/* Register shortcut key group */
	geany_key_group = plugin_set_key_group(
						geany_plugin, _("unix_ts_converter"), 1, NULL);

	/* Ctrl + Alt + c */
	keybindings_set_item(geany_key_group, 0, kb_run_unix_ts_converter,
                         GDK_c, GDK_CONTROL_MASK | GDK_MOD1_MASK,
                         "run_unix_ts_converter",
                         _("Run the Unix Timestamp Converter"),
                         main_menu_item);
}


GtkWidget *plugin_configure(GtkDialog *dialog)
{
#if !(GTK_CHECK_VERSION(3, 2, 0))

	GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
	GtkWidget *_hbox1 = gtk_hbox_new(FALSE, 6);
	GtkWidget *_hbox2 = gtk_hbox_new(FALSE, 6);
	GtkWidget *_hbox3 = gtk_hbox_new(FALSE, 6);
	GtkWidget *_hbox4 = gtk_hbox_new(FALSE, 6);

#else

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	GtkWidget *_hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *_hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *_hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *_hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

#endif

	result_in_msgwin_btn = gtk_check_button_new_with_label(
		_("Show the converted output in a message window."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(result_in_msgwin_btn),
		config_get_setting(keyfile_plugin,
							"show_result_in_message_window"));


	show_failure_msgs_btn = gtk_check_button_new_with_label(
		_("Show the failure messages."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(show_failure_msgs_btn),
		config_get_setting(keyfile_plugin, "show_failure_messages"));


	work_with_clipbrd_btn = gtk_check_button_new_with_label(
		_("Work with the clipboard if no text was selected."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(work_with_clipbrd_btn),
		config_get_setting(keyfile_plugin, "use_clipboard_too"));


	autodetect_timestamp_in_milliseconds_btn =
			gtk_check_button_new_with_label(
				_("Automatically detect timestamps in milliseconds and "
				"microseconds."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(autodetect_timestamp_in_milliseconds_btn),
		config_get_setting(keyfile_plugin,
			"autodetect_timestamp_in_ms_us"));


	gtk_box_pack_start(GTK_BOX(_hbox1), result_in_msgwin_btn, TRUE,
						TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_hbox2), show_failure_msgs_btn, TRUE,
						TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_hbox3), work_with_clipbrd_btn, TRUE,
						TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_hbox4),
			autodetect_timestamp_in_milliseconds_btn, TRUE,	TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), _hbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _hbox2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _hbox3, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _hbox4, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	g_signal_connect(dialog, "response",
						G_CALLBACK(on_configure_response), NULL);

	return vbox;
}


void plugin_cleanup(void)
{
	g_free(plugin_config_path);
	g_key_file_free(keyfile_plugin);
	gtk_widget_destroy(main_menu_item);
}
