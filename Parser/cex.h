#pragma once

#include "winuser.h"
#include "stdio.h"
#include "tchar.h"
#include "Messages.h"
#include "string"
#include "algorithm"

using namespace std;

#define EXCEPTION_BUFFER_SIZE 2048

	class Cex
	{
	public:

		Cex (UINT nID,...)
		{

			_TCHAR lpszFormat[512];
			int nCount = LoadString(GetModuleHandle(NULL),nID,lpszFormat,512);
			// String is truncated to 511 characters
			m_szBuffer = NULL;
			if(nCount) 
			{
				m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
				*m_szBuffer = _T('\x0');
				va_list argList;
				va_start(argList, nID);
				_vsntprintf_s(m_szBuffer,EXCEPTION_BUFFER_SIZE-1,_TRUNCATE,lpszFormat,argList);
				va_end(argList);
			}
		}

		Cex(Cex* pDummy, UINT nID, va_list argm)
		{
			_TCHAR lpszFormat[512];
			int nCount = LoadString(GetModuleHandle(NULL),nID,lpszFormat,512);
			// String is truncated to 511 characters
			m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
			*m_szBuffer = _T('\x0');
			_vsntprintf_s(m_szBuffer,EXCEPTION_BUFFER_SIZE-1,_TRUNCATE,lpszFormat,argm);
		}

		Cex(Cex* pDummy, const _TCHAR* lpszFormat, va_list argm)
		{
			m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
			*m_szBuffer = _T('\x0');
			_vsntprintf_s(m_szBuffer,EXCEPTION_BUFFER_SIZE-1,_TRUNCATE,lpszFormat,argm);
		}

		Cex(const _TCHAR *lpszFormat,...)
		{
			
			m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
			*m_szBuffer = _T('\x0');
			va_list argList;
			va_start(argList, lpszFormat);
			_vsntprintf_s(m_szBuffer,EXCEPTION_BUFFER_SIZE-1,_TRUNCATE,lpszFormat,argList);
			va_end(argList);
		}

		Cex(const Cex *pCex)
		{
			m_szBuffer = new _TCHAR[EXCEPTION_BUFFER_SIZE];
			_tcsncpy_s(m_szBuffer,EXCEPTION_BUFFER_SIZE-1,pCex->m_szBuffer,_TRUNCATE);
		}

		virtual ~Cex()
		{
			if( m_szBuffer != NULL )
				delete [] m_szBuffer;
			m_szBuffer = NULL;
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
			DWORD dwError = ::GetLastError();
			if (dwError != 0)
			{
				LPTSTR messageBuffer = NULL;
				size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&messageBuffer, 0, NULL);
				std::wstring message(messageBuffer, size);
				LocalFree(messageBuffer);
				m_strMessage += Cex(_T(" (%s)"),message.c_str());
			}
		}
	};
	

	static inline void ltrim(wstring &s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !isspace(ch); }));
	}

	static inline void rtrim(std::wstring &s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !isspace(ch); }).base(), s.end());
	}

	static inline void trim(std::wstring &s) { ltrim(s);  rtrim(s); }