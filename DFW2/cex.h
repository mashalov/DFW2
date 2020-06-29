#pragma once

#include "winuser.h"
#include "stdio.h"
#include "tchar.h"
#include "Messages.h"
#include "ShlObj.h"
#include "memory"
#include "stringutils.h"
#include "..\fmt\include\fmt\core.h"
#include "..\fmt\include\fmt\format.h"

class CDFW2Exception
{
protected:
	std::wstring m_strMessage;
public:
	CDFW2Exception(std::wstring_view Description) : m_strMessage(Description) {}
	const _TCHAR* Message() noexcept { return m_strMessage.c_str(); }
};

class CDFW2GetLastErrorException : public CDFW2Exception
{
public:
	CDFW2GetLastErrorException(std::wstring_view Description) : CDFW2Exception(Description)
	{
		std::wstring strGetLastErrorMsg = GetLastErrorMessage();
		if(!strGetLastErrorMsg.empty())
			m_strMessage += fmt::format(_T(" {}"), strGetLastErrorMsg.c_str());
		stringutils::removecrlf(m_strMessage);
	}

	static std::wstring GetLastErrorMessage()
	{
		const DWORD dwError = ::GetLastError();
		if (dwError != 0)
		{
			LPTSTR messageBuffer = nullptr;
			const size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&messageBuffer, 0, NULL);
			std::wstring message(messageBuffer, size);
			LocalFree(messageBuffer);
			return message;
		}
		return std::wstring(_T(""));
	}
};


static const _TCHAR* cszDoubleSlash = _T("\\\\");

static void NormalizePath(std::wstring& Path)
{
	stringutils::trim(Path);

	size_t nDoubleSlashIndex = Path.find(cszDoubleSlash);

	while (nDoubleSlashIndex != std::wstring::npos)
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

static bool CreateAllDirectories(const _TCHAR *szDir) noexcept
{
	switch (SHCreateDirectoryEx(NULL, szDir, NULL))
	{
	case ERROR_SUCCESS:
	case ERROR_FILE_EXISTS:
	case ERROR_ALREADY_EXISTS:
		return true;
	}

	return false;
}

const static std::wstring GetDirectory(const _TCHAR *cszPath)
{
	_TCHAR Drv[_MAX_DRIVE];
	_TCHAR Dir[_MAX_DIR];
	_TCHAR FileName[_MAX_FNAME];
	_TCHAR Ext[_MAX_EXT];
	_TCHAR szPath[_MAX_PATH];
	if (!_tsplitpath_s(cszPath, Drv, _MAX_DRIVE, Dir, _MAX_DIR, FileName, _MAX_FNAME, Ext, _MAX_EXT))
	{
		if (!_tmakepath_s(szPath, _MAX_PATH, Drv, Dir, NULL, NULL))
			return std::wstring(szPath);
	}
	return std::wstring();
}