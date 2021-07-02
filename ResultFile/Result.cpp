// Result2.cpp : Implementation of CResult2

#include "stdafx.h"
#include "Result.h"
#include "ResultRead.h"
#include "ResultWrite.h"


// CResult2

STDMETHODIMP CResult::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IResult
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CResult::Load(BSTR PathName, VARIANT *ResultRead)
{
	HRESULT hRes = E_INVALIDARG;
	CComObject<CResultRead> *pResultRead;
	if (SUCCEEDED(VariantClear(ResultRead)))
	{
		if (SUCCEEDED(CComObject<CResultRead>::CreateInstance(&pResultRead)))
		{
			pResultRead->AddRef();
			try
			{
				pResultRead->OpenFile(stringutils::utf8_encode(PathName));
				ResultRead->vt = VT_DISPATCH;
				ResultRead->pdispVal = pResultRead;
				hRes = S_OK;
			}
			catch (CFileException& ex)
			{
				pResultRead->Release();
				Error(ex.whatw(), IID_IResult, hRes);
			}
		}
	}
	return hRes;
}

STDMETHODIMP CResult::Create(BSTR PathName, VARIANT *ResultWrite)
{
	HRESULT hRes = E_INVALIDARG;
	CComObject<CResultWrite> *pResultWrite;
	if (SUCCEEDED(VariantClear(ResultWrite)))
	{
		if (SUCCEEDED(CComObject<CResultWrite>::CreateInstance(&pResultWrite)))
		{
			pResultWrite->AddRef();
			try
			{
				pResultWrite->CreateFile(stringutils::utf8_encode(PathName));
				ResultWrite->vt = VT_DISPATCH;
				ResultWrite->pdispVal = pResultWrite;
				hRes = S_OK;
			}
			catch (CFileException& ex)
			{
				pResultWrite->Release();
				Error(ex.whatw(), IID_IResult, hRes);
			}
		}
	}

	return hRes;
}
