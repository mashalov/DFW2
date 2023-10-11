#pragma once
#ifdef _MSC_VER
#include <Windows.h>
#include <stringapiset.h>
#endif 
#include <locale>
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

	// используем локаль для isspace для возможности работы в UTF-8
	static inline std::string& ltrim(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char ch) noexcept {
			return !std::isspace(ch, utf8locale);
			}));
		return s;
	}

	static inline std::string& rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](char ch) noexcept {
			return !std::isspace(ch, utf8locale);
			}).base(), s.end());
		return s;
	}

	static inline void tolower(std::string& s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	}

	static inline std::string& trim(std::string& s) { ltrim(s);  rtrim(s); return s; }
	static inline std::string ctrim(const std::string& s) { std::string st(s); return trim(st); }

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

	static std::string acp_encode(const std::string_view& str)
	{
#ifdef _MSC_VER
		std::wstring wstr{utf8_decode(str)};
		const auto size_needed{ WideCharToMultiByte(CP_ACP, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL) };
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_ACP, 0, &wstr[0], static_cast<int>(wstr.size()), &strTo[0], size_needed, NULL, NULL);
		return strTo;
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

	static std::locale GetLocaleUTF8()
	{
		std::locale  currentLocale{ std::locale() };
		std::string localeName{ currentLocale.name() };
		std::transform(localeName.begin(), localeName.end(), localeName.begin(), [](unsigned char c) { return std::tolower(c); });
		if (localeName.find("utf-8") == std::string::npos && localeName.find("utf8") == std::string::npos)
		{
			try
			{
				currentLocale = std::locale("en_US.UTF-8");
			}
			catch (const std::exception&)
			{
				try
				{
					currentLocale = std::locale("ru_RU.UTF-8");
				}
				catch (const std::exception&) {}
			}
		}
		return currentLocale;
	}

#ifdef _MSC_VER

	//! функции для доступа к выводу консоли
	//! Возвращает текущую кодовую страницу для консоли
	static UINT GetConsoleCodePage()
	{
		// если выполняемся уже в консоли, считываем
		// текущую кодовую страницу
		UINT acp{ GetConsoleOutputCP() };
		if (acp != 0)
			return acp;

		// если GetConsoleOutputCP() вернула ноль - это ошибка,
		// и консоли с процессом не связано. В этом случае
		// получаем кодовую страницу из текущей локали пользователя.
		// Рецепт: https://devblogs.microsoft.com/oldnewthing/20161007-00/?p=94475

		const int sizeInChars{ sizeof(acp) / sizeof(TCHAR) };

		if (GetLocaleInfo(GetUserDefaultLCID(),
			LOCALE_IDEFAULTCODEPAGE |
			LOCALE_RETURN_NUMBER,
			reinterpret_cast<LPTSTR>(&acp),
			sizeInChars) != sizeInChars)
			acp = 866; // и если что-то пошло не так с локалью, ставим русскую 866

		return acp;
	}

	//! Декодирует строку, полученную из консоли в unicode
	static std::wstring console_decode(const std::string_view& str)
	{
		if (str.empty()) return std::wstring();
		// получаем кодовую страницу консоли
		const auto ConsoleCodePage(GetConsoleCodePage());
		// подсчитываем размер буфера для кодовой страницы
		const int size_needed{ MultiByteToWideChar(ConsoleCodePage, 0, &str[0], (int)str.size(), NULL, 0) };
		std::wstring wstrTo(size_needed, 0);
		// и конвертируем кодовую страницу консоли в unicode
		MultiByteToWideChar(ConsoleCodePage, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}
#else
	// на linux функция ничего не делает и возвращает тот же std::string
	static std::string console_decode(const std::string_view& str)
	{
		return std::string(str);
	}
#endif


	static inline const std::locale utf8locale = GetLocaleUTF8();
};

