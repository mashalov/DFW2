// Variables.cpp : Implementation of CVariablesCollection

#include "stdafx.h"
#include "SlowVariables.h"


// CVariablesCollection

STDMETHODIMP CSlowVariables::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] =
	{
		&IID_ISlowVariables
	};

	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CSlowVariables::Find(LONG DeviceTypeId, VARIANT DeviceIds, BSTR VariableName, VARIANT *SlowVariable)
{
	HRESULT hRes = E_FAIL;
	if (m_pFileReader && SUCCEEDED(VariantClear(SlowVariable)))
	{
		const CSlowVariablesSet& SlowVariables = m_pFileReader->GetSlowVariables();
		LONGVECTOR vecDeviceIds;
		if (SlowVariables.VariantToIds(&DeviceIds, vecDeviceIds))
		{
			CSlowVariableItem FindItem(DeviceTypeId, vecDeviceIds, stringutils::utf8_encode(VariableName));
			CSlowVariablesSet::const_iterator it = SlowVariables.find(&FindItem);
			if (it != SlowVariables.end())
			{
				CComObject<CSlowVariable> *pSlowVariable;
				if (SUCCEEDED(CComObject<CSlowVariable>::CreateInstance(&pSlowVariable)))
				{
					pSlowVariable->SetVariableInfo(*it);
					pSlowVariable->AddRef();
					SlowVariable->vt = VT_DISPATCH;
					SlowVariable->pdispVal = pSlowVariable;
				}
			}

			hRes = S_OK;
		}
	}
	return hRes;
}