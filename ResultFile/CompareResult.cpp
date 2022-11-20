#include "stdafx.h"
#include "Timestamped.h"
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
	CComObject<CTimestamped>* pTimestamped;
	if (SUCCEEDED(VariantClear(Left)))
	{
		if (SUCCEEDED(CComObject<CTimestamped>::CreateInstance(&pTimestamped)))
		{
			pTimestamped->AddRef();
			Left->vt = VT_DISPATCH;
			Left->pdispVal = pTimestamped;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CCompareResult::get_Right(VARIANT* Right)
{
	HRESULT hRes{ E_FAIL };
	CComObject<CTimestamped>* pTimestamped;
	if (SUCCEEDED(VariantClear(Right)))
	{
		if (SUCCEEDED(CComObject<CTimestamped>::CreateInstance(&pTimestamped)))
		{
			pTimestamped->AddRef();
			Right->vt = VT_DISPATCH;
			Right->pdispVal = pTimestamped;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CCompareResult::get_Max(VARIANT* Max)
{
	HRESULT hRes{ E_FAIL };
	CComObject<CTimestamped>* pTimestamped;
	if (SUCCEEDED(VariantClear(Max)))
	{
		if (SUCCEEDED(CComObject<CTimestamped>::CreateInstance(&pTimestamped)))
		{
			pTimestamped->AddRef();
			Max->vt = VT_DISPATCH;
			Max->pdispVal = pTimestamped;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CCompareResult::get_Average(double* Average)
{
	HRESULT hRes{ E_FAIL };
	return hRes;
}

STDMETHODIMP CCompareResult::get_StdDev(double* StdDev)
{
	HRESULT hRes{ E_FAIL };
	return hRes;
}