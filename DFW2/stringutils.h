#pragma once
#include "list"
#include "algorithm"
#include "string"

typedef std::list<std::string> STRINGLIST;
typedef STRINGLIST::iterator STRINGLISTITR;

class stringutils
{
public:
	static inline void removecrlf(std::string& s)
	{
		size_t nPos = std::string::npos;
		while ((nPos = s.find("\r\n")) != std::string::npos)
			s.replace(nPos, 2, "");
	}

	static inline void ltrim(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) noexcept { return !isspace(ch); }));
	}

	static inline void rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) noexcept { return !isspace(ch); }).base(), s.end());
	}

	static inline void trim(std::string& s) { ltrim(s);  rtrim(s); }

	static size_t split(std::string_view str, std::string_view Delimiters, STRINGLIST& result)
	{
		result.clear();
		for(size_t nPos(0); nPos < str.length() ; )
		{
			if (size_t nMinDelPos = str.find_first_of(Delimiters, nPos); nMinDelPos == std::string::npos)
			{
				result.push_back(std::string(str.substr(nPos)));
				break;
			}
			else
			{
				result.push_back(std::string(str.substr(nPos, nMinDelPos - nPos)));
				nPos = nMinDelPos + 1;
			}
		}
		return result.size();
	}

	static std::string utf8_encode(const std::wstring_view& wstr)
	{
#ifdef _MSC_VER
		if (wstr.empty()) return std::string();
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
#else
		return std::string(); // nothing to convert on linux
#endif
	}

	static std::string acp_decode(const std::string_view& str)
	{
#ifdef _MSC_VER
		if (str.empty()) return std::string();
		int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return utf8_encode(wstrTo);
#else
		return std::string(); // nothing to convert on linux
#endif
	}

	static std::wstring utf8_decode(const std::string_view& str)
	{
#ifdef _MSC_VER		
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
#else
		return std::wstring();  // nothing to convert on linux
#endif
	}

};

