#define PURPLE_PLUGINS

#include <glib.h>

#include "gtkplugin.h"
#include "account.h"
#include "debug.h"
#include "signals.h"
#include "plugin.h"
#include "version.h"
#include "xmlnode.h"
#include "blist.h"

typedef int (__cdecl *initProc)(); 
typedef int (__cdecl *showToastProc)(const char * sender, const char * message, const char * imagePath, const char * protocolName); 

HINSTANCE hinstLib;
initProc initAdd;
showToastProc showToastProcAdd;

char* getStringWithoutXml(const char * text);

char* copyCharArrayToHeap(const char* source);
	
char* copyCharArrayToHeap(const char* source) {
	size_t size = strlen(source) + 1;
	char* dest = (char*)malloc(size);
	if (dest != NULL) {
		strcpy(dest, source);
	}
	return dest;
}

char* getStringWithoutXml(const char * text) {
	xmlnode * xml = xmlnode_from_str(text,strlen(text));
	if (xml == NULL) {
		return copyCharArrayToHeap(text);
	} else {
		return xmlnode_get_data(xml);	
	}
}

static void
received_im_msg_cb(PurpleAccount *account, char *sender, char *buffer,
				   PurpleConversation *conv, PurpleMessageFlags flags, void *data)
{
	int callResult;
    const char *protocolName = NULL;
	char *simpleMessage = NULL;
	const char *constSimpleMessage = "";
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
	
	if (buffer != NULL) {
		simpleMessage = buffer;
		constSimpleMessage = simpleMessage;
	}
	
	protocolName = purple_account_get_protocol_name(account);

	purple_debug_misc("win_toast_notifications", "received-im-msg (%s, %s, %s, %s, %s, %s, %d)\n",
					protocolName, purple_account_get_username(account), sender, buffer, constSimpleMessage,
					senderName, flags);
	callResult = (showToastProcAdd)(senderName, constSimpleMessage, iconPath, protocolName); 
	purple_debug_misc("win_toast_notifications",
		"Result: %d\n",
		callResult);
	/*if (simpleMessage != NULL) {
		free(simpleMessage);
	}
	if (iconPath != NULL) {
		g_free(iconPath);
	}*/
}

static void
received_chat_msg_cb(PurpleAccount *account, char *sender, char *buffer,
					 PurpleConversation *chat, PurpleMessageFlags flags, void *data)
{
    const char *constChatName;
	int callResult;
	char *simpleMessage = NULL;
	const char *constSimpleMessage = "";

	if (chat != NULL) {
		constChatName = purple_conversation_get_title(chat);
	} else {
		constChatName = sender;
	}
	
	if (buffer != NULL) {
		simpleMessage = getStringWithoutXml(buffer);
		constSimpleMessage = simpleMessage;
	}

	purple_debug_misc("win_toast_notifications", "received-chat-msg (%s, %s, %s, %s, %s, %d)\n",
					purple_account_get_username(account), sender, buffer, constSimpleMessage,
					constChatName, flags);
	callResult = (showToastProcAdd)(constChatName, constSimpleMessage, NULL, NULL); 
	purple_debug_misc("win_toast_notifications",
		"Result: %d\n",
		callResult);
	if (simpleMessage != NULL) {
		free(simpleMessage);
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
			purple_debug_misc("win_toast_notifications",
			 	"Result: %d\n",
			 	callResult);
			
			conv_handle = purple_conversations_get_handle();
			purple_signal_connect(conv_handle, "received-im-msg",
				plugin, PURPLE_CALLBACK(received_im_msg_cb), NULL);
			purple_signal_connect(conv_handle, "received-chat-msg",
				plugin, PURPLE_CALLBACK(received_chat_msg_cb), NULL);
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