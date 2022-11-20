#include "stdafx.h"
#include "Timestamped.h"


STDMETHODIMP CTimestamped::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] =
	{
		&IID_ITimestamped
	};

	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CTimestamped::get_Time(double* Time)
{
	HRESULT hRes{ E_FAIL };
	if (Time)
	{
		*Time = difference_.t;
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CTimestamped::get_Value(double* Value)
{
	HRESULT hRes{ E_FAIL };
	if (Value)
	{
		*Value = difference_.v1;
		hRes = S_OK;
	}
	return hRes;
}
