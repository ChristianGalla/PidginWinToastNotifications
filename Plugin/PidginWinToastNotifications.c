#define PURPLE_PLUGINS

#include <glib.h>
#include "string.h"

#include "gtkplugin.h"
#include "gtkconv.h"
#include "gtkconvwin.h"
#include "account.h"
#include "debug.h"
#include "signals.h"
#include "plugin.h"
#include "version.h"
#include "blist.h"

typedef int(__cdecl *initProc)(void (*clickCallback)(void *conv));
typedef int(__cdecl *showToastProc)(const char *sender, const char *message, const char *imagePath, const char *protocolName, void *conv);

const char *base_settings_path = "/plugins/gtk/gallax-win_toast_notifications";

struct charListNode {
    char *str;
	struct charListNode *next;
};

struct localSettingsData {
    struct charListNode *paths;
};

struct status_options_paths {
	const char * parent;
	const char * enabled;
	const char * for_im;
	const char * for_chat;
	const char * for_chat_mentioned;
	const char * for_focus;
};

const struct status_options_paths labels = {
	NULL,
	"Enabled",
	"Direct messages",
	"Every message in chats",
	"Messages in chats when mentioned",
	"Even if the window is focused"
};

HINSTANCE hinstLib;
initProc initAdd;
showToastProc showToastProcAdd;

typedef enum {SETTING_NONE, SETTING_ENABLED, SETTING_FOR_IM, SETTING_FOR_CHAT, SETTING_FOR_CHAT_MENTIONED, SETTING_FOR_FOCUS} Setting;
typedef enum {BUDDY_TYPE_GLOBAL, BUDDY_TYPE_BUDDY, BUDDY_TYPE_CHAT} Buddy_type;

static void output_toast_error(int errorNumber, char *message);
static char* get_attr_text(const char *protocolName, const char *userName, const char *chatName);
static void toast_clicked_cb(PurpleConversation *conv);
static gboolean should_show(PurpleAccount *account, PurpleConversation *conv, PurpleConversationType convType, PurpleMessageFlags flags, const char *sender);
static void displayed_msg_cb(PurpleAccount *account, char *sender, char *buffer, PurpleConversation *conv, PurpleMessageFlags flags);
static void button_clicked_cb(GtkButton *button, char *path);
static void settings_dialog_destroy_cb(GtkWidget *w, struct localSettingsData *data);
static void local_settings_dialog_response_cb(GtkWidget *dialog, gint resp, struct localSettingsData *data);
static void show_local_settings_dialog(PurpleBlistNode *node, gpointer plugin);
static void context_menu(PurpleBlistNode *node, GList **menu, gpointer plugin);
static gboolean plugin_load(PurplePlugin *plugin);
static gboolean plugin_unload(PurplePlugin *plugin);
static void init_plugin(PurplePlugin *plugin);
static char* get_prefs_path(PurpleStatusPrimitive status, Setting setting, Buddy_type buddy_type, const char *protocol_id, const char *account_name, const char *buddy_name);
static const char* get_prefs_sub_path(Setting setting);
static const char* get_buddy_type_sub_path(Buddy_type buddy_type);
static void add_setting_button(
	PurpleStatusPrimitive status,
	Buddy_type buddy_type,
	char *protocol_id,
	char *account_name,
	char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data,
	Setting setting,
	char *labelText
);
static void add_setting_buttons(
	PurpleStatusPrimitive status,
	Buddy_type buddy_type,
	char *protocol_id,
	char *account_name,
	char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data,
	char *labelText
);
static void add_setting_groups(
	Buddy_type buddy_type,
	char *protocol_id,
	char *account_name,
	char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data
);
static void ensure_prefs_path(const char *path);
static void ensure_pref_default(PurpleStatusPrimitive status, Setting setting, gboolean value);
static void set_default_prefs();
static gboolean get_effective_setting(PurpleStatusPrimitive status, Setting setting, Buddy_type buddy_type, const char *protocol_id, const char *account_name, const char *buddy_name);
GtkWidget * get_config_frame(PurplePlugin *plugin);

static void ensure_prefs_path(const char *path) {
	char *subPath;
	int i = 0;
	gboolean exists;
	while (path[i] != '\0') {
		if (i > 0 && path[i] == '/') {
			subPath = (char *) malloc((i+1) * sizeof(char));
			memcpy(subPath, path, i); // copy everything except '/'
			subPath[i] = '\0';
			exists = purple_prefs_exists(subPath);
			if (!exists) {
				purple_prefs_add_none(subPath);
			}
			free(subPath);
		}
		i++;
	}
}

