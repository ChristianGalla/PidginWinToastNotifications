// PidginWinToastLib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "PidginWinToastLib.h"

using namespace std;
using namespace WinToastLib;

bool isInit = false;

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

extern "C" int pidginWinToastLibShowMessage(const char * sender, const char * message, const char * imagePath, const char * protocolName)
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

            if (protocolName != NULL) {
                std::wstring sProtocolName = converter.from_bytes(protocolName);
                templ.setAttributionText(sProtocolName);
            }


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

std::wstring getTextFromHtml(std::wstring html) {
    std::stack<std::wstring> tags;
    std::wstringstream ret;

    size_t startPos = 0;
    size_t endPos = 0;
    bool inTag = false;
    bool searchTagEnd = false;
    wchar_t last = L'';
    for (size_t pos = 0; pos < html.length(); pos++) {
        wchar_t c = html[pos];
        if (c == L'\n' || c == L'\r' || c == L'\t') {
            c = L' ';
            if (last == L' ') {
                continue;
            }
        }

        if (c == L'<') {
            // In start tag, end tag or comment
            startPos = pos + 1;
            if (pos+1 < html.length()) {
                pos++;
                c = html[pos];
                if (c == L'!' && pos < html.length()) {
                    // comment <!-- -->

                }
                else if (c == L'!') {
                }
            }
            inTag = true;
        } else if (inTag && (c == L'>' || c == L' ' || c == L'/')) {
            if (startPos == pos && c == L'/') {
                // in endtag
            }
            endPos = pos - 1;
            inTag = false;
            if (endPos > startPos) {
                tags.push(html.substr(startPos, endPos - startPos));
            }
        }
        last = c;
    }
    ret;
    return ret.str();
}