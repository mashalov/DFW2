#include "stdafx.h"
#include "IMinMax.h"
#include "CompareResult.h"


STDMETHODIMP CCompareResult::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] =
	{
		&IID_ICompareResult
	};

	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CCompareResult::get_Left(VARIANT* Left)
{
	HRESULT hRes{ E_FAIL };
	CComObject<CMinMaxData>* pMinMax;
	if (SUCCEEDED(VariantClear(Left)))
	{
		if (SUCCEEDED(CComObject<CMinMaxData>::CreateInstance(&pMinMax)))
		{
			pMinMax->AddRef();
			Left->vt = VT_DISPATCH;
			Left->pdispVal = pMinMax;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CCompareResult::get_Right(VARIANT* Right)
{
	HRESULT hRes{ E_FAIL };
	CComObject<CMinMaxData>* pMinMax;
	if (SUCCEEDED(VariantClear(Right)))
	{
		if (SUCCEEDED(CComObject<CMinMaxData>::CreateInstance(&pMinMax)))
		{
			pMinMax->AddRef();
			Right->vt = VT_DISPATCH;
			Right->pdispVal = pMinMax;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CCompareResult::get_Max(VARIANT* Max)
{
	HRESULT hRes{ E_FAIL };
	CComObject<CMinMaxData>* pMinMax;
	if (SUCCEEDED(VariantClear(Max)))
	{
		if (SUCCEEDED(CComObject<CMinMaxData>::CreateInstance(&pMinMax)))
		{
			pMinMax->AddRef();
			pMinMax->data_ = results_.Max();
			Max->vt = VT_DISPATCH;
			Max->pdispVal = pMinMax;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CCompareResult::get_Min(VARIANT* Min)
{
	HRESULT hRes{ E_FAIL };
	CComObject<CMinMaxData>* pMinMax;
	if (SUCCEEDED(VariantClear(Min)))
	{
		if (SUCCEEDED(CComObject<CMinMaxData>::CreateInstance(&pMinMax)))
		{
			pMinMax->AddRef();
			pMinMax->data_ = results_.Min();
			Min->vt = VT_DISPATCH;
			Min->pdispVal = pMinMax;
			hRes = S_OK;
		}
	}
	return hRes;
}


STDMETHODIMP CCompareResult::get_Average(double* Average)
{
	HRESULT hRes{ E_FAIL };
	if (Average)
	{
		*Average = results_.Avg();
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CCompareResult::get_SquaredDiff(double* StdDev)
{
	HRESULT hRes{ E_FAIL };
	if (StdDev)
	{
		*StdDev = results_.SqSum();
		hRes = S_OK;
	}
	return hRes;
}