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

struct localSettingsData {
    GtkWidget *local_overwrite;
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

const struct status_options_paths paths_default = {
	"/plugins/gtk/gallax-win_toast_notifications",
	NULL,
	"/plugins/gtk/gallax-win_toast_notifications/for_im",
	"/plugins/gtk/gallax-win_toast_notifications/for_chat",
	"/plugins/gtk/gallax-win_toast_notifications/for_chat_mentioned",
	"/plugins/gtk/gallax-win_toast_notifications/for_focus"
};

const struct status_options_paths paths_available = {
	"/plugins/gtk/gallax-win_toast_notifications/available",
	"/plugins/gtk/gallax-win_toast_notifications/available/enabled",
	"/plugins/gtk/gallax-win_toast_notifications/available/for_im",
	"/plugins/gtk/gallax-win_toast_notifications/available/for_chat",
	"/plugins/gtk/gallax-win_toast_notifications/available/for_chat_mentioned",
	"/plugins/gtk/gallax-win_toast_notifications/available/for_focus"
};

const struct status_options_paths paths_away = {
	"/plugins/gtk/gallax-win_toast_notifications/away",
	"/plugins/gtk/gallax-win_toast_notifications/away/enabled",
	"/plugins/gtk/gallax-win_toast_notifications/away/for_im",
	"/plugins/gtk/gallax-win_toast_notifications/away/for_chat",
	"/plugins/gtk/gallax-win_toast_notifications/away/for_chat_mentioned",
	"/plugins/gtk/gallax-win_toast_notifications/away/for_focus"
};

const struct status_options_paths paths_unavailable = {
	"/plugins/gtk/gallax-win_toast_notifications/unavailable",
	"/plugins/gtk/gallax-win_toast_notifications/unavailable/enabled",
	"/plugins/gtk/gallax-win_toast_notifications/unavailable/for_im",
	"/plugins/gtk/gallax-win_toast_notifications/unavailable/for_chat",
	"/plugins/gtk/gallax-win_toast_notifications/unavailable/for_chat_mentioned",
	"/plugins/gtk/gallax-win_toast_notifications/unavailable/for_focus"
};

const struct status_options_paths paths_invisible = {
	"/plugins/gtk/gallax-win_toast_notifications/invisible",
	"/plugins/gtk/gallax-win_toast_notifications/invisible/enabled",
	"/plugins/gtk/gallax-win_toast_notifications/invisible/for_im",
	"/plugins/gtk/gallax-win_toast_notifications/invisible/for_chat",
	"/plugins/gtk/gallax-win_toast_notifications/invisible/for_chat_mentioned",
	"/plugins/gtk/gallax-win_toast_notifications/invisible/for_focus"
};

const struct status_options_paths paths_extended_away = {
	"/plugins/gtk/gallax-win_toast_notifications/extended_away",
	"/plugins/gtk/gallax-win_toast_notifications/extended_away/enabled",
	"/plugins/gtk/gallax-win_toast_notifications/extended_away/for_im",
	"/plugins/gtk/gallax-win_toast_notifications/extended_away/for_chat",
	"/plugins/gtk/gallax-win_toast_notifications/extended_away/for_chat_mentioned",
	"/plugins/gtk/gallax-win_toast_notifications/extended_away/for_focus"
};

HINSTANCE hinstLib;
initProc initAdd;
showToastProc showToastProcAdd;

typedef enum {SETTING_NONE, SETTING_ENABLED, SETTING_FOR_IM, SETTING_FOR_CHAT, SETTING_FOR_CHAT_MENTIONED, SETTING_FOR_FOCUS} Setting;
typedef enum {BUDDY_TYPE_GLOBAL, BUDDY_TYPE_BUDDY, BUDDY_TYPE_CHAT} Buddy_type;

static void output_toast_error(int errorNumber, char *message);
static char* get_attr_text(const char *protocolName, const char *userName, const char *chatName);
static void toast_clicked_cb(PurpleConversation *conv);
static void add_status_specific_pref(PurplePluginPrefFrame * frame, PurpleStatusPrimitive status);
static gboolean should_show(PurpleAccount *account, PurpleConversation *conv, PurpleConversationType convType, PurpleMessageFlags flags);
static void displayed_msg_cb(PurpleAccount *account, char *sender, char *buffer, PurpleConversation *conv, PurpleMessageFlags flags);
static void local_overwrite_clicked_cb(GtkButton *button, struct localSettingsData *data);
static void local_settings_dialog_destroy_cb(GtkWidget *w, struct localSettingsData *data);
static void local_settings_dialog_response_cb(GtkWidget *dialog, gint resp, struct localSettingsData *data);
static void show_local_settings_dialog(PurpleBlistNode *node, gpointer plugin);
static void context_menu(PurpleBlistNode *node, GList **menu, gpointer plugin);
static gboolean plugin_load(PurplePlugin *plugin);
static gboolean plugin_unload(PurplePlugin *plugin);
static void add_status_specific_pref(PurplePluginPrefFrame * frame, PurpleStatusPrimitive status);
static PurplePluginPrefFrame * get_plugin_pref_frame(PurplePlugin *plugin);
static void init_plugin(PurplePlugin *plugin);
static char* get_settings_path(PurpleStatusPrimitive status, Setting setting, Buddy_type buddy_type, char *protocol_id, char *account_name, char *buddy_name);
static const char* get_setting_sub_path(Setting setting);
static const char* get_buddy_type_sub_path(Buddy_type buddy_type);

static const char* get_setting_sub_path(Setting setting) {
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

static char* get_settings_path(PurpleStatusPrimitive status, Setting setting, Buddy_type buddy_type, char *protocol_id, char *account_name, char *buddy_name) {
	// global example: "/plugins/gtk/gallax-win_toast_notifications/global/available/enabled"
	// buddy example: "/plugins/gtk/gallax-win_toast_notifications/buddy/<protocol_id>/<account_name>/<buddy_name>/available/enabled"
	// xmpp example: "/plugins/gtk/gallax-win_toast_notifications/buddy/<protocol_id>/<account_name>/<ressource_name>/<buddy_name>/<ressource_name>/available/enabled"
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

	int targetPos = 0;

	statusStr = purple_primitive_get_id_from_type(status);
	settingStr = get_setting_sub_path(setting);
	buddyTypeStr = get_buddy_type_sub_path(buddy_type);

	basePathSize = strlen(base_settings_path);
	buddyTypeSize = strlen(buddyTypeStr);
	statusSize = strlen(statusStr) + 1; // +1 for '/'
	settingSize = strlen(settingStr);
	if (buddy_type != BUDDY_TYPE_GLOBAL) {
		protocolSize = strlen(protocol_id) + 1;
		accountSize = strlen(account_name) + 1;
		buddyNameSize = strlen(buddy_name) + 1;
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

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame,
	0,	/* page_num (Reserved) */
	NULL, /* frame (Reserved) */
	/* Padding */
	NULL,
	NULL,
	NULL,
	NULL};

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

	NULL,
	NULL,
	&prefs_info,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL};

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

static gboolean should_show(PurpleAccount *account, PurpleConversation *conv, PurpleConversationType convType, PurpleMessageFlags flags) {
	const struct status_options_paths * paths = &paths_default;
	PurpleStatus * purpleStatus = NULL;
	PurpleStatusType * statusType = NULL;
	PurpleStatusPrimitive primStatus = 0;

	if (!(flags & PURPLE_MESSAGE_RECV)) {
		return FALSE;
	}
	if (flags & PURPLE_MESSAGE_SYSTEM) {
		return FALSE;
	}

	purpleStatus = purple_account_get_active_status(account);
	statusType = purple_status_get_type(purpleStatus);
	primStatus = purple_status_type_get_primitive(statusType);
	switch (primStatus) {
		case PURPLE_STATUS_AVAILABLE:
			if (purple_prefs_get_bool(paths_available.enabled)) {
				paths = &paths_available;
			}
			break;
		case PURPLE_STATUS_AWAY:
			if (purple_prefs_get_bool(paths_away.enabled)) {
				paths = &paths_away;
			}
			break;
		case PURPLE_STATUS_EXTENDED_AWAY:
			if (purple_prefs_get_bool(paths_extended_away.enabled)) {
				paths = &paths_extended_away;
			}
			break;
		case PURPLE_STATUS_UNAVAILABLE:
			if (purple_prefs_get_bool(paths_unavailable.enabled)) {
				paths = &paths_unavailable;
			}
			break;
		case PURPLE_STATUS_INVISIBLE:
			if (purple_prefs_get_bool(paths_invisible.enabled)) {
				paths = &paths_invisible;
			}
			break;
		default:
			break;
	}

	if (convType == PURPLE_CONV_TYPE_IM) {
		if (!purple_prefs_get_bool(paths->for_im)) {
			return FALSE;
		}
	}
	if (convType == PURPLE_CONV_TYPE_CHAT) {
		if (!purple_prefs_get_bool(paths->for_chat)) {
			if (!(flags & PURPLE_MESSAGE_NICK && purple_prefs_get_bool(paths->for_chat_mentioned))) {
				return FALSE;
			}
		}
	}
	if (conv != NULL && purple_conversation_has_focus(conv) && !purple_prefs_get_bool(paths->for_focus))
	{
		return FALSE;
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

	if (should_show(account, conv, convType, flags))
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

static void local_overwrite_clicked_cb(GtkButton *button, struct localSettingsData *data)
{
    gboolean local_overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->local_overwrite));
    if (local_overwrite) {
		purple_debug_misc("win_toast_notifications", "local_overwrite enabled\n");
    } else {
		purple_debug_misc("win_toast_notifications", "local_overwrite disabled\n");
    }
}

static void local_settings_dialog_destroy_cb(GtkWidget *w, struct localSettingsData *data)
{
	free(data);
}

static void local_settings_dialog_response_cb(GtkWidget *dialog, gint resp, struct localSettingsData *data)
{
    gtk_widget_destroy(dialog);
}

static void show_local_settings_dialog(PurpleBlistNode *node, gpointer plugin)
{
	PurplePlugin *prpl;
	PurplePluginProtocolInfo *prpl_info = NULL;
	struct localSettingsData *data;
	GtkWidget *dialog;
	GtkWidget *label;
	char *markup;
	GtkWidget * button;
	PurpleBuddy * buddyNode;
	PurpleChat * chatNode;
	char *chatName;
	char *path;
	gchar *label_markup = "<span weight=\"bold\">%s</span>";

	path = get_settings_path(PURPLE_STATUS_AVAILABLE, SETTING_ENABLED, BUDDY_TYPE_GLOBAL, NULL, NULL, NULL);
	purple_debug_misc("win_toast_notifications", "path: %s\n", path);
	if (PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		buddyNode = (PurpleBuddy*) node;
		purple_debug_misc("win_toast_notifications", "Open local settings dialog for buddy: %s, protocol: %s, username: %s\n", buddyNode->name, buddyNode->account->protocol_id, buddyNode->account->username);
		path = get_settings_path(PURPLE_STATUS_AVAILABLE, SETTING_ENABLED, BUDDY_TYPE_BUDDY, buddyNode->account->protocol_id, buddyNode->account->username, buddyNode->name);
		purple_debug_misc("win_toast_notifications", "path: %s\n", path);
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
			path = get_settings_path(PURPLE_STATUS_AVAILABLE, SETTING_ENABLED, BUDDY_TYPE_CHAT, chatNode->account->protocol_id, chatNode->account->username, chatName);
			purple_debug_misc("win_toast_notifications", "path: %s\n", path);
		} else {
			purple_debug_error("win_toast_notifications", "Cannot open local settings dialog for chat because it has no chat_info");
			return;
		}
	} else {
		return;
	}

	data = malloc(sizeof(struct localSettingsData));

	dialog = gtk_dialog_new_with_buttons(
		"Local Windows Toast Notifications Settings",
	    NULL,
		0,
	    GTK_STOCK_CLOSE,
		GTK_RESPONSE_CLOSE,
	    NULL);
	gtk_widget_set_size_request(dialog, 300, -1);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 0);
    gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 0);

	button = gtk_check_button_new_with_label("Overwrite global settings for this conversation");
	// gtk_misc_set_alignment(GTK_MISC(button), 0, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), button, FALSE, FALSE, 5);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(local_overwrite_clicked_cb), data);
	data->local_overwrite = button;

	label = gtk_label_new(NULL);
	markup = g_markup_printf_escaped(label_markup, "In every status notify for");
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, FALSE, 5);
	
	g_signal_connect(G_OBJECT(dialog), "destroy",
	    G_CALLBACK(local_settings_dialog_destroy_cb), data);
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

