// Variables.cpp : Implementation of CVariablesCollection

#include "stdafx.h"
#include "Variables.h"


// CVariablesCollection

STDMETHODIMP CVariables::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IVariables
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
