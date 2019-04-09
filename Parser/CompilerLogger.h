#pragma once
#include "stdio.h"
#include "tchar.h"

class CCompilerLogger
{
public:
	void Log(const _TCHAR *cszMessage)
	{
		_tcprintf(_T("\n%s"), cszMessage);
	}
};
