#pragma once
#include "list"
#include "algorithm"
#include "string"

typedef std::list<std::wstring> STRINGLIST;
typedef STRINGLIST::iterator STRINGLISTITR;

class stringutils
{
public:
	static inline void removecrlf(std::wstring& s)
	{
		size_t nPos = std::wstring::npos;
		while ((nPos = s.find(_T("\r\n"))) != std::wstring::npos)
			s.replace(nPos, 2, _T(""));
	}

	static inline void ltrim(std::wstring& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) noexcept { return !isspace(ch); }));
	}

	static inline void rtrim(std::wstring& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) noexcept { return !isspace(ch); }).base(), s.end());
	}

	static inline void trim(std::wstring& s) { ltrim(s);  rtrim(s); }

	static size_t split(std::wstring_view str, std::wstring_view Delimiters, STRINGLIST& result)
	{
		result.clear();
		for(size_t nPos(0); nPos < str.length() ; )
		{
			if (size_t nMinDelPos = str.find_first_of(Delimiters, nPos); nMinDelPos == std::wstring::npos)
			{
				result.push_back(std::wstring(str.substr(nPos)));
				break;
			}
			else
			{
				result.push_back(std::wstring(str.substr(nPos, nMinDelPos - nPos)));
				nPos = nMinDelPos + 1;
			}
		}
		return result.size();
	}

	static std::string utf8_encode(const std::wstring& wstr)
	{
		if (wstr.empty()) return std::string();
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	static std::wstring utf8_decode(const std::string& str)
	{
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

};

