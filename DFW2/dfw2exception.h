﻿#pragma once
#include <stdexcept>
#include <memory>

class dfw2error : public std::runtime_error
{
private:
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