# PidginWinToastNotifications

Windows toast notification plugin for Pidgin.

Shows notifications on message receive in the standard Windows design.

Direct message from someone who has a profile picture:

![Direct message with picture](./img/direct_with_picture.jpg "Direct message with picture")

Direct message from someone who has no profile picture:

![Direct message without picture](./img/direct_without_picture.jpg "Direct message without picture")

Chat message from someone who has no profile picture:

![Chat message without picture](./img/chat_without_picture.jpg "Chat message without picture")

This plugin uses Pidgin's standard C API so no other tools like Perl are required.

## Features

It can be configured if notifications should be shown for direct messages, every message in chats, only for messages in chats when mentioned or only if the conversation window is not focused.

This configuration can be overwritten per status (Available, Away, Do not disturb, Extended away or Invisible).

Global configuration:

![Configuration](./img/configuration.png "Configuration")

Also, it is possible to overwrite the global settings for each conversation.

Configuration for a chat:

![Configuration](./img/configuration_chat.png "Configuration for chat")

Configuration for a buddy:

![Configuration](./img/configuration_buddy.png "Configuration for buddy")

The global configuration can be accessed via the default plugin configuration page (In Buddy List: Tools / Plugins / Windows Toast Notifications).

The local settings can be opened from the conversation menu underneath *More* or from the context menu of a buddy or chat in the Buddy List:

![Configuration](./img/configuration_menu.png "Open settings for a conversation")

## Supported operating systems

The plugin was developed and tested with the newest Windows 10 version.

Other Windows 10 versions, Windows 8 and Windows 8.1 should work, too, but this wasn't tested.

To display the protocol, account and chat name in the toast notifications the Windows 10 Anniversary Update (also known as version 1607 or Redstone 1) or newer is required.

If you experience any problems, feel free to open an issue.

## Changelog

All changes between versions are documented in the file [CHANGELOG.md](./CHANGELOG.md).

## Installation

1.  Download the dll files from the [newest release](https://github.com/ChristianGalla/PidginWinToastNotifications/releases/latest).
2. Copy the file PidginWinToastLib.dll to the installation directory of Pidgin. This is usually "C:\Program Files (x86)\Pidgin" on 64-bit versions of Windows or "C:\Program Files\Pidgin" on a 32-bit version.
3. Copy the file PidginWinToastNotifications.dll to the plugin directory of Pidgin. This is usually "C:\Program Files (x86)\Pidgin\plugins" on 64-bit versions of Windows or "C:\Program Files\Pidgin\plugins" on a 32-bit version.
4. Start Pidgin. Open the plugin window and enable "Windows Toast Notifications".

## Architecture

This Plugin consists of the two dll files PidginWinToastNotifications.dll and PidginWinToastLib.dll.

PidginWinToastNotifications.dll is loaded from Pidgin as plugin. It is written in ANSI C and it provides necessary plugin information. It registers callback functions in Pidgin's message API and relays received messages to PidginWinToastLib.dll.

PidginWinToastLib.dll is written in C++ and calls Windows's toast notification API to display the toast notifications. For that it uses [WinToast](https://github.com/mohabouje/WinToast) as library.

## How to build on Windows

### PidginWinToastNotifications.dll

This file is build using the GNU Compiler Collection (GCC).

1. Follow the [official instructions of the Pidgin wiki to set up your build environment](https://developer.pidgin.im/wiki/BuildingWinPidgin#Setupyourbuildenvironment). For this I recommend [pidgin-windev](https://github.com/renatosilva/pidgin-windev).
2. Copy the files Plugin/PidginWinToastNotifications.h and Plugin/PidginWinToastNotifications.c into the subfolder pidgin\plugins inside of your Pidgin development directory.
3. Inside a Cygwin Terminal navigate to pidgin\plugins inside of your Pidgin development directory.
4. Run the Command: *make -f Makefile.mingw PidginWinToastNotifications.dll*

Now the file PidginWinToastNotifications.dll should be in the same folder.

### PidginWinToastLib.dll

This file is build using Visual Studio 2017.

Open the Solution lib/PidginWinToastLib.sln in Visual Studio 2017. Right click on the Project and hit Build.

Now the file PidginWinToastLib.dll should be in the Release folder of your Solution.