static const char* get_prefs_sub_path(Setting setting) {
	switch (setting) {
		case SETTING_ENABLED:
			return "/enabled";
		case SETTING_FOR_IM:
			return "/for_im";
		case SETTING_FOR_CHAT:
			return "/for_chat";
		case SETTING_FOR_CHAT_MENTIONED:
			return "/for_chat_mentioned";
		case SETTING_FOR_FOCUS:
			return "/for_focus";
		default:
			return "";
	}
}

static const char* get_buddy_type_sub_path(Buddy_type buddy_type) {
	switch (buddy_type) {
		case BUDDY_TYPE_GLOBAL:
			return "/global";
		case BUDDY_TYPE_BUDDY:
			return "/buddy";
		case BUDDY_TYPE_CHAT:
			return "/chat";
		default:
			return "/unknown";
	}
}

static char* get_prefs_path(PurpleStatusPrimitive status, Setting setting, Buddy_type buddy_type, const char *protocol_id, const char *account_name, const char *buddy_name) {
	// global example: "/plugins/gtk/gallax-win_toast_notifications/global/available/enabled"
	// buddy example: "/plugins/gtk/gallax-win_toast_notifications/buddy/<protocol_id>/<account_name>/<buddy_name>/available/enabled"
	// chat example: "/plugins/gtk/gallax-win_toast_notifications/chat/<protocol_id>/<account_name>/<chat_name>/available/enabled"
	char *ret = 0;
	const char *statusStr = 0;
	const char *settingStr = 0;
	const char *buddyTypeStr = 0;

	int size = 0;
	int basePathSize = 0;
	int protocolSize = 0;
	int accountSize = 0;
	int statusSize = 0;
	int settingSize = 0;
	int buddyTypeSize = 0;
	int buddyNameSize = 0;

	char * pch = 0;
	int targetPos = 0;

	statusStr = purple_primitive_get_id_from_type(status);
	settingStr = get_prefs_sub_path(setting);
	buddyTypeStr = get_buddy_type_sub_path(buddy_type);

	basePathSize = strlen(base_settings_path);
	buddyTypeSize = strlen(buddyTypeStr);
	statusSize = strlen(statusStr) + 1; // +1 for '/'
	settingSize = strlen(settingStr);
	if (buddy_type != BUDDY_TYPE_GLOBAL) {
		protocolSize = strlen(protocol_id) + 1;
		// ignore anything behind a '/', like a XMPP ressource
		accountSize = strlen(account_name);
		pch=strchr(account_name,'/');
		if (pch != NULL) {
			accountSize = pch - account_name;
		}
		accountSize++;
		buddyNameSize = strlen(buddy_name);
		pch=strchr(buddy_name,'/');
		if (pch != NULL) {
			buddyNameSize = pch - buddy_name;
		}
		buddyNameSize++;
	}

	size = basePathSize + statusSize + settingSize + protocolSize + buddyTypeSize + accountSize + buddyNameSize + 1;
	ret = (char *)malloc(size);
	memcpy(ret + targetPos, base_settings_path, basePathSize);
	targetPos += basePathSize;
	memcpy(ret + targetPos, buddyTypeStr, buddyTypeSize);
	targetPos += buddyTypeSize;
	if (buddy_type != BUDDY_TYPE_GLOBAL) {
		ret[targetPos] = '/';
		targetPos++;
		memcpy(ret + targetPos, protocol_id, protocolSize-1);
		targetPos += protocolSize-1;
		ret[targetPos] = '/';
		targetPos++;
		memcpy(ret + targetPos, account_name, accountSize-1);
		targetPos += accountSize-1;
		ret[targetPos] = '/';
		targetPos++;
		memcpy(ret + targetPos, buddy_name, buddyNameSize-1);
		targetPos += buddyNameSize-1;
	}
	ret[targetPos] = '/';
	targetPos++;
	memcpy(ret + targetPos, statusStr, statusSize-1);
	targetPos += statusSize-1;
	memcpy(ret + targetPos, settingStr, settingSize);
	targetPos += settingSize;
	ret[targetPos] = '\0';
	return ret;
}

GtkWidget * get_config_frame(PurplePlugin *plugin) {
	struct localSettingsData *data;
	GtkWidget *vbox;

	data = malloc(sizeof(struct localSettingsData));
	data->paths = 0;

	vbox = gtk_vbox_new(FALSE, 5);

	add_setting_groups(
		BUDDY_TYPE_GLOBAL,
		NULL,
		NULL,
		NULL,
		vbox,
		data
	);
	
	g_signal_connect(G_OBJECT(vbox), "destroy",
	    G_CALLBACK(settings_dialog_destroy_cb), data);

	gtk_widget_show_all(vbox);
	return vbox;
}

