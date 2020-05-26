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

	static size_t split(const std::wstring& str, const _TCHAR* cszDelimiters, STRINGLIST& result)
	{
		result.clear();
		size_t nPos = 0;
		if (cszDelimiters)
		{
			const size_t nDelimitersCount = _tcslen(cszDelimiters);
			const _TCHAR* pLastDelimiter = cszDelimiters + nDelimitersCount;
			while (nPos < str.length())
			{
				size_t nMinDelPos = std::wstring::npos;
				const _TCHAR* pDelimiter = cszDelimiters;
				while (pDelimiter < pLastDelimiter)
				{
					const size_t nDelPos = str.find(*pDelimiter, nPos);
					if (nDelPos != std::wstring::npos)
						if (nMinDelPos == std::wstring::npos || nMinDelPos > nDelPos)
							nMinDelPos = nDelPos;
					pDelimiter++;
				}
				if (nMinDelPos == std::wstring::npos)
				{
					result.push_back(str.substr(nPos));
					break;
				}
				else
				{
					result.push_back(str.substr(nPos, nMinDelPos - nPos));
					nPos = nMinDelPos + 1;
				}
			}
		}
		else
			result.push_back(str);

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

