#define PURPLE_PLUGINS

#include <glib.h>

#include "gtkplugin.h"
#include "account.h"
#include "debug.h"
#include "signals.h"
#include "plugin.h"
#include "version.h"
#include "blist.h"

typedef int (__cdecl *initProc)(); 
typedef int (__cdecl *showToastProc)(const char * sender, const char * message, const char * imagePath, const char * protocolName); 

HINSTANCE hinstLib;
initProc initAdd;
showToastProc showToastProcAdd;

void output_toast_error(int errorNumber, char *message);

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

static void
received_im_msg_cb(PurpleAccount *account, char *sender, char *buffer,
				   PurpleConversation *conv, PurpleMessageFlags flags, void *data)
{
	int callResult;
    const char *protocolName = NULL;
	PurpleBuddy * buddy = NULL;
    const char *senderName = NULL;
	PurpleBuddyIcon * icon = NULL;
	char* iconPath = NULL;
	
	buddy = purple_find_buddy(account, sender);
	senderName = purple_buddy_get_alias(buddy);
	icon = purple_buddy_get_icon(buddy);
	if (icon != NULL) {
		iconPath = purple_buddy_icon_get_full_path(icon);
	}
	
	protocolName = purple_account_get_protocol_name(account);

	purple_debug_misc("win_toast_notifications", "received-im-msg (%s, %s, %s, %s, %s, %d)\n",
					protocolName, purple_account_get_username(account), sender, buffer, 
					senderName, flags);
	callResult = (showToastProcAdd)(senderName, buffer, iconPath, protocolName); 
	if (callResult) {
		output_toast_error(callResult, "Failed to show Toast Notification");
	} else {
		purple_debug_misc("win_toast_notifications","Showed Toast Notification\n");
	}
	if (iconPath != NULL) {
		g_free(iconPath);
	}
}

static void
received_chat_msg_cb(PurpleAccount *account, char *sender, char *buffer,
					 PurpleConversation *chat, PurpleMessageFlags flags, void *data)
{
    const char *constChatName;
	int callResult;

	if (chat != NULL) {
		constChatName = purple_conversation_get_title(chat);
	} else {
		constChatName = sender;
	}

	purple_debug_misc("win_toast_notifications", "received-chat-msg (%s, %s, %s, %s, %d)\n",
					purple_account_get_username(account), sender, buffer,
					constChatName, flags);
	callResult = (showToastProcAdd)(constChatName, buffer, NULL, NULL);
	if (callResult) {
		output_toast_error(callResult, "Failed to show Toast Notification");
	} else {
		purple_debug_misc("win_toast_notifications","Showed Toast Notification\n");
	}
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
 
        // If the function address is valid, call the function.
 
		if (initAdd == NULL) {
			purple_debug_misc("win_toast_notifications", "pidginWinToastLibInit not found!\n");
		} else if (showToastProcAdd == NULL) {
			purple_debug_misc("win_toast_notifications", "pidginWinToastLibShowMessage not found!\n");
		} else {
			int callResult;
			void *conv_handle;
			purple_debug_misc("win_toast_notifications",
								"pidginWinToastLibInit called\n");
            callResult = (initAdd)();
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
    "1.0",

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