static PidginPluginUiInfo pidgin_plugin_info = {
	get_config_frame,
	0,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	PIDGIN_PLUGIN_TYPE,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"gtk-win32-gallax-win_toast_notifications",
	"Windows Toast Notifications",
	"1.4.0",

	"Native Windows Toast Notifications.",
	"Displays native Windows Toast Notifications.",
	"Christian Galla <ChristianGalla@users.noreply.github.com>",
	"https://github.com/ChristianGalla/PidginWinToastNotifications",

	plugin_load,
	plugin_unload,
	NULL,

	&pidgin_plugin_info,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void output_toast_error(int errorNumber, char *message)
{
	char *errorMessage = NULL;
	switch (errorNumber)
	{
	case 0:
		errorMessage = "No error. The process was executed correctly";
		break;
	case 1:
		errorMessage = "The library has not been initialized";
		break;
	case 2:
		errorMessage = "The OS does not support WinToast";
		break;
	case 3:
		errorMessage = "The library was not able to create a Shell Link for the app";
		break;
	case 4:
		errorMessage = "The AUMI is not a valid one";
		break;
	case 5:
		errorMessage = "The parameters used to configure the library are not valid normally because an invalid AUMI or App Name";
		break;
	case 6:
		errorMessage = "The toast was created correctly but WinToast was not able to display the toast";
		break;
	case 7:
		errorMessage = "Unknown error";
		break;
	default:
		errorMessage = "Unknown exception";
		break;
	}
	purple_debug_error("win_toast_notifications", "%s: %s\n",
					   message, errorMessage);
}

static char *get_attr_text(const char *protocolName, const char *userName, const char *chatName)
{
	const char *startText = "Via ";
	const char *chatText = "In chat ";
	const char *formatText = " ()";
	const char *formatChat = "\r\n";
	int startTextSize = strlen(startText);
	int formatTextSize = strlen(formatText);
	int protocolNameSize = strlen(protocolName);
	int userNameSize = strlen(userName);
	int targetPos = 0;
	int chatTextSize = 0;
	int chatNameSize = 0;
	int formatChatSize = 0;
	int size;
	char *ret = NULL;
	if (chatName != NULL)
	{
		chatNameSize = strlen(chatName);
		chatTextSize = strlen(chatText);
		formatChatSize = strlen(formatChat);
	}
	size = startTextSize + formatTextSize + protocolNameSize + userNameSize + chatNameSize + chatTextSize + formatChatSize + 1;
	ret = (char *)malloc(size);
	memcpy(ret + targetPos, startText, startTextSize);
	targetPos += startTextSize;
	memcpy(ret + targetPos, protocolName, protocolNameSize);
	targetPos += protocolNameSize;
	ret[targetPos] = ' ';
	targetPos++;
	ret[targetPos] = '(';
	targetPos++;
	memcpy(ret + targetPos, userName, userNameSize);
	targetPos += userNameSize;
	ret[targetPos] = ')';
	targetPos++;
	if (chatNameSize > 0)
	{
		ret[targetPos] = '\r';
		targetPos++;
		ret[targetPos] = '\n';
		targetPos++;
		memcpy(ret + targetPos, chatText, chatTextSize);
		targetPos += chatTextSize;
		memcpy(ret + targetPos, chatName, chatNameSize);
		targetPos += chatNameSize;
	}
	ret[targetPos] = '\0';
	return ret;
}

static void toast_clicked_cb(PurpleConversation *conv)
{
	PidginConversation *gtkconv;
	purple_debug_misc("win_toast_notifications", "toast clicked\n");
	pidgin_conv_attach_to_conversation(conv);
	gtkconv = PIDGIN_CONVERSATION(conv);
	pidgin_conv_switch_active_conversation(conv);
	pidgin_conv_window_switch_gtkconv(gtkconv->win, gtkconv);
	gtk_window_present(GTK_WINDOW(gtkconv->win->window));
}

static gboolean get_effective_setting(PurpleStatusPrimitive status, Setting setting, Buddy_type buddy_type, const char *protocol_id, const char *account_name, const char *buddy_name) {
	char *path;
	gboolean exists;
	gboolean value;

	purple_debug_misc("win_toast_notifications", "Checking effective settings\n");

	// check buddy setting for status
	path = get_prefs_path(status, SETTING_ENABLED, buddy_type, protocol_id, account_name, buddy_name);
	exists = purple_prefs_exists(path);
	if (exists) {
		if (purple_prefs_get_bool(path)) {
			purple_debug_misc("win_toast_notifications", "Found enabled buddy setting for status: %s\n", path);
			free(path);
			path = get_prefs_path(status, setting, buddy_type, protocol_id, account_name, buddy_name);
			exists = purple_prefs_exists(path);
			if (exists) {
				value = purple_prefs_get_bool(path);
				purple_debug_misc("win_toast_notifications", "Found setting for status %i: %i, path: %s\n", setting, value, path);
				free(path);
				return value;
			} else {
				free(path);
			}
		} else {
			free(path);
		}
	}

	// check buddy setting for unset status
	path = get_prefs_path(PURPLE_STATUS_UNSET, SETTING_ENABLED, buddy_type, protocol_id, account_name, buddy_name);
	exists = purple_prefs_exists(path);
	if (exists) {
		if (purple_prefs_get_bool(path)) {
			purple_debug_misc("win_toast_notifications", "Found enabled buddy setting for unset status: %s\n", path);
			free(path);
			path = get_prefs_path(PURPLE_STATUS_UNSET, setting, buddy_type, protocol_id, account_name, buddy_name);
			exists = purple_prefs_exists(path);
			if (exists) {
				value = purple_prefs_get_bool(path);
				purple_debug_misc("win_toast_notifications", "Found setting for status %i: %i, path: %s\n", setting, value, path);
				free(path);
				return value;
			} else {
				free(path);
			}
		} else {
			free(path);
		}
	}

	// check global setting for status
	path = get_prefs_path(status, SETTING_ENABLED, BUDDY_TYPE_GLOBAL, NULL, NULL, NULL);
	exists = purple_prefs_exists(path);
	if (exists) {
		if (purple_prefs_get_bool(path)) {
			purple_debug_misc("win_toast_notifications", "Found enabled global setting for status: %s\n", path);
			free(path);
			path = get_prefs_path(status, setting, BUDDY_TYPE_GLOBAL, NULL, NULL, NULL);
			exists = purple_prefs_exists(path);
			if (exists) {
				value = purple_prefs_get_bool(path);
				purple_debug_misc("win_toast_notifications", "Found setting for status %i: %i, path: %s\n", setting, value, path);
				free(path);
				return value;
			} else {
				free(path);
			}
		} else {
			free(path);
		}
	}

	// check global setting for unset status
	path = get_prefs_path(PURPLE_STATUS_UNSET, SETTING_ENABLED, BUDDY_TYPE_GLOBAL, NULL, NULL, NULL);
	exists = purple_prefs_exists(path);
	if (exists) {
		if (purple_prefs_get_bool(path)) {
			purple_debug_misc("win_toast_notifications", "Found enabled global setting for unset status: %s\n", path);
			free(path);
			path = get_prefs_path(PURPLE_STATUS_UNSET, setting, BUDDY_TYPE_GLOBAL, NULL, NULL, NULL);
			exists = purple_prefs_exists(path);
			if (exists) {
				value = purple_prefs_get_bool(path);
				purple_debug_misc("win_toast_notifications", "Found setting for status %i: %i, path: %s\n", setting, value, path);
				free(path);
				return value;
			} else {
				free(path);
			}
		} else {
			free(path);
		}
	}

	return FALSE;	
}

static gboolean should_show(PurpleAccount *account, PurpleConversation *conv, PurpleConversationType convType, PurpleMessageFlags flags, const char *sender) {
	PurpleStatus * purpleStatus = NULL;
	PurpleStatusType * statusType = NULL;
	PurpleStatusPrimitive primStatus = 0;
	const char * protocol_id = NULL;
	const char * account_name = NULL;

	if (!(flags & PURPLE_MESSAGE_RECV)) {
		return FALSE;
	}
	if (flags & PURPLE_MESSAGE_SYSTEM) {
		return FALSE;
	}

	protocol_id = purple_account_get_protocol_id(account);
	account_name = purple_account_get_username(account);

	purpleStatus = purple_account_get_active_status(account);
	statusType = purple_status_get_type(purpleStatus);
	primStatus = purple_status_type_get_primitive(statusType);

	if (convType == PURPLE_CONV_TYPE_IM) {
		purple_debug_misc("win_toast_notifications", "Checking im settings\n");
		if (!get_effective_setting(primStatus, SETTING_FOR_IM, BUDDY_TYPE_BUDDY, protocol_id, account_name, sender)) {
			return FALSE;
		}
		if (conv != NULL && purple_conversation_has_focus(conv) && !get_effective_setting(primStatus, SETTING_FOR_FOCUS, BUDDY_TYPE_BUDDY, protocol_id, account_name, sender))
		{
			return FALSE;
		}
	} else if (convType == PURPLE_CONV_TYPE_CHAT) {
		purple_debug_misc("win_toast_notifications", "Checking chat settings\n");
		if (!get_effective_setting(primStatus, SETTING_FOR_CHAT, BUDDY_TYPE_CHAT, protocol_id, account_name, sender)) {
			if (!(flags & PURPLE_MESSAGE_NICK && get_effective_setting(primStatus, SETTING_FOR_CHAT_MENTIONED, BUDDY_TYPE_CHAT, protocol_id, account_name, sender))) {
				return FALSE;
			}
		}
		if (conv != NULL && purple_conversation_has_focus(conv) && !get_effective_setting(primStatus, SETTING_FOR_FOCUS, BUDDY_TYPE_CHAT, protocol_id, account_name, sender))
		{
			return FALSE;
		}
	}

	return TRUE;
}

static void
displayed_msg_cb(PurpleAccount *account, char *sender, char *buffer,
				 PurpleConversation *conv, PurpleMessageFlags flags)
{
	const char *chatName = NULL;
	const char *protocolName = NULL;
	const char *userName = NULL;
	char *attrText = NULL;
	PurpleBuddy *buddy = NULL;
	int callResult;
	const char *senderName = NULL;
	PurpleBuddyIcon *icon = NULL;
	const char *iconPath = NULL;
	PurpleConversationType convType = purple_conversation_get_type(conv);

	if (should_show(account, conv, convType, flags, sender))
	{
		buddy = purple_find_buddy(account, sender);
		if (buddy != NULL)
		{
			purple_debug_misc("win_toast_notifications", "Received a message from a buddy\n");
			senderName = purple_buddy_get_alias(buddy);
			if (senderName == NULL)
			{
				senderName = purple_buddy_get_name(buddy);
				if (senderName == NULL)
				{
					senderName = sender;
				}
			}
			icon = purple_buddy_get_icon(buddy);
			if (icon != NULL)
			{
				iconPath = purple_buddy_icon_get_full_path(icon);
			}
		}
		else
		{
			purple_debug_misc("win_toast_notifications", "Received a message from someone who is not a buddy\n");
			senderName = sender;
		}

		if (conv != NULL && convType == PURPLE_CONV_TYPE_CHAT)
		{
			chatName = purple_conversation_get_title(conv);
		}

		userName = purple_account_get_username(account);
		protocolName = purple_account_get_protocol_name(account);
		attrText = get_attr_text(protocolName, userName, chatName);

		purple_debug_misc("win_toast_notifications", "displayed_msg_cb (%s, %s, %s, %s, %s, %d)\n",
							protocolName, userName, sender, buffer,
							chatName, flags);
		callResult = (showToastProcAdd)(senderName, buffer, iconPath, attrText, conv);
		if (callResult)
		{
			output_toast_error(callResult, "Failed to show Toast Notification");
		}
		else
		{
			purple_debug_misc("win_toast_notifications", "Showed Toast Notification\n");
		}
		free(attrText);
	}
}

static void button_clicked_cb(GtkButton *button, char *path)
{
    gboolean value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	purple_prefs_set_bool(path, value);
}

static void settings_dialog_destroy_cb(GtkWidget *w, struct localSettingsData *data)
{
	struct charListNode *node = 0;
	struct charListNode *lastNode = 0;
	node = data->paths;
	while (node != 0) {
		free(node->str);
		lastNode = node;
		node = node->next;
		free(lastNode);
	}
	free(data);
}

static void local_settings_dialog_response_cb(GtkWidget *dialog, gint resp, struct localSettingsData *data)
{
    gtk_widget_destroy(dialog);
}

static void add_setting_button(
	PurpleStatusPrimitive status,
	Buddy_type buddy_type,
	char *protocol_id,
	char *account_name,
	char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data,
	Setting setting,
	char *labelText
) {
	GtkWidget * button;
	char *path;
	struct charListNode *node;
	gboolean exists;
	gboolean value;

	button = gtk_check_button_new_with_label(labelText);
	gtk_container_add(GTK_CONTAINER(vbox), button);
	path = get_prefs_path(status, setting, buddy_type, protocol_id, account_name, buddy_name);
	node = malloc(sizeof(struct charListNode));
	node->next = data->paths;
	data->paths = node;
	node->str = path;
	ensure_prefs_path(path);
	exists = purple_prefs_exists(path);
	if (!exists) {
		purple_prefs_add_bool(path, FALSE);
	}
	value = purple_prefs_get_bool(path);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), value);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(button_clicked_cb), path);
}

