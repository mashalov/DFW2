#pragma once
#include <stdexcept>
#include <memory>
#include "stringutils.h"
#include "fmt/core.h"
#include "fmt/format.h"
#include <system_error>

class dfw2error : public std::runtime_error
{
protected:
	mutable std::wstring _whatw;
public:
	dfw2error(std::string& Message) : runtime_error(Message) {}
	dfw2error(std::string_view Message) : runtime_error(std::string(Message)) {}
	dfw2error(const wchar_t* Message) : runtime_error(stringutils::utf8_encode(Message)) {}
	dfw2error(dfw2error& err) : dfw2error(err.what()) {}
#ifdef _MSC_VER	
	const wchar_t* whatw() const {	return (_whatw = stringutils::utf8_decode(what())).c_str();  }
#endif	
};

// добавляет к строке исключение информацию от GetLastError
class dfw2errorGLE : public dfw2error
{
public:
	dfw2errorGLE(std::string& Message) : dfw2error(MessageFormat(Message)) {}
	dfw2errorGLE(std::string_view Message) : dfw2error(MessageFormat(Message)) {}
	dfw2errorGLE(const wchar_t* Message) : dfw2error(MessageFormat(stringutils::utf8_encode(Message))) {}
	dfw2errorGLE(dfw2error& err) : dfw2error(err.what()) {}

	static std::string MessageFormat(std::string_view Message)
	{
		std::string message;
#ifdef _MSC_VER
		std::error_code code(::GetLastError(), std::system_category());
		const DWORD dwError(::GetLastError());
		if (dwError != 0)
		{
			LPTSTR messageBuffer = nullptr;
			const size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&messageBuffer, 0, NULL);
			std::wstring wmessage(messageBuffer, size);
			LocalFree(messageBuffer);
			message = stringutils::utf8_encode(wmessage);
		}

#else
		std::error_code code(errno, std::system_category());
		message = stringutils::acp_decode(code.message());
#endif 
		// Описание ошибки от code приходит в CP_ACP, поэтому его декодируем с помощью
		// утилиты acp_decode
	    // https://blogs.msmvps.com/gdicanio/2017/08/16/what-is-the-encoding-used-by-the-error_code-message-string/
		stringutils::removecrlf(message);
		return fmt::format("{} Системная ошибка № {}: \"{}\"", Message, code.value(), message);
	}
};