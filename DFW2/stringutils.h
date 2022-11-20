#pragma once
#ifdef _MSC_VER
#include <Windows.h>
#include <stringapiset.h>
#endif 
#include <list>
#include <algorithm>
#include <string>

using STRINGLIST = std::list<std::string>;

class stringutils
{
public:
	static inline void removecrlf(std::string& s)
	{
		s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
		s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
	}

	static inline void ltrim(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) noexcept { return !std::isspace(ch); }));
	}

	static inline void rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) noexcept { return !std::isspace(ch); }).base(), s.end());
	}

	static inline void tolower(std::string& s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	}

	static inline void trim(std::string& s) { ltrim(s);  rtrim(s); }

	template<typename T>
	static std::string join(const T& container, const std::string::value_type Delimiter = ',')
	{
		std::string result;
		for (auto it = container.begin(); it != container.end(); it++)
		{
			if (it != container.begin())
				result.push_back(Delimiter);
			result.append(*it);
		}
		return result;
	}

	template<typename T>
	static size_t split(std::string_view str, T& result, std::string_view Delimiters = ",;")
	{
		result.clear();
		for(size_t nPos(0); nPos < str.length() ; )
		{
			if (const auto nMinDelPos{ str.find_first_of(Delimiters, nPos) }; nMinDelPos == std::string::npos)
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
		const auto size_needed{ WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL) };
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &strTo[0], size_needed, NULL, NULL);
		return strTo;
#else
		return std::string(); // nothing to convert on linux
#endif
	}

#ifdef _MSC_VER
	static std::string COM_decode(const std::wstring_view wstr)
	{
		if (wstr.empty()) return std::string();
		const auto size_needed{ WideCharToMultiByte(CP_ACP, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL) };
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_ACP, 0, &wstr[0], static_cast<int>(wstr.size()), &strTo[0], size_needed, NULL, NULL);
		return strTo;
		return std::string();
	}
#else
	static std::string COM_decode(const std::string_view str)
	{
		return std::string(str);
	}
#endif 

#ifdef _MSC_VER
	static std::wstring COM_encode(const std::string_view& str)
	{
		if (str.empty()) return std::wstring();
		const auto size_needed{ MultiByteToWideChar(CP_ACP, 0, &str[0], static_cast<int>(str.size()), NULL, 0) };
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_ACP, 0, &str[0], static_cast<int>(str.size()), &wstrTo[0], size_needed);
		return wstrTo;
	}
#else
	static std::string COM_encode(const std::string_view& str)
	{
		return std::string(str);
	}
#endif

	static std::string utf8_encode(const wchar_t *wstr)
	{
		if (!wstr)
			return {};
#ifdef _MSC_VER
		return stringutils::utf8_encode(std::wstring_view(wstr));
#else
		return std::string(); // nothing to convert on linux
#endif
	}
#ifndef _MSC_VER	
	static std::string utf8_encode(const char *str)
	{
		return std::string(str);
	}

	static std::string utf8_encode(const std::string& str)
	{
		return std::string(str);
	}
#endif	

	static std::string acp_decode(const std::string_view& str)
	{
#ifdef _MSC_VER
		if (str.empty()) return std::string();
		const auto size_needed{ MultiByteToWideChar(CP_ACP, 0, &str[0], static_cast<int>(str.size()), NULL, 0) };
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_ACP, 0, &str[0], static_cast<int>(str.size()), &wstrTo[0], size_needed);
		return utf8_encode(wstrTo);
#else
		return std::string(str); // nothing to convert on linux
#endif
	}

#ifdef _MSC_VER		
	static std::wstring utf8_decode(const std::string_view& str)
	{
		if (str.empty()) return std::wstring();
		const auto size_needed{ MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), NULL, 0) };
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &wstrTo[0], size_needed);
		return wstrTo;
	}
#else
	// на linux функция ничего не делает и возвращает тот же std::string
	static std::string utf8_decode(const std::string_view& str)
	{
		return std::string(str);
	}
#endif

	// возвращает строку из массива строк strArray,
	// соответствующую e из перечисления T или "???" если
	// e >= size(strArray)
	template <typename T, std::size_t N>
	static const char* enum_text(const T e, const char* const (&strArray)[N])
	{
		const auto nx{ static_cast<typename std::underlying_type<T>::type>(e) };
		if (nx >= 0 && nx < N)
			return strArray[nx];
		else
			return "???";
	}
};

