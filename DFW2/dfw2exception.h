#pragma once
#include <stdexcept>
#include <memory>
#include "utf8conv.h"

class dfw2error : public std::runtime_error
{
private:
	std::wstring m_Message;
public:

	dfw2error(std::wstring& Message) : runtime_error(utf8_encode(Message)), m_Message(Message) {}
	dfw2error(dfw2error& err) : dfw2error(err.uwhat()) {}
	dfw2error(const _TCHAR* Message) : runtime_error(utf8_encode(Message)), m_Message(Message) {}

	const _TCHAR* uwhat()
	{
		return m_Message.c_str();
	}
};