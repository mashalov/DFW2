// SlowVariable.cpp : Implementation of CSlowVariable

#include "stdafx.h"
#include "SlowVariable.h"


// CSlowVariable

STDMETHODIMP CSlowVariable::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] =
	{
		&IID_ISlowVariable
	};

	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CSlowVariable::get_DeviceType(LONG* Type)
{
	HRESULT hRes = E_FAIL;
	if (m_pItem && Type)
	{
		*Type = m_pItem->m_DeviceTypeId; 
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CSlowVariable::get_Name(BSTR* Name)
{
	HRESULT hRes = E_FAIL;
	if (m_pItem)
	{
		*Name = SysAllocString(m_pItem->m_strVarName.c_str());
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CSlowVariable::get_DeviceIds(VARIANT *Ids)
{
	HRESULT hRes = E_FAIL;

	if (m_pItem && Ids && SUCCEEDED(VariantClear(Ids)))
	{
		LONGVECTOR& vec = m_pItem->m_DeviceIds;

		if (vec.size() == 1)
		{
			Ids->vt = VT_I4;
			Ids->lVal = vec[0];
			hRes = S_OK;
		}
		else
		{
			Ids->vt = VT_I4 | VT_ARRAY;
			SAFEARRAYBOUND sabounds = { static_cast<ULONG>(vec.size()), 0 };
			Ids->parray = SafeArrayCreate(VT_I4, 1, &sabounds);
			if (Ids->parray)
			{
				long *p;
				if (SUCCEEDED(SafeArrayAccessData(Ids->parray, (void**)&p)))
				{
					for (LONGVECTOR::const_iterator it = vec.begin(); it != vec.end(); it++, p++)
						*p = *it;

					if (SUCCEEDED(SafeArrayUnaccessData(Ids->parray)))
						hRes = S_OK;
				}
			}
		}
	}

	return hRes;
}

STDMETHODIMP CSlowVariable::get_Plot(VARIANT *Plot)
{
	HRESULT hRes = E_FAIL;
	if (m_pItem && SUCCEEDED(VariantClear(Plot)))
	{
		Plot->vt = VT_ARRAY | VT_VARIANT;
		SLOWGRAPHSET& Graph = m_pItem->m_Graph;
		SAFEARRAYBOUND sa[2] = { { static_cast<ULONG>(Graph.size()), 0 }, { 3, 0 } };
		Plot->parray = SafeArrayCreate(VT_VARIANT, 2, sa);
		VARIANT *pVals;
		if (Plot->parray && SUCCEEDED(SafeArrayAccessData(Plot->parray, (void**)&pVals)))
		{
			VARIANT *pTime = pVals + sa->cElements;
			VARIANT *pDesc = pTime + sa->cElements;
			for (SLOWGRAPHSETITRCONST it = Graph.begin(); it != Graph.end(); it++)
			{
				pVals->vt = VT_R8;	pVals->dblVal = it->m_dValue;
				pTime->vt = VT_R8;	pTime->dblVal = it->m_dTime;
				pDesc->vt = VT_BSTR;
				pDesc->bstrVal = SysAllocString(it->m_strDescription.c_str());

				pVals++;
				pTime++;
				pDesc++;
			}
			if (SUCCEEDED(SafeArrayUnaccessData(Plot->parray)))
				hRes = S_OK;
		}
	}
	return hRes;
}

void CSlowVariable::SetVariableInfo(CSlowVariableItem *pItem)
{
	_ASSERTE(pItem);
	m_pItem = pItem;
}