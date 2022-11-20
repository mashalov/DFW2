#include "stdafx.h"
#include "IMinMax.h"


STDMETHODIMP CMinMaxData::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] =
	{
		&IID_IMinMaxData
	};

	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CMinMaxData::get_Time(double* Time)
{
	HRESULT hRes{ E_FAIL };
	if (Time)
	{
		*Time = data_.t();
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CMinMaxData::get_Metric(double* Metric)
{
	HRESULT hRes{ E_FAIL };
	if (Metric)
	{
		*Metric = data_.v();
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CMinMaxData::get_Value1(double* Value)
{
	HRESULT hRes{ E_FAIL };
	if (Value)
	{
		*Value = data_.v1();
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CMinMaxData::get_Value2(double* Value)
{
	HRESULT hRes{ E_FAIL };
	if (Value)
	{
		*Value = data_.v2();
		hRes = S_OK;
	}
	return hRes;
}