static void add_setting_buttons(
	PurpleStatusPrimitive status,
	Buddy_type buddy_type,
	char *protocol_id,
	char *account_name,
	char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data,
	char *labelText
) {
	GtkWidget *label;
	char *markup;
	gchar *label_markup = "<span weight=\"bold\">%s</span>";

	label = gtk_label_new(NULL);
	markup = g_markup_printf_escaped(label_markup, labelText);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_container_add(GTK_CONTAINER(vbox), label);

	add_setting_button(
		status,
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data,
		SETTING_ENABLED,
		"Enabled"
	);
	if (buddy_type != BUDDY_TYPE_CHAT) {
		add_setting_button(
			status,
			buddy_type,
			protocol_id,
			account_name,
			buddy_name,
			vbox,
			data,
			SETTING_FOR_IM,
			"Direct messages"
		);
	}
	if (buddy_type != BUDDY_TYPE_BUDDY) {
		add_setting_button(
			status,
			buddy_type,
			protocol_id,
			account_name,
			buddy_name,
			vbox,
			data,
			SETTING_FOR_CHAT,
			"Every message in chats"
		);
		add_setting_button(
			status,
			buddy_type,
			protocol_id,
			account_name,
			buddy_name,
			vbox,
			data,
			SETTING_FOR_CHAT_MENTIONED,
			"Messages in chats when mentioned"
		);
	}
	add_setting_button(
		status,
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data,
		SETTING_FOR_FOCUS,
		"Even if the window is focused"
	);
}

