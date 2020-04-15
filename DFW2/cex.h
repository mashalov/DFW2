#pragma once

#include "winuser.h"
#include "stdio.h"
#include "tchar.h"
#include "Messages.h"
#include "string"
#include "list"
#include "algorithm"
#include "ShlObj.h"

#define EXCEPTION_BUFFER_SIZE 2048


typedef std::list<std::wstring> STRINGLIST;
typedef STRINGLIST::iterator STRINGLISTITR;

class stringutils
{
public:
	static inline void removecrlf(std::wstring& s)
	{
		size_t nPos = std::wstring::npos;
		while ((nPos = s.find(_T("\r\n"))) != std::wstring::npos)
			s.replace(nPos, 2, _T(""));
	}

	static inline void ltrim(std::wstring &s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) noexcept { return !isspace(ch); }));
	}

	static inline void rtrim(std::wstring &s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) noexcept { return !isspace(ch); }).base(), s.end());
	}

	static inline void trim(std::wstring &s) { ltrim(s);  rtrim(s); }

	static size_t split(const std::wstring& str, const _TCHAR* cszDelimiters, STRINGLIST& result)
	{
		result.clear();
		size_t nPos = 0;
		if (cszDelimiters)
		{
			const size_t nDelimitersCount = _tcslen(cszDelimiters);
			const _TCHAR* pLastDelimiter = cszDelimiters + nDelimitersCount;
			while (nPos < str.length())
			{
				size_t nMinDelPos = std::wstring::npos;
				const _TCHAR *pDelimiter = cszDelimiters;
				while (pDelimiter < pLastDelimiter)
				{
					const size_t nDelPos = str.find(*pDelimiter, nPos);
					if (nDelPos != std::wstring::npos)
						if (nMinDelPos == std::wstring::npos || nMinDelPos > nDelPos)
							nMinDelPos = nDelPos;
					pDelimiter++;
				}
				if (nMinDelPos == std::wstring::npos)
				{
					result.push_back(str.substr(nPos));
					break;
				}
				else
				{
					result.push_back(str.substr(nPos, nMinDelPos - nPos));
					nPos = nMinDelPos + 1;
				}
			}
		}
		else
			result.push_back(str);

		return result.size();
	}
};

using TCHARString = std::unique_ptr<_TCHAR[]>;

class Cex
{
public:

	Cex(UINT nID, ...)
	{

		_TCHAR lpszFormat[512];
		const int nCount = LoadString(GetModuleHandle(NULL), nID, lpszFormat, 512);
		// String is truncated to 511 characters
		m_szBuffer = nullptr;
		if (nCount)
		{
			m_szBuffer = std::make_unique<_TCHAR[]>(512);
			m_szBuffer[0] = _T('\x0');
			va_list argList;
			va_start(argList, nID);
			_vsntprintf_s(m_szBuffer.get(), EXCEPTION_BUFFER_SIZE - 1, _TRUNCATE, lpszFormat, argList);
			va_end(argList);
		}
	}

	Cex(Cex* pDummy, UINT nID, va_list argm)
	{
		_TCHAR lpszFormat[512];
		const int nCount = LoadString(GetModuleHandle(NULL), nID, lpszFormat, 512);
		// String is truncated to 511 characters
		m_szBuffer = std::make_unique<_TCHAR[]>(EXCEPTION_BUFFER_SIZE);
		m_szBuffer[0] = _T('\x0');
		_vsntprintf_s(m_szBuffer.get(), EXCEPTION_BUFFER_SIZE - 1, _TRUNCATE, lpszFormat, argm);
	}

	Cex(Cex* pDummy, const _TCHAR* lpszFormat, va_list argm)
	{
		m_szBuffer = std::make_unique<_TCHAR[]>(EXCEPTION_BUFFER_SIZE);
		m_szBuffer[0] = _T('\x0');
		_vsntprintf_s(m_szBuffer.get(), EXCEPTION_BUFFER_SIZE - 1, _TRUNCATE, lpszFormat, argm);
	}

	Cex(const _TCHAR *lpszFormat, ...)
	{

		m_szBuffer = std::make_unique<_TCHAR[]>(EXCEPTION_BUFFER_SIZE);
		m_szBuffer[0] = _T('\x0');
		va_list argList;
		va_start(argList, lpszFormat);
		_vsntprintf_s(m_szBuffer.get(), EXCEPTION_BUFFER_SIZE - 1, _TRUNCATE, lpszFormat, argList);
		va_end(argList);
	}

	Cex(const Cex *pCex)
	{
		m_szBuffer = std::make_unique<_TCHAR[]>(EXCEPTION_BUFFER_SIZE);
		_tcsncpy_s(m_szBuffer.get(), EXCEPTION_BUFFER_SIZE - 1, pCex->m_szBuffer.get(), _TRUNCATE);
	}

	operator const _TCHAR*() noexcept
	{
		return m_szBuffer.get();
	}

protected:
	TCHARString m_szBuffer;
};


class CDFW2Exception
{
protected:
	std::wstring m_strMessage;
public:
	CDFW2Exception(const _TCHAR *cszMessage) : m_strMessage(cszMessage) {}
	const _TCHAR* Message() noexcept { return m_strMessage.c_str(); }
};

class CDFW2GetLastErrorException : public CDFW2Exception
{
public:
	CDFW2GetLastErrorException(const _TCHAR *cszMessage) : CDFW2Exception(cszMessage)
	{
		std::wstring strGetLastErrorMsg = GetLastErrorMessage();
		if(!strGetLastErrorMsg.empty())
			m_strMessage += Cex(_T(" (%s)"), strGetLastErrorMsg.c_str());
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