static void
add_status_specific_pref(PurplePluginPrefFrame * frame, PurpleStatusPrimitive status)
{
	PurplePluginPref *ppref;
	const char * title_lable = NULL;
	const struct status_options_paths * paths = NULL;

	switch (status) {
		case PURPLE_STATUS_AVAILABLE:
			paths = &paths_available;
			title_lable = "If in status 'Available' instead notify for";
			break;
		case PURPLE_STATUS_AWAY:
			title_lable = "If in status 'Away' instead notify for";
			paths = &paths_away;
			break;
		case PURPLE_STATUS_UNAVAILABLE:
			title_lable = "If in status 'Do not disturb' instead notify for";
			paths = &paths_unavailable;
			break;
		case PURPLE_STATUS_EXTENDED_AWAY:
			title_lable = "If in status 'Extended away' away instead notify for";
			paths = &paths_extended_away;
			break;
		case PURPLE_STATUS_INVISIBLE:
			title_lable = "If in status 'Invisible' instead notify for";
			paths = &paths_invisible;
			break;
		default:
			return;
	}

	ppref = purple_plugin_pref_new_with_label(title_lable);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths->enabled,
		labels.enabled);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths->for_im,
		labels.for_im);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths->for_chat,
		labels.for_chat);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths->for_chat_mentioned,
		labels.for_chat_mentioned);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths->for_focus,
		labels.for_focus);
	purple_plugin_pref_frame_add(frame, ppref);
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	frame = purple_plugin_pref_frame_new();

	ppref = purple_plugin_pref_new_with_label("In every status notify for");
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths_default.for_im,
		labels.for_im);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths_default.for_chat,
		labels.for_chat);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths_default.for_chat_mentioned,
		labels.for_chat_mentioned);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
		paths_default.for_focus,
		labels.for_focus);
	purple_plugin_pref_frame_add(frame, ppref);

	add_status_specific_pref(frame, PURPLE_STATUS_AVAILABLE);
	add_status_specific_pref(frame, PURPLE_STATUS_AWAY);
	add_status_specific_pref(frame, PURPLE_STATUS_UNAVAILABLE);
	add_status_specific_pref(frame, PURPLE_STATUS_INVISIBLE);
	add_status_specific_pref(frame, PURPLE_STATUS_EXTENDED_AWAY);

	return frame;
}