static void add_setting_groups(
	Buddy_type buddy_type,
	char *protocol_id,
	char *account_name,
	char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data
) {
	add_setting_buttons(
		PURPLE_STATUS_UNSET,
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data,
		"In every status notify for"
	);
	add_setting_buttons(
		PURPLE_STATUS_AVAILABLE,
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data,
		"If in status 'Available' instead notify for"
	);
	add_setting_buttons(
		PURPLE_STATUS_AWAY,
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data,
		"If in status 'Away' instead notify for"
	);
	add_setting_buttons(
		PURPLE_STATUS_UNAVAILABLE,
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data,
		"If in status 'Do not disturb' instead notify for"
	);
	add_setting_buttons(
		PURPLE_STATUS_INVISIBLE,
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data,
		"If in status 'Invisible' instead notify for"
	);
	add_setting_buttons(
		PURPLE_STATUS_EXTENDED_AWAY,
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data,
		"If in status 'Extended away' instead notify for"
	);
}

static void show_local_settings_dialog(PurpleBlistNode *node, gpointer plugin)
{
	PurplePlugin *prpl;
	PurplePluginProtocolInfo *prpl_info = NULL;
	struct localSettingsData *data;
	GtkWidget *dialog;
	GtkWidget *scrolled_window;
	GtkWidget *vbox;
	GtkWidget *label;
	Buddy_type buddy_type;
	PurpleBuddy * buddyNode;
	PurpleChat * chatNode;
	char *chatName;
	char *account_name;
	char *buddy_name;
	char *protocol_id;

	// path = get_prefs_path(PURPLE_STATUS_AVAILABLE, SETTING_ENABLED, BUDDY_TYPE_GLOBAL, NULL, NULL, NULL);
	// purple_debug_misc("win_toast_notifications", "path: %s\n", path);
	if (PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		buddyNode = (PurpleBuddy*) node;
		purple_debug_misc("win_toast_notifications", "Open local settings dialog for buddy: %s, protocol: %s, username: %s\n", buddyNode->name, buddyNode->account->protocol_id, buddyNode->account->username);
		// path = get_prefs_path(PURPLE_STATUS_AVAILABLE, SETTING_ENABLED, BUDDY_TYPE_BUDDY, buddyNode->account->protocol_id, buddyNode->account->username, buddyNode->name);
		// purple_debug_misc("win_toast_notifications", "path: %s\n", path);
		buddy_type = BUDDY_TYPE_BUDDY;
		account_name = buddyNode->account->username;
		buddy_name = buddyNode->name;
		protocol_id = buddyNode->account->protocol_id;
	} else if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
		chatNode = (PurpleChat*) node;
		prpl = purple_find_prpl(purple_account_get_protocol_id(chatNode->account));
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);
		if (prpl_info->chat_info) {
			struct proto_chat_entry *pce;
			GList *parts = prpl_info->chat_info(purple_account_get_connection(chatNode->account));
			pce = parts->data;
			chatName = g_hash_table_lookup(chatNode->components, pce->identifier);
			g_list_foreach(parts, (GFunc)g_free, NULL);
			g_list_free(parts);
			purple_debug_misc("win_toast_notifications", "Open local settings dialog for chat: %s, protocol: %s, username: %s\n", chatName, chatNode->account->protocol_id, chatNode->account->username);
			// path = get_prefs_path(PURPLE_STATUS_AVAILABLE, SETTING_ENABLED, BUDDY_TYPE_CHAT, chatNode->account->protocol_id, chatNode->account->username, chatName);
			// purple_debug_misc("win_toast_notifications", "path: %s\n", path);
			buddy_type = BUDDY_TYPE_CHAT;
			account_name = chatNode->account->username;
			buddy_name = chatName;
			protocol_id = chatNode->account->protocol_id;
		} else {
			purple_debug_error("win_toast_notifications", "Cannot open local settings dialog for chat because it has no chat_info");
			return;
		}
	} else {
		return;
	}

	data = malloc(sizeof(struct localSettingsData));
	data->paths = 0;

	dialog = gtk_dialog_new_with_buttons(
		"Local Windows Toast Notifications Settings",
	    NULL,
		0,
	    GTK_STOCK_CLOSE,
		GTK_RESPONSE_CLOSE,
	    NULL);
	gtk_widget_set_size_request(dialog, 400, 400);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 0);
    gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 0);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);				
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled_window, 
			TRUE, TRUE, 0);
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox);

	label = gtk_label_new("Overwrite global settings for this conversation");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_container_add(GTK_CONTAINER(vbox), label);

	add_setting_groups(
		buddy_type,
		protocol_id,
		account_name,
		buddy_name,
		vbox,
		data
	);
	
	g_signal_connect(G_OBJECT(dialog), "destroy",
	    G_CALLBACK(settings_dialog_destroy_cb), data);
    g_signal_connect(G_OBJECT(dialog), "response",
	    G_CALLBACK(local_settings_dialog_response_cb), data);

	gtk_widget_show_all(dialog);
}

