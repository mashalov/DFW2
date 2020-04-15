#pragma once
#include "stdio.h"
#include "tchar.h"
#include "cex.h"

class CCompilerLogger
{
protected:

	STRINGLIST m_lst;

public:
	void Log(const _TCHAR *cszMessage)
	{
		m_lst.push_back(cszMessage);
		_tcprintf(_T("\n%s"), cszMessage);
	}
	SAFEARRAY* GetSafeArray();
};
