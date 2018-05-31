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

typedef int (__cdecl *initProc)(void(*clickCallback)(void *conv));
typedef int (__cdecl *showToastProc)(const char * sender, const char * message, const char * imagePath, const char * protocolName, void *conv);

HINSTANCE hinstLib;
initProc initAdd;
showToastProc showToastProcAdd;

void output_toast_error(int errorNumber, char *message);
char * get_attr_text(const char *protocolName, const char *userName, const char *chatName);
void toast_clicked_cb(PurpleConversation *conv);

void output_toast_error(int errorNumber, char *message) {
	char *errorMessage = NULL;
	switch(errorNumber) {
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

char * get_attr_text(const char *protocolName, const char *userName, const char *chatName) {
	const char * startText = "Via ";
	const char * chatText = "In chat ";
	const char * formatText = " ()";
	const char * formatChat = "\r\n";
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
	if (chatName != NULL) {
		chatNameSize = strlen(chatName);
		chatTextSize = strlen(chatText);
		formatChatSize = strlen(formatChat);
	}
	size = startTextSize + formatTextSize + protocolNameSize + userNameSize + chatNameSize + chatTextSize + formatChatSize + 1;
	ret = (char*)malloc(size);
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
	if (chatNameSize > 0) {
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

void toast_clicked_cb(PurpleConversation *conv) {
	PidginConversation *gtkconv;
	purple_debug_misc("win_toast_notifications", "toast clicked\n");
	pidgin_conv_attach_to_conversation(conv);
	gtkconv = PIDGIN_CONVERSATION(conv);
	pidgin_conv_switch_active_conversation(conv);
	pidgin_conv_window_switch_gtkconv(gtkconv->win, gtkconv);
	gtk_window_present(GTK_WINDOW(gtkconv->win->window));
}

static void
received_im_msg_cb(PurpleAccount *account, char *sender, char *buffer,
				   PurpleConversation *conv, PurpleMessageFlags flags, void *data)
{
	int callResult;
    const char *protocolName = NULL;
    char *attrText = NULL;
	PurpleBuddy * buddy = NULL;
	const char *userName = NULL;
	PurpleBuddyIcon * icon = NULL;
	const char *iconPath = NULL;
	gboolean hasFocus = FALSE;
	const char *senderName = NULL;
	
	buddy = purple_find_buddy(account, sender);
	if (buddy != NULL) {
		purple_debug_misc("win_toast_notifications","Received a direct message from a buddy\n");
		senderName = purple_buddy_get_alias(buddy);
		if (senderName == NULL) {
			senderName = purple_buddy_get_name(buddy);
			if (senderName == NULL) {
				senderName = sender;
			}
		}
		icon = purple_buddy_get_icon(buddy);
		if (icon != NULL) {
			iconPath = purple_buddy_icon_get_full_path(icon);
		}
	} else {
		purple_debug_misc("win_toast_notifications","Received a direct message from someone who is not a buddy\n");
		senderName = sender;
	}
	
	userName = purple_account_get_username(account);
	protocolName = purple_account_get_protocol_name(account);
	attrText = get_attr_text(protocolName, userName, NULL);

	purple_debug_misc("win_toast_notifications", "received-im-msg (%s, %s, %s, %s, %s, %d)\n",
					protocolName, userName, sender, buffer, 
					senderName, flags);
	if (conv != NULL) {
		hasFocus = purple_conversation_has_focus(conv);
	} else {
		purple_debug_misc("win_toast_notifications","PurpleConversation is NULL\n");
		// @todo create conversation?
	}
	if (!hasFocus) {
		callResult = (showToastProcAdd)(senderName, buffer, iconPath, attrText, conv); 
		if (callResult) {
			output_toast_error(callResult, "Failed to show Toast Notification");
		} else {
			purple_debug_misc("win_toast_notifications","Showed Toast Notification\n");
		}
	}
	free(attrText);
}

static void
received_chat_msg_cb(PurpleAccount *account, char *sender, char *buffer,
					 PurpleConversation *chat, PurpleMessageFlags flags, void *data)
{
    const char *chatName = NULL;
    const char *protocolName = NULL;
	const char *userName = NULL;
    char *attrText = NULL;
	PurpleBuddy * buddy = NULL;
	int callResult;
	gboolean hasFocus = FALSE;
	const char *senderName = NULL;
	PurpleBuddyIcon * icon = NULL;
	const char *iconPath = NULL;

	buddy = purple_find_buddy(account, sender);
	if (buddy != NULL) {
		purple_debug_misc("win_toast_notifications","Received a chat message from a buddy\n");
		senderName = purple_buddy_get_alias(buddy);
		if (senderName == NULL) {
			senderName = purple_buddy_get_name(buddy);
			if (senderName == NULL) {
				senderName = sender;
			}
		}
		icon = purple_buddy_get_icon(buddy);
		if (icon != NULL) {
			iconPath = purple_buddy_icon_get_full_path(icon);
		}
	} else {
		purple_debug_misc("win_toast_notifications","Received a chat message from someone who is not a buddy\n");
		senderName = sender;
	}

	if (chat != NULL) {
		chatName = purple_conversation_get_title(chat);
	}

	userName = purple_account_get_username(account);
	protocolName = purple_account_get_protocol_name(account);
	attrText = get_attr_text(protocolName, userName, chatName);

	purple_debug_misc("win_toast_notifications", "received-chat-msg (%s, %s, %s, %s, %s, %d)\n",
					protocolName, userName, sender, buffer,
					chatName, flags);
	if (chat != NULL) {
		hasFocus = purple_conversation_has_focus(chat);
	} else {
		purple_debug_misc("win_toast_notifications","PurpleConversation is NULL\n");
		// @todo create chat?
	}
	if (!hasFocus) {
		callResult = (showToastProcAdd)(senderName, buffer, iconPath, attrText, chat);
		if (callResult) {
			output_toast_error(callResult, "Failed to show Toast Notification");
		} else {
			purple_debug_misc("win_toast_notifications","Showed Toast Notification\n");
		}
	}
	free(attrText);
}

static gboolean
plugin_load(PurplePlugin *plugin) {

	purple_debug_misc("win_toast_notifications",
						"loading...\n");
	hinstLib = LoadLibrary(TEXT("PidginWinToastLib.dll")); 
	
    if (hinstLib != NULL) 
    { 
		purple_debug_misc("win_toast_notifications",
							"dll loaded\n");

        initAdd = (initProc) GetProcAddress(hinstLib, "pidginWinToastLibInit"); 
        showToastProcAdd = (showToastProc) GetProcAddress(hinstLib, "pidginWinToastLibShowMessage"); 
		
		if (initAdd == NULL) {
			purple_debug_misc("win_toast_notifications", "pidginWinToastLibInit not found!\n");
		} else if (showToastProcAdd == NULL) {
			purple_debug_misc("win_toast_notifications", "pidginWinToastLibShowMessage not found!\n");
		} else {
			int callResult;
			void *conv_handle;
			purple_debug_misc("win_toast_notifications",
								"pidginWinToastLibInit called\n");
            callResult = (initAdd)((void*)toast_clicked_cb);
			if (callResult) {
				output_toast_error(callResult, "Initialization failed");
			} else {
				purple_debug_misc("win_toast_notifications",
									"pidginWinToastLibInit initialized\n");
				conv_handle = purple_conversations_get_handle();
				purple_signal_connect(conv_handle, "received-im-msg",
					plugin, PURPLE_CALLBACK(received_im_msg_cb), NULL);
				purple_signal_connect(conv_handle, "received-chat-msg",
					plugin, PURPLE_CALLBACK(received_chat_msg_cb), NULL);
			}
        }
    } else {
		purple_debug_misc("win_toast_notifications",
							"failed to load dll\n");
	}

    return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	purple_signals_disconnect_by_handle(plugin);
	
	if (hinstLib != NULL) {
		FreeLibrary(hinstLib); 
	}

	return TRUE;
}

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
    "1.3.0",

    "Native Windows Toast Notifications.",          
    "Displays native Windows Toast Notifications.",          
    "Christian Galla <ChristianGalla@users.noreply.github.com>",                          
    "https://github.com/ChristianGalla/PidginWinToastNotifications",     
    
    plugin_load,                   
    plugin_unload,                          
    NULL,                          
                                   
    NULL,                          
    NULL,                          
    NULL,                        
    NULL,                   
    NULL,                          
    NULL,                          
    NULL,                          
    NULL                           
};                               
    
static void                        
init_plugin(PurplePlugin *plugin)
{                                  
}

PURPLE_INIT_PLUGIN(win_toast_notifications, init_plugin, info)