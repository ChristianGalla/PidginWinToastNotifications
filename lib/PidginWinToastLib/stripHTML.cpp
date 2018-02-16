#include "stripHTML.h"

std::map<std::wstring, std::wstring> namedCharCodes {
    { L"&lt;", L"<" },
    { L"&gt;", L">" },
    { L"&amp;", L"&" },
    { L"&quot;", L"\"" },
    { L"&apos;", L"'" },
};

std::wstring replaceNamedCharCodes(std::wstring source) {
    std::wsmatch cm;
    if (std::regex_search(source, cm, std::wregex(L"&\\w+;"))) {
        std::map<std::wstring, std::wstring>::iterator it = namedCharCodes.find(cm[0]);
        if (it != namedCharCodes.end()) {
            std::wstring ret = cm.prefix().str() + namedCharCodes.find(cm[0])->second;
            if (cm.suffix().length() > 0) {
                ret += replaceNamedCharCodes(cm.suffix().str());
            }
            return ret;
        }
        else {
            return source;
        }
    }
    else {
        return source;
    }
}

// https://www.codeproject.com/Articles/11902/Convert-HTML-to-Plain-Text
std::wstring stripHTML(std::wstring source)
{
    std::wstring result = source;

    // Replace any white space characters(line breaks, tabs, spaces) with space because browsers inserts space
    // replaceAll(result, L"\s", L" ");
    result = std::regex_replace(source, std::wregex(L"\\s", std::regex::ECMAScript), L" ");
    // Remove repeating speces becuase browsers ignore them
    // replaceAll(result, L" {2,}", L" ");
    result = std::regex_replace(source, std::wregex(L" {2,}", std::regex::ECMAScript), L" ");
        
    // Remove HTML comment
    result = std::regex_replace(result, std::wregex(L"<! *--.*?-- *>", std::regex::ECMAScript), L"");
    // Remove the header
    result = std::regex_replace(result, std::wregex(L"< *head( *>| [^>]*>).*< */ *head *>", std::regex::ECMAScript | std::regex::icase), L"");
    // remove all scripts
    result = std::regex_replace(result, std::wregex(L"< *script( *>| [^>]*>).*?< */ *script *>", std::regex::ECMAScript | std::regex::icase), L"");
    // remove all styles (prepare first by clearing attributes)
    result = std::regex_replace(result, std::wregex(L"< *style( *>| [^>]*>).*?< */ *style *>", std::regex::ECMAScript | std::regex::icase), L"");
    // insert tabs in spaces of <td> tags
    result = std::regex_replace(result, std::wregex(L"< *td[^>]*>", std::regex::ECMAScript | std::regex::icase), L"\t");
    // insert line breaks in places of <BR> and <LI> tags
    result = std::regex_replace(result, std::wregex(L"< *(br|li) */{0,1} *>", std::regex::ECMAScript | std::regex::icase), L"\r");
    // insert line breaks
    result = std::regex_replace(result, std::wregex(L"< *(div|tr|p)( *>| [^>]*>)", std::regex::ECMAScript | std::regex::icase), L"\r\r");
    // Remove remaining tags like <a>, links, images, etc - anything thats enclosed inside < > 
    result = std::regex_replace(result, std::wregex(L"<[^>]*>", std::regex::ECMAScript | std::regex::icase), L"");
    // whitespace optimizations
    result = std::regex_replace(result, std::wregex(L"&nbsp;", std::regex::ECMAScript | std::regex::icase), L" ");
    // Remove extra line breaks and tabs:
    // Romove any whitespace and tab at and of any line
    result = std::regex_replace(result, std::wregex(L"[ \t]+\r", std::regex::ECMAScript | std::regex::icase), L"\r");
    // Remove whitespace beetween tabs
    result = std::regex_replace(result, std::wregex(L"\t +\t", std::regex::ECMAScript | std::regex::icase), L"\t\t");
    // Remove whitespace begining of a line if followed by a tab
    result = std::regex_replace(result, std::wregex(L"\r +\t", std::regex::ECMAScript | std::regex::icase), L"\r\t");
    // Remove multible tabs following a linebreak with just one tab 
    result = std::regex_replace(result, std::wregex(L"\r\t{2,}", std::regex::ECMAScript | std::regex::icase), L"\r\t");
    // replace over 2 breaks with 2 and over 4 tabs with 4. 
    result = std::regex_replace(result, std::wregex(L"\r{3,}", std::regex::ECMAScript | std::regex::icase), L"\r\r");
    result = std::regex_replace(result, std::wregex(L"\t{4,}", std::regex::ECMAScript | std::regex::icase), L"\t\t\t\t");

    result = replaceNamedCharCodes(result);
    return result;
}