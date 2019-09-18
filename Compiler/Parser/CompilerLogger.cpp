#include "..\stdafx.h"
#include "CompilerLogger.h"

SAFEARRAY* CCompilerLogger::GetSafeArray()
{
	SAFEARRAYBOUND sa = { static_cast<ULONG>(m_lst.size()), 0 };
	SAFEARRAY *pSALogArray = SafeArrayCreate(VT_BSTR, 1, &sa);
	if (pSALogArray)
	{
		BSTR *ppData;
		if (SUCCEEDED(SafeArrayAccessData(pSALogArray, (void**)&ppData)))
		{
			for (STRINGLISTITR it = m_lst.begin(); it != m_lst.end(); it++, ppData++)
				*ppData = SysAllocString(it->c_str());

			SafeArrayUnaccessData(pSALogArray);
		}
		else
		{
			SafeArrayDestroy(pSALogArray);
			pSALogArray = nullptr;
		}
	}
	return pSALogArray; 
}