#pragma once
#include <stdexcept>
#include <memory>
#include "..\fmt\include\fmt\core.h"
#include "..\fmt\include\fmt\format.h"
#include <system_error>

class dfw2error : public std::runtime_error
{
protected:
public:

	dfw2error(std::string& Message) : runtime_error(Message) {}
	dfw2error(std::string_view Message) : runtime_error(std::string(Message)) {}
	dfw2error(const wchar_t* Message) : runtime_error(stringutils::utf8_encode(Message)) {}
	dfw2error(dfw2error& err) : dfw2error(err.what()) {}
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
		std::error_code code(::GetLastError(), std::system_category());
		// Описание ошибки от code приходит в CP_ACP, поэтому его декодируем с помощью
		// утилиты acp_decode
	    // https://blogs.msmvps.com/gdicanio/2017/08/16/what-is-the-encoding-used-by-the-error_code-message-string/
		return fmt::format("{} Системная ошибка № {}: \"{}\"", Message, code.value(), stringutils::acp_decode(code.message()));
	}
};