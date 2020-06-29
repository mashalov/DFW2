#pragma once
#include "stdio.h"
#include "tchar.h"
#include "..\..\DFW2\cex.h"

class CCompilerLogger
{
protected:

	STRINGLIST m_lst;

public:
	void Log(std::wstring_view Message)
	{
		m_lst.push_back(std::wstring(Message));
		_tcprintf(_T("\n%s"), std::wstring(Message).c_str());
	}
	SAFEARRAY* GetSafeArray();
};
