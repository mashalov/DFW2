// VariablesCollection.cpp : Implementation of CVariablesCollection

#include "stdafx.h"
#include "VariablesCollection.h"


// CVariablesCollection

STDMETHODIMP CVariablesCollection::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IVariablesCollection
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
