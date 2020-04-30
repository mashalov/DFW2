// AutomaticCompiler.cpp : Implementation of CAutomaticCompiler

#include "stdafx.h"
#include "AutomaticCompiler.h"


// CAutomaticCompiler

STDMETHODIMP CAutomaticCompiler::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IAutomaticCompiler
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CAutomaticCompiler::AddStarter(LONG Type, LONG Id, BSTR Name, BSTR Expression, LONG LinkType, BSTR ObjectClass, BSTR ObjectKey, BSTR ObjectProp)
{
	HRESULT hr = E_INVALIDARG;
	CAutoItem AutoItem(Type, Id, Name, Expression);
	CCompilerModelLink ModelLink(LinkType, ObjectClass, ObjectKey, ObjectProp);
	CAutoStarterItem StarterItem(AutoItem, ModelLink);
	if (m_pAutoCompiler->AddStarter(StarterItem))
	{
		hr = S_OK;
	}
	return hr;
}


STDMETHODIMP CAutomaticCompiler::AddLogic(LONG Type, LONG Id, BSTR Name, BSTR Expression, BSTR Actions, BSTR DelayExpression, LONG OutputMode)
{
	HRESULT hr = E_INVALIDARG;
	CAutoItem AutoItem(Type, Id, Name, Expression);
	CAutoLogicItem LogicItem(AutoItem, Actions, DelayExpression, OutputMode);
	if (m_pAutoCompiler->AddLogic(LogicItem))
	{
		hr = S_OK;
	}
	return hr;

}

STDMETHODIMP CAutomaticCompiler::AddAction(LONG Type, LONG Id, BSTR Name, BSTR Expression, LONG LinkType, BSTR ObjectClass, BSTR ObjectKey, BSTR ObjectProp, LONG ActionGroup, LONG OutputMode, LONG RunsCount)
{
	HRESULT hr = E_INVALIDARG;
	CAutoItem AutoItem(Type, Id, Name, Expression);
	CCompilerModelLink ModelLink(LinkType, ObjectClass, ObjectKey, ObjectProp);
	CAutoActionItem ActionItem(AutoItem, ModelLink,ActionGroup,OutputMode,RunsCount);
	if (m_pAutoCompiler->AddAction(ActionItem))
	{
		hr = S_OK;
	}
	return hr;
}


