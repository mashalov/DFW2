#pragma once
#include <stdexcept>
#include <string>
#include <codecvt>
#include <tchar.h>
#include <memory>

class dfw2error : public std::runtime_error
{
private:
	std::wstring m_Message;
	using utf8conv = std::wstring_convert<std::codecvt_utf8<wchar_t>>;
public:
	dfw2error(std::wstring& Message) : runtime_error(std::make_unique<utf8conv>()->to_bytes(Message)), m_Message(Message) {}
	dfw2error(dfw2error& err) : dfw2error(err.uwhat()) {}
	dfw2error(const _TCHAR* Message) : runtime_error(std::make_unique<utf8conv>()->to_bytes(Message)), m_Message(Message) {}
	const _TCHAR* uwhat()
	{
		return m_Message.c_str();
	}
};