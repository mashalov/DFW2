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
	HRESULT hRes{ E_FAIL };
	if (pItem_ && Type)
	{
		*Type = static_cast<LONG>(pItem_->DeviceTypeId_); 
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CSlowVariable::get_Name(BSTR* Name)
{
	HRESULT hRes{ E_FAIL };
	if (pItem_)
	{
		*Name = SysAllocString(stringutils::utf8_decode(pItem_->VarName_).c_str());
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CSlowVariable::get_DeviceIds(VARIANT *Ids)
{
	HRESULT hRes{ E_FAIL };

	if (pItem_ && Ids && SUCCEEDED(VariantClear(Ids)))
	{
		auto& vec{ pItem_->DeviceIds_ };

		if (vec.size() == 1)
		{
			Ids->vt = VT_I4;
			Ids->lVal = static_cast<LONG>(vec[0]);
			hRes = S_OK;
		}
		else
		{
			Ids->vt = VT_I4 | VT_ARRAY;
			SAFEARRAYBOUND sabounds = { static_cast<ULONG>(vec.size()), 0 };
			Ids->parray = SafeArrayCreate(VT_I4, 1, &sabounds);
			if (Ids->parray)
			{
				long* p{ nullptr };
				if (SUCCEEDED(SafeArrayAccessData(Ids->parray, (void**)&p)))
				{
					for (ResultIds::const_iterator it = vec.begin(); it != vec.end(); it++, p++)
						*p = static_cast<LONG>(*it);

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
	HRESULT hRes{ E_FAIL };
	if (pItem_ && SUCCEEDED(VariantClear(Plot)))
	{
		Plot->vt = VT_ARRAY | VT_VARIANT;
		const SLOWGRAPHSET& Graph{ pItem_->Graph_ };
		SAFEARRAYBOUND sa[2] = { { static_cast<ULONG>(Graph.size()), 0 }, { 3, 0 } };
		Plot->parray = SafeArrayCreate(VT_VARIANT, 2, sa);
		VARIANT *pVals;
		if (Plot->parray && SUCCEEDED(SafeArrayAccessData(Plot->parray, (void**)&pVals)))
		{
			VARIANT* pTime{ pVals + sa->cElements }, * pDesc{ pTime + sa->cElements };
			for (const auto& it : Graph)
			{
				pVals->vt = VT_R8;	pVals->dblVal = it.Value_;
				pTime->vt = VT_R8;	pTime->dblVal = it.Time_;
				pDesc->vt = VT_BSTR;
				pDesc->bstrVal = SysAllocString(stringutils::utf8_decode(it.Description_).c_str());

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
	pItem_ = pItem;
}