static void
context_menu(PurpleBlistNode *node, GList **menu, gpointer plugin)
{
	PurpleMenuAction *action;

	if (!PURPLE_BLIST_NODE_IS_BUDDY(node) & !PURPLE_BLIST_NODE_IS_CHAT(node)) {
		return;
	}
	action = purple_menu_action_new("Local Windows Toast Notifications Settings",
					PURPLE_CALLBACK(show_local_settings_dialog), plugin, NULL);
	(*menu) = g_list_prepend(*menu, action);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_debug_misc("win_toast_notifications", "loading...\n");
	hinstLib = LoadLibrary(TEXT("PidginWinToastLib.dll"));

	if (hinstLib != NULL)
	{
		purple_debug_misc("win_toast_notifications",
						  "dll loaded\n");

		initAdd = (initProc)GetProcAddress(hinstLib, "pidginWinToastLibInit");
		showToastProcAdd = (showToastProc)GetProcAddress(hinstLib, "pidginWinToastLibShowMessage");

		if (initAdd == NULL)
		{
			purple_debug_misc("win_toast_notifications", "pidginWinToastLibInit not found!\n");
		}
		else if (showToastProcAdd == NULL)
		{
			purple_debug_misc("win_toast_notifications", "pidginWinToastLibShowMessage not found!\n");
		}
		else
		{
			int callResult;
			void *conv_handle;
			purple_debug_misc("win_toast_notifications",
							  "pidginWinToastLibInit called\n");
			callResult = (initAdd)((void *)toast_clicked_cb);
			if (callResult)
			{
				output_toast_error(callResult, "Initialization failed");
			}
			else
			{
				purple_debug_misc("win_toast_notifications",
								  "pidginWinToastLibInit initialized\n");
				conv_handle = pidgin_conversations_get_handle();
				purple_signal_connect(conv_handle, "displayed-im-msg",
									  plugin, PURPLE_CALLBACK(displayed_msg_cb), NULL);
				purple_signal_connect(conv_handle, "displayed-chat-msg",
									  plugin, PURPLE_CALLBACK(displayed_msg_cb), NULL);
			}
		}
	}
	else
	{
		purple_debug_misc("win_toast_notifications",
						  "failed to load dll\n");
	}
	purple_signal_connect(purple_blist_get_handle(), "blist-node-extended-menu", plugin,
						PURPLE_CALLBACK(context_menu), plugin);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	purple_signals_disconnect_by_handle(plugin);

	if (hinstLib != NULL)
	{
		FreeLibrary(hinstLib);
	}

	return TRUE;
}

