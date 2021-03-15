#pragma once
#include <winuser.h>
#include <memory>
#include "Messages.h"
#include "stringutils.h"
#include "..\fmt\include\fmt\core.h"
#include "..\fmt\include\fmt\format.h"
#include <filesystem>

class CDFW2Exception
{
protected:
	std::string m_strMessage;
public:
	CDFW2Exception(std::string_view Description) : m_strMessage(Description) {}
	const char* Message() noexcept { return m_strMessage.c_str(); }
};

class CDFW2GetLastErrorException : public CDFW2Exception
{
public:
	CDFW2GetLastErrorException(std::string_view Description) : CDFW2Exception(Description)
	{
		std::string strGetLastErrorMsg = GetLastErrorMessage();
		if(!strGetLastErrorMsg.empty())
			m_strMessage += fmt::format(" {}", strGetLastErrorMsg);
		stringutils::removecrlf(m_strMessage);
	}

	static std::string GetLastErrorMessage()
	{
		const DWORD dwError = ::GetLastError();
		if (dwError != 0)
		{
			LPTSTR messageBuffer = nullptr;
			const size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&messageBuffer, 0, NULL);
			std::string message(stringutils::utf8_encode(std::wstring(messageBuffer, size)));
			LocalFree(messageBuffer);
			return message;
		}
		return std::string("");
	}
};


static const char* cszDoubleSlash = "\\\\";

static void NormalizePath(std::string& Path)
{
	stringutils::trim(Path);

	size_t nDoubleSlashIndex = Path.find(cszDoubleSlash);

	while (nDoubleSlashIndex != std::string::npos)
	{
		Path.erase(nDoubleSlashIndex, 1);
		nDoubleSlashIndex = Path.find(cszDoubleSlash);
	}

	while (Path.length() > 0)
	{
		if (Path.at(Path.length() - 1) == _T('\\'))
			Path.pop_back();
		else
			break;
	}
}

static bool CreateAllDirectories(std::string_view Dir)
{
	return std::filesystem::create_directories(Dir);
}

const static std::string GetDirectory(std::string_view Path)
{
	std::filesystem::path getPath(Path);
	getPath.remove_filename();
	return getPath.string();
}