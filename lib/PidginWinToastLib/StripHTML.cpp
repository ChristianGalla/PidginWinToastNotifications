#include "StripHTML.h"

void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to) {
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::wstring StripHTML(std::wstring source)
{
    std::wstring result;

    // Replace any white space characters(line breaks, tabs, spaces) with space because browsers inserts space
    replaceAll(source, L"\s", L" ");
    // Remove repeating speces becuase browsers ignore them
    replaceAll(source, L" {2,}", L" ");
        
    // Remove HTML comment
    result = std::regex_replace(result, std::wregex(L"<! *--.*?-- *>"), L"");
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

    std::wcmatch cm;
    std::regex_match(result, cm.str(), std::wregex(L"&\w+;"));

    // Replace named character codes
    for (size_t i = 0; i<cm.size(); i++) {
    {
        if (SpecChars.ContainsKey(AmpCodes[i].Value))
        {
            result = result.Substring(0, AmpCodes[i].Index) +
                Convert.ToChar(SpecChars[AmpCodes[i].Value]) +
                result.Substring(AmpCodes[i].Index + AmpCodes[i].Length);
        }
    }
    // Remove all others
    result = std::regex_replace(result, std::wregex(L"&[^;]*;", std::regex::ECMAScript | std::regex::icase), L"");
    return result;
}