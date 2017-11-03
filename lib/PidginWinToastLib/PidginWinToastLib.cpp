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

class WinToastHandlerExample : public IWinToastHandler {
public:
	WinToastHandlerExample();
	// Public interfaces
	void toastActivated() const;
	void toastDismissed(WinToastDismissalReason state) const;
	void toastFailed() const;
};

WinToastHandlerExample::WinToastHandlerExample() {
}

void WinToastHandlerExample::toastActivated() const {
}

void WinToastHandlerExample::toastDismissed(WinToastDismissalReason state) const {
}

void WinToastHandlerExample::toastFailed() const {
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

extern "C" int PIDGINWINTOASTLIB_API pidginWinToastLibShowMessage(const char * sender, const char * message, const char * imagePath)
{
	if (isInit) {
		try {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			WinToastHandlerExample* handler = new WinToastHandlerExample;
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