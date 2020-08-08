#pragma once
#include <stdexcept>
#include <memory>
#include "..\fmt\include\fmt\core.h"
#include "..\fmt\include\fmt\format.h"
#include <system_error>

class dfw2error : public std::runtime_error
{
protected:
	std::wstring m_Message;
public:

	dfw2error(std::wstring& Message) : runtime_error(stringutils::utf8_encode(Message)), m_Message(Message) {}
	dfw2error(std::wstring_view Message) : runtime_error(stringutils::utf8_encode(Message)), m_Message(Message) {}
	dfw2error(const _TCHAR* Message) : runtime_error(stringutils::utf8_encode(Message)), m_Message(Message) {}
	dfw2error(dfw2error& err) : dfw2error(err.uwhat()) {}
	const _TCHAR* uwhat()
	{
		return m_Message.c_str();
	}
};

// добавляет к строке исключение информацию от GetLastError
class dfw2errorGLE : public dfw2error
{
public:
	dfw2errorGLE(std::wstring& Message) : dfw2error(MessageFormat(Message)) {}
	dfw2errorGLE(std::wstring_view Message) : dfw2error(MessageFormat(Message)) {}
	dfw2errorGLE(const _TCHAR* Message) : dfw2error(MessageFormat(Message)) {}
	dfw2errorGLE(dfw2error& err) : dfw2error(err.uwhat()) {}

	static std::wstring MessageFormat(std::wstring_view Message)
	{
		std::error_code code(::GetLastError(), std::system_category());
		// Описание ошибки от code приходит в CP_ACP, поэтому его декодируем с помощью
		// утилиты acp_decode
	    // https://blogs.msmvps.com/gdicanio/2017/08/16/what-is-the-encoding-used-by-the-error_code-message-string/
		return fmt::format(_T("{} Системная ошибка № {}: \"{}\""), Message, code.value(), stringutils::acp_decode(code.message()));
	}
};