STDMETHODIMP CAutomaticCompiler::Compile()
{
	HRESULT hr = E_FAIL;
	NormalizePath(m_strCompilerRoot);
	NormalizePath(m_strModelRoot);
	NormalizePath(m_strModelDLLName);

	std::wstring strSourceFile = Cex(_T("%s\\%s\\%s.c"), m_strModelRoot.c_str(), m_strModelDLLName.c_str(), m_strModelDLLName.c_str());

	if (m_pAutoCompiler->Generate(strSourceFile.c_str()))
	{


		std::wstring strCommand = m_strCompilerRoot + _T("\\bin\\make.exe");
		std::wstring strHeadersDir = Cex(_T("%s\\headers"), m_strModelRoot.c_str());
		NormalizePath(strHeadersDir);

#ifdef _WIN64
		std::wstring strCommandLine = Cex(_T("%s\\makefile64"), strHeadersDir.c_str());
#else
		std::wstring strCommandLine = Cex(_T("%s\\makefile"), strHeadersDir.c_str());
#endif
		std::wstring strCompilerLogTag = cszLCCCompiler;

		NormalizePath(strCommand);
		NormalizePath(strCommandLine);

		strCommandLine = Cex(_T("\"%s\" -f \"%s\" LCCROOT=\"%s\" ROOTDIR=\"%s\" OUTNAME=\"%s\""),
			strCommand.c_str(), 
			strCommandLine.c_str(),
			m_strCompilerRoot.c_str(),
			m_strModelRoot.c_str(),
			m_strModelDLLName.c_str());

		NormalizePath(strCommandLine);


		_TCHAR szCommandLine[32768];
		_tcscpy_s(szCommandLine, 32768, strCommandLine.c_str());
		DWORD dwCreationFlags = 0;


		HANDLE hStdWrite = NULL;
		HANDLE hStdRead = NULL;


		SECURITY_ATTRIBUTES stdSA;
		ZeroMemory(&stdSA, sizeof(SECURITY_ATTRIBUTES));
		stdSA.nLength = sizeof(SECURITY_ATTRIBUTES);
		stdSA.bInheritHandle = TRUE;
		stdSA.lpSecurityDescriptor = NULL;

		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

		try
		{
			if (!CreatePipe(&hStdRead, &hStdWrite, &stdSA, 0))
				throw CDFW2GetLastErrorException(cszCompileCreatePipeFailed);

			si.cb = sizeof(STARTUPINFO);
			si.hStdError = hStdWrite;
			si.hStdOutput = hStdWrite;
			si.dwFlags |= STARTF_USESTDHANDLES;

			if (!SetHandleInformation(hStdRead, HANDLE_FLAG_INHERIT, 0))
				throw CDFW2GetLastErrorException(cszCompileCreatePipeFailed);

			if (!CreateProcess(NULL,
				szCommandLine,
				NULL,
				NULL,
				TRUE,
				dwCreationFlags,
				NULL,
				strHeadersDir.c_str(),
				&si,
				&pi))
				throw CDFW2GetLastErrorException(cszCompileCreateProcessFailed);

			if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0)
				throw CDFW2GetLastErrorException(cszCompileCreateProcessFailed);

			DWORD dwResult;

			if (!GetExitCodeProcess(pi.hProcess, &dwResult))
				throw CDFW2GetLastErrorException(cszCompileCreateProcessFailed);

			if (dwResult != 0)
			{
				DWORD dwRead;
				char chBuf[500];
				BOOL bSuccess = FALSE;
				std::wstring strOut;

				if (!CloseHandle(hStdWrite))
					throw CDFW2GetLastErrorException(cszCompileCreateProcessFailed);

				hStdWrite = NULL;

				for (;;)
				{
					bSuccess = ReadFile(hStdRead, chBuf, 500, &dwRead, NULL);
					if (!bSuccess || dwRead == 0) break;

					std::string strMsgLine(static_cast<const char*>(chBuf), dwRead);
					strOut += std::wstring(strMsgLine.begin(), strMsgLine.end());

					size_t nFind = std::wstring::npos;
					while ((nFind = strOut.find(_T("\r\n"))) != std::wstring::npos)
					{
						m_pAutoCompiler->m_Logger.Log((strCompilerLogTag + strOut.substr(0, nFind)).c_str());
						strOut.erase(0, nFind + 2);
					}

					if (strOut.length() > 0)
						m_pAutoCompiler->m_Logger.Log((strCompilerLogTag + strOut.substr(0, nFind)).c_str());
				}
			}
			else
				hr = S_OK;
		}
		catch (CDFW2GetLastErrorException& ex)
		{
			m_pAutoCompiler->m_Logger.Log(ex.Message());
			Error(ex.Message(), IID_IAutomaticCompiler, hr);
		}

		if (pi.hProcess) CloseHandle(pi.hProcess);
		if (pi.hThread)	CloseHandle(pi.hThread);
		if (hStdWrite)	CloseHandle(hStdWrite);
		if (hStdRead)	CloseHandle(hStdRead);
	}

	return hr;
}

STDMETHODIMP CAutomaticCompiler::get_Log(VARIANT* LogArray)
{
	HRESULT hr = E_FAIL;
	VariantClear(LogArray);
	SAFEARRAY *pSALog = m_pAutoCompiler->m_Logger.GetSafeArray();
	if (pSALog)
	{
		LogArray->vt = VT_ARRAY | VT_BSTR;
		LogArray->parray = pSALog;
		hr = S_OK;
	}
	return hr;
}

const _TCHAR *CAutomaticCompiler::cszLCCCompiler = _T("Компилятор LCC : ");
const _TCHAR *CAutomaticCompiler::cszCompileCreatePipeFailed = _T("Не удалось создать пайп для вывода результатов компиляции");
const _TCHAR *CAutomaticCompiler::cszCompileCreateProcessFailed = _T("Ошибка при запуске компилятора");
