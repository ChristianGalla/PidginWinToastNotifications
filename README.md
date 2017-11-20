# PidginWinToastNotifications

Windows toast notification plugin for Pidgin.

Shows notifications on message receive in the standard Windows design.

![sample notification](/img/sample.png "sample notification")

This plugin uses Pidgin's standard C API so no other tools like Perl are required.

## Supported operating systems

The plugin was developed and tested with the newest Windows 10 version 1709.

Other Windows 10 versions, Windows 8 and Windows 8.1 should work, too, but this wasn't tested.
If you experience any problems, feel free to open an issue.

## Installation

1.  Download the dll files from the [newest release](https://github.com/ChristianGalla/PidginWinToastNotifications/releases/latest).
2. Copy the file PidginWinToastLib.dll to the installation directory of Pidgin. This is usually "C:\Program Files (x86)\Pidgin" on 64-bit versions of Windows or "C:\Program Files\Pidgin" on a 32-bit version.
3. Copy the file PidginWinToastNotifications.dll to the plugin directory of Pidgin. This is usually "C:\Program Files (x86)\Pidgin\plugins" on 64-bit versions of Windows or "C:\Program Files\Pidgin\plugins" on a 32-bit version.
4. Start Pidgin. Open the plugin window and enable "Windows Toast Notifications".