static void ensure_pref_default(PurpleStatusPrimitive status, Setting setting, gboolean value) {
	char *path;
	gboolean exists;

	path = get_prefs_path(status, setting, BUDDY_TYPE_GLOBAL, NULL, NULL, NULL);
	ensure_prefs_path(path);	
	exists = purple_prefs_exists(path);
	if (!exists) {
		purple_prefs_add_bool(path, value);
	}

	// todo: copy settings of previous version
}

static void set_default_prefs() {
	ensure_pref_default(PURPLE_STATUS_UNSET, SETTING_ENABLED, TRUE);
	ensure_pref_default(PURPLE_STATUS_UNSET, SETTING_FOR_IM, TRUE);
	ensure_pref_default(PURPLE_STATUS_UNSET, SETTING_FOR_CHAT, TRUE);
	ensure_pref_default(PURPLE_STATUS_UNSET, SETTING_FOR_CHAT_MENTIONED, TRUE);
	ensure_pref_default(PURPLE_STATUS_UNSET, SETTING_FOR_FOCUS, FALSE);
	
	ensure_pref_default(PURPLE_STATUS_AVAILABLE, SETTING_ENABLED, FALSE);
	ensure_pref_default(PURPLE_STATUS_AVAILABLE, SETTING_FOR_IM, TRUE);
	ensure_pref_default(PURPLE_STATUS_AVAILABLE, SETTING_FOR_CHAT, TRUE);
	ensure_pref_default(PURPLE_STATUS_AVAILABLE, SETTING_FOR_CHAT_MENTIONED, TRUE);
	ensure_pref_default(PURPLE_STATUS_AVAILABLE, SETTING_FOR_FOCUS, FALSE);
	
	ensure_pref_default(PURPLE_STATUS_AWAY, SETTING_ENABLED, FALSE);
	ensure_pref_default(PURPLE_STATUS_AWAY, SETTING_FOR_IM, TRUE);
	ensure_pref_default(PURPLE_STATUS_AWAY, SETTING_FOR_CHAT, FALSE);
	ensure_pref_default(PURPLE_STATUS_AWAY, SETTING_FOR_CHAT_MENTIONED, TRUE);
	ensure_pref_default(PURPLE_STATUS_AWAY, SETTING_FOR_FOCUS, FALSE);
	
	ensure_pref_default(PURPLE_STATUS_UNAVAILABLE, SETTING_ENABLED, TRUE);
	ensure_pref_default(PURPLE_STATUS_UNAVAILABLE, SETTING_FOR_IM, FALSE);
	ensure_pref_default(PURPLE_STATUS_UNAVAILABLE, SETTING_FOR_CHAT, FALSE);
	ensure_pref_default(PURPLE_STATUS_UNAVAILABLE, SETTING_FOR_CHAT_MENTIONED, FALSE);
	ensure_pref_default(PURPLE_STATUS_UNAVAILABLE, SETTING_FOR_FOCUS, FALSE);
	
	ensure_pref_default(PURPLE_STATUS_INVISIBLE, SETTING_ENABLED, FALSE);
	ensure_pref_default(PURPLE_STATUS_INVISIBLE, SETTING_FOR_IM, TRUE);
	ensure_pref_default(PURPLE_STATUS_INVISIBLE, SETTING_FOR_CHAT, FALSE);
	ensure_pref_default(PURPLE_STATUS_INVISIBLE, SETTING_FOR_CHAT_MENTIONED, TRUE);
	ensure_pref_default(PURPLE_STATUS_INVISIBLE, SETTING_FOR_FOCUS, FALSE);
	
	ensure_pref_default(PURPLE_STATUS_EXTENDED_AWAY, SETTING_ENABLED, FALSE);
	ensure_pref_default(PURPLE_STATUS_EXTENDED_AWAY, SETTING_FOR_IM, TRUE);
	ensure_pref_default(PURPLE_STATUS_EXTENDED_AWAY, SETTING_FOR_CHAT, FALSE);
	ensure_pref_default(PURPLE_STATUS_EXTENDED_AWAY, SETTING_FOR_CHAT_MENTIONED, TRUE);
	ensure_pref_default(PURPLE_STATUS_EXTENDED_AWAY, SETTING_FOR_FOCUS, FALSE);
}

static void
init_plugin(PurplePlugin *plugin)
{
	set_default_prefs();
}

PURPLE_INIT_PLUGIN(win_toast_notifications, init_plugin, info);