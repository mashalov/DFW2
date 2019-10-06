#pragma once
#include <stdexcept>
#include <string>
#include <codecvt>
#include <tchar.h>
#include <memory>

class dfw2error : public std::runtime_error
{
	using utf8conv = std::wstring_convert<std::codecvt_utf8<wchar_t>>;
public:
	dfw2error(std::wstring& Message) : runtime_error(std::make_unique<utf8conv>()->to_bytes(Message)) {}
	dfw2error(dfw2error& err) : runtime_error(err) {}
	dfw2error(const _TCHAR* Message) : runtime_error(std::make_unique<utf8conv>()->to_bytes(Message)) {}
	std::wstring uwhat()
	{
		return std::make_unique<utf8conv>()->from_bytes(what()).c_str();
	}
};