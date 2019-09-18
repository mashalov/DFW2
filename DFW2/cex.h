#pragma once

#include "winuser.h"
#include "stdio.h"
#include "tchar.h"
#include "Messages.h"
#include "string"
#include "list"
#include "algorithm"
#include "ShlObj.h"

using namespace std;

#define EXCEPTION_BUFFER_SIZE 2048


typedef std::list<std::wstring> STRINGLIST;
typedef STRINGLIST::iterator STRINGLISTITR;

class stringutils
{
public:
	static inline void removecrlf(wstring& s)
	{
		size_t nPos = wstring::npos;
		while ((nPos = s.find(_T("\r\n"))) != wstring::npos)
			s.replace(nPos, 2, _T(""));
	}

	static inline void ltrim(wstring &s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !isspace(ch); }));
	}

	static inline void rtrim(std::wstring &s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !isspace(ch); }).base(), s.end());
	}

	static inline void trim(std::wstring &s) { ltrim(s);  rtrim(s); }

	static size_t split(const wstring& str, const _TCHAR* cszDelimiters, STRINGLIST& result)
	{
		result.clear();
		size_t nPos = 0;
		if (cszDelimiters)
		{
			size_t nDelimitersCount = _tcslen(cszDelimiters);
			const _TCHAR* pLastDelimiter = cszDelimiters + nDelimitersCount;
			while (nPos < str.length())
			{
				size_t nMinDelPos = wstring::npos;
				const _TCHAR *pDelimiter = cszDelimiters;
				while (pDelimiter < pLastDelimiter)
				{
					size_t nDelPos = str.find(*pDelimiter, nPos);
					if (nDelPos != wstring::npos)
						if (nMinDelPos == wstring::npos || nMinDelPos > nDelPos)
							nMinDelPos = nDelPos;
					pDelimiter++;
				}
				if (nMinDelPos == wstring::npos)
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


class Cex
{
public:

	Cex(UINT nID, ...)
	{

		_TCHAR lpszFormat[512];
		int nCount = LoadString(GetModuleHandle(NULL), nID, lpszFormat, 512);
		// String is truncated to 511 characters
		m_szBuffer = nullptr;
		if (nCount)
		{
			m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
			*m_szBuffer = _T('\x0');
			va_list argList;
			va_start(argList, nID);
			_vsntprintf_s(m_szBuffer, EXCEPTION_BUFFER_SIZE - 1, _TRUNCATE, lpszFormat, argList);
			va_end(argList);
		}
	}

	Cex(Cex* pDummy, UINT nID, va_list argm)
	{
		_TCHAR lpszFormat[512];
		int nCount = LoadString(GetModuleHandle(NULL), nID, lpszFormat, 512);
		// String is truncated to 511 characters
		m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
		*m_szBuffer = _T('\x0');
		_vsntprintf_s(m_szBuffer, EXCEPTION_BUFFER_SIZE - 1, _TRUNCATE, lpszFormat, argm);
	}

	Cex(Cex* pDummy, const _TCHAR* lpszFormat, va_list argm)
	{
		m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
		*m_szBuffer = _T('\x0');
		_vsntprintf_s(m_szBuffer, EXCEPTION_BUFFER_SIZE - 1, _TRUNCATE, lpszFormat, argm);
	}

	Cex(const _TCHAR *lpszFormat, ...)
	{

		m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
		*m_szBuffer = _T('\x0');
		va_list argList;
		va_start(argList, lpszFormat);
		_vsntprintf_s(m_szBuffer, EXCEPTION_BUFFER_SIZE - 1, _TRUNCATE, lpszFormat, argList);
		va_end(argList);
	}

	Cex(const Cex *pCex)
	{
		m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
		_tcsncpy_s(m_szBuffer, EXCEPTION_BUFFER_SIZE - 1, pCex->m_szBuffer, _TRUNCATE);
	}

	virtual ~Cex()
	{
		if (m_szBuffer != nullptr)
			delete[] m_szBuffer;
		m_szBuffer = nullptr;
	}

	operator const _TCHAR*()
	{
		return m_szBuffer;
	}

protected:
	_TCHAR *m_szBuffer;
};


class CDFW2Exception
{
protected:
	std::wstring m_strMessage;
public:
	CDFW2Exception(const _TCHAR *cszMessage) : m_strMessage(cszMessage) {}
	const _TCHAR* Message()  { return m_strMessage.c_str(); }
};

class CDFW2GetLastErrorException : public CDFW2Exception
{
public:
	CDFW2GetLastErrorException(const _TCHAR *cszMessage) : CDFW2Exception(cszMessage)
	{
		wstring strGetLastErrorMsg = GetLastErrorMessage();
		if(!strGetLastErrorMsg.empty())
			m_strMessage += Cex(_T(" (%s)"), strGetLastErrorMsg.c_str());
		stringutils::removecrlf(m_strMessage);
	}

	static std::wstring GetLastErrorMessage()
	{
		DWORD dwError = ::GetLastError();
		if (dwError != 0)
		{
			LPTSTR messageBuffer = nullptr;
			size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
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

	while (nDoubleSlashIndex != wstring::npos)
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

static bool CreateAllDirectories(const _TCHAR *szDir)
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
			return wstring(szPath);
	}
	return wstring();
}