// PidginWinToastLib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "PidginWinToastLib.h"

using namespace std;
using namespace WinToastLib;

bool isInit = false;

//char* copyCharArrayToHeap(char* source) {
//	size_t size = strlen(source) + 1;
//	char* dest = (char*)malloc(size);
//	if (dest != nullptr) {
//		strcpy(dest, source);
//	}
//	return dest;
//}

class WinToastHandler : public IWinToastHandler {
public:
	WinToastHandler();
	// Public interfaces
	void toastActivated() const;
    void toastActivated(int actionIndex) const;
	void toastDismissed(WinToastDismissalReason state) const;
	void toastFailed() const;
};

WinToastHandler::WinToastHandler() {
}

void WinToastHandler::toastActivated() const {
}

void WinToastHandler::toastActivated(int actionIndex) const
{
}

void WinToastHandler::toastDismissed(WinToastDismissalReason state) const {
}

void WinToastHandler::toastFailed() const {
}

extern "C" int pidginWinToastLibInit()
{
	try {
		WinToast::instance()->setAppName(L"Pidgin");
		WinToast::instance()->setAppUserModelId(
			WinToast::configureAUMI(L"GallaChristian", L"Pidgin"));

		bool initSuccess = WinToast::instance()->initialize();
		if (!initSuccess) {
			// Could not initialize the lib
			return 1;
		}
		isInit = true;
		return 0;
	}
	catch (...) {
		// unknown exception
		return -1;
	}
}

extern "C" int pidginWinToastLibShowMessage(const char * sender, const char * message, const char * imagePath)
{
	if (isInit) {
		try {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			WinToastHandler* handler = new WinToastHandler();
			WinToastTemplate templ;
			std::wstring sImagePath;

			if (imagePath != NULL) {
				templ = WinToastTemplate(WinToastTemplate::ImageAndText02);
				sImagePath = converter.from_bytes(imagePath);
				templ.setImagePath(sImagePath);
			}
			else {
				templ = WinToastTemplate(WinToastTemplate::Text02);
			}

			std::wstring sSender = converter.from_bytes(sender);
			templ.setTextField(sSender, WinToastTemplate::FirstLine);
			std::wstring sMessage = converter.from_bytes(message);
			templ.setTextField(sMessage, WinToastTemplate::SecondLine);


			INT64 toastResult = WinToast::instance()->showToast(templ, handler);
			if (toastResult == -1) {
				// Could not launch toast notification
				return 2;
			}

			return 0;
		}
		catch (...) {
			// unknown exception
			return -1;
		}
	}
	else {
		// not initialized
		return 1;
	}
}