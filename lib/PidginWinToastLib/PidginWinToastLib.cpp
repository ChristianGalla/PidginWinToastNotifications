// PidginWinToastLib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "PidginWinToastLib.h"

using namespace std;
using namespace WinToastLib;

bool isInit = false;
void(*clickCallback)(void *conv) = nullptr;

std::wstring getTextFromHtml(std::wstring);

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
    void *conv = nullptr;
	WinToastHandler(void *conv = nullptr);
	// Public interfaces
	void toastActivated() const;
    void toastActivated(int actionIndex) const;
	void toastDismissed(WinToastDismissalReason state) const;
	void toastFailed() const;
};

WinToastHandler::WinToastHandler(void *conv): 
    conv(conv) {
}

void WinToastHandler::toastActivated() const {
    if (clickCallback != nullptr && this->conv != nullptr) {
        clickCallback(this->conv);
    }
}

void WinToastHandler::toastActivated(int actionIndex) const
{
    if (clickCallback != nullptr && this->conv != nullptr) {
        clickCallback(this->conv);
    }
}

void WinToastHandler::toastDismissed(WinToastDismissalReason state) const {
}

void WinToastHandler::toastFailed() const {
}

extern "C" int pidginWinToastLibInit(void(*_clickCallback)(void *conv))
{
    clickCallback = _clickCallback;
	try {
		WinToast::instance()->setAppName(L"Pidgin");
		WinToast::instance()->setAppUserModelId(
			WinToast::configureAUMI(L"Pidgin", L"Pidgin"));

        WinToast::WinToastError error;
        isInit = WinToast::instance()->initialize(&error);
		return error;
	}
	catch (...) {
		// unknown exception
		return -1;
	}
}

extern "C" int pidginWinToastLibShowMessage(const char * sender, const char * message, const char * imagePath, const char * protocolName, void *conv)
{
	if (isInit) {
        if (sender == nullptr) {
            return WinToast::WinToastError::InvalidParameters;
        }
		try {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			WinToastHandler* handler = new WinToastHandler(conv);
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
            sMessage = stripHTML(sMessage);
			templ.setTextField(sMessage, WinToastTemplate::SecondLine);

            if (protocolName != NULL) {
                std::wstring sProtocolName = converter.from_bytes(protocolName);
                templ.setAttributionText(sProtocolName);
            }

            WinToast::WinToastError error;
			INT64 toastResult = WinToast::instance()->showToast(templ, handler, &error);
			
            return error;
		}
		catch (...) {
			// unknown exception
			return -1;
		}
	}
	else {
		return WinToast::WinToastError::NotInitialized;
	}
}