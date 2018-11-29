#define PURPLE_PLUGINS

#include <search.h>
#include "string.h"

#include <glib.h>
#include "gtkplugin.h"
#include "gtkconv.h"
#include "gtkconvwin.h"

#include "account.h"
#include "debug.h"
#include "signals.h"
#include "plugin.h"
#include "version.h"
#include "blist.h"

typedef int(__cdecl *initProc)(
	void (*clickCallback)(void *conv)
);

typedef int(__cdecl *showToastProc)(
	const char *sender,
	const char *message,
	const char *imagePath,
	const char *protocolName,
	void *conv
);

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

typedef struct {
	PurpleAccount * account;
	time_t connect_time;
} ConnectionNode;

int compare_connection_nodes (const void *a, const void *b);

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

typedef enum {
	SETTING_NONE,
	SETTING_ENABLED,
	SETTING_FOR_IM,
	SETTING_FOR_CHAT,
	SETTING_FOR_CHAT_MENTIONED,
	SETTING_FOR_FOCUS,
	SETTING_BUDDY_SIGNED_ON,
	SETTING_BUDDY_SIGNED_OFF
} Setting;

typedef enum {
	BUDDY_TYPE_GLOBAL,
	BUDDY_TYPE_GROUP,
	BUDDY_TYPE_BUDDY,
	BUDDY_TYPE_CHAT
} Buddy_type;

static void output_toast_error(
	int errorNumber,
	const char *message
);

static char* get_attr_text(
	const char *protocolName,
	const char *userName,
	const char *chatName
);

static void toast_clicked_cb(
	PurpleConversation *conv
);

static gboolean should_show_message(
	PurpleAccount *account,
	PurpleConversation *conv,
	PurpleConversationType convType,
	PurpleMessageFlags flags,
	const char *sender
);

static void displayed_msg_cb(
	PurpleAccount *account,
	const char *sender,
	const char *buffer,
	PurpleConversation *conv,
	PurpleMessageFlags flags
);

static void account_signed_on(
	PurpleAccount *account
);

static void buddy_sign_cb(
	PurpleBuddy *buddy,
	BOOL online
);

static void button_clicked_cb(
	GtkButton *button,
	const char *path
);

static void settings_dialog_destroy_cb(
	GtkWidget *w,
	struct localSettingsData *data
);

static void local_settings_dialog_response_cb(
	GtkWidget *dialog,
	gint resp,
	struct localSettingsData *data
);

static void show_local_settings_dialog(
	PurpleBlistNode *node,
	gpointer plugin
);

static void context_menu(
	PurpleBlistNode *node,
	GList **menu,
	gpointer plugin
);

static gboolean plugin_load(
	PurplePlugin *plugin
);

static gboolean plugin_unload(
	PurplePlugin *plugin
);

static void init_plugin(
	PurplePlugin *plugin
);

static char* get_old_prefs_path(
	PurpleStatusPrimitive status,
	Setting setting
);

static char* get_prefs_path(
	PurpleStatusPrimitive status,
	Setting setting,
	Buddy_type buddy_type,
	const char *group_name,
	const char *protocol_id,
	const char *account_name,
	const char *buddy_name
);

static const char* get_prefs_sub_path(
	Setting setting
);

static const char* get_buddy_type_sub_path(
	Buddy_type buddy_type
);

static void add_setting_button(
	PurpleStatusPrimitive status,
	Buddy_type buddy_type,
	const char *group_name,
	const char *protocol_id,
	const char *account_name,
	const char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data,
	Setting setting,
	const char *labelText
);

static void add_setting_buttons(
	PurpleStatusPrimitive status,
	Buddy_type buddy_type,
	const char *group_name,
	const char *protocol_id,
	const char *account_name,
	const char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data,
	const char *labelText
);

static void add_setting_groups(
	Buddy_type buddy_type,
	const char *group_name,
	const char *protocol_id,
	const char *account_name,
	const char *buddy_name,
	GtkWidget *vbox,
	struct localSettingsData *data
);

static void ensure_prefs_path(
	const char *path
);

static void ensure_pref_default(
	PurpleStatusPrimitive status,
	Setting setting,
	gboolean value
);

static void set_default_prefs(void);

static gboolean get_effective_setting(
	PurpleStatusPrimitive status,
	Setting setting,
	Buddy_type buddy_type,
	const char *group_name,
	const char *protocol_id,
	const char *account_name,
	const char *buddy_name
);

GtkWidget * get_config_frame(
	PurplePlugin *plugin
);

void add_connection_node_to_delete_array (const void *nodep, VISIT value, int level);