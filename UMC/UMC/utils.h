#pragma once
#include <locale>
#define NOMINMAX
#ifdef _MSC_VER
#include <Windows.h>
#endif

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T>
std::string to_string(const T& t)
{
    std::string str{ fmt::format("{}", t) };
    int offset{ 1 };
    if (str.find_last_not_of('0') == str.find('.'))
    {
        offset = 0;
    }
    str.erase(str.find_last_not_of('0') + offset, std::string::npos);
    return str;
}

static char localestring[] = "en_US.UTF8";

// используем локаль для isspace для возможности работы в UTF-8
static inline std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) noexcept { return !std::isspace(ch, std::locale(localestring)); }));
    return s;
}

static inline std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) noexcept { return !std::isspace(ch, std::locale(localestring)); }).base(), s.end());
    return s;
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

	static std::string utf8_encode(const wchar_t *wstr)
	{
#ifdef _MSC_VER
		return utf8_encode(std::wstring_view(wstr));
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

#ifdef _MSC_VER		
	static std::wstring utf8_decode(const std::string_view& str)
	{
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}
#else
	// на linux функция ничего не делает и возвращает тот же std::string
	static std::string utf8_decode(const std::string_view& str)
	{
		return std::string(str);
	}
#endif

static inline std::string& trim(std::string& s) { ltrim(s);  rtrim(s); return s; }
static inline std::string ctrim(const std::string& s) { std::string st(s); return trim(st); }

#ifdef _MSC_VER
#define EXCEPTIONMSG(x) { _ASSERTE(!(x)); throw std::runtime_error(fmt::format("{} {} in {} {}", __FUNCSIG__, (x), __FILE__, __LINE__));}
#else
#define EXCEPTIONMSG(x) { throw std::runtime_error(fmt::format("{} {} in {} {}", __PRETTY_FUNCTION__, (x), __FILE__, __LINE__));}
#endif
#define EXCEPTION  EXCEPTIONMSG("");

#ifndef _MSC_VER
#define _ASSERTE(x)
#endif