static void
init_plugin(PurplePlugin *plugin)
{
	purple_prefs_add_none(paths_default.parent);
	purple_prefs_add_bool(paths_default.for_im, TRUE);
	purple_prefs_add_bool(paths_default.for_chat, TRUE);
	purple_prefs_add_bool(paths_default.for_chat_mentioned, TRUE);
	purple_prefs_add_bool(paths_default.for_focus, FALSE);
	
	purple_prefs_add_none(paths_available.parent);
	purple_prefs_add_bool(paths_available.enabled, FALSE);
	purple_prefs_add_bool(paths_available.for_im, TRUE);
	purple_prefs_add_bool(paths_available.for_chat, TRUE);
	purple_prefs_add_bool(paths_available.for_chat_mentioned, TRUE);
	purple_prefs_add_bool(paths_available.for_focus, FALSE);
	
	purple_prefs_add_none(paths_away.parent);
	purple_prefs_add_bool(paths_away.enabled, FALSE);
	purple_prefs_add_bool(paths_away.for_im, TRUE);
	purple_prefs_add_bool(paths_away.for_chat, FALSE);
	purple_prefs_add_bool(paths_away.for_chat_mentioned, TRUE);
	purple_prefs_add_bool(paths_away.for_focus, FALSE);
	
	purple_prefs_add_none(paths_unavailable.parent);
	purple_prefs_add_bool(paths_unavailable.enabled, TRUE);
	purple_prefs_add_bool(paths_unavailable.for_im, FALSE);
	purple_prefs_add_bool(paths_unavailable.for_chat, FALSE);
	purple_prefs_add_bool(paths_unavailable.for_chat_mentioned, FALSE);
	purple_prefs_add_bool(paths_unavailable.for_focus, FALSE);
	
	purple_prefs_add_none(paths_invisible.parent);
	purple_prefs_add_bool(paths_invisible.enabled, FALSE);
	purple_prefs_add_bool(paths_invisible.for_im, TRUE);
	purple_prefs_add_bool(paths_invisible.for_chat, FALSE);
	purple_prefs_add_bool(paths_invisible.for_chat_mentioned, TRUE);
	purple_prefs_add_bool(paths_invisible.for_focus, FALSE);
	
	purple_prefs_add_none(paths_extended_away.parent);
	purple_prefs_add_bool(paths_extended_away.enabled, FALSE);
	purple_prefs_add_bool(paths_extended_away.for_im, TRUE);
	purple_prefs_add_bool(paths_extended_away.for_chat, FALSE);
	purple_prefs_add_bool(paths_extended_away.for_chat_mentioned, TRUE);
	purple_prefs_add_bool(paths_extended_away.for_focus, FALSE);
}

PURPLE_INIT_PLUGIN(win_toast_notifications, init_plugin, info);