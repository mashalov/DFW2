// Variable.cpp : Implementation of CVariable

#include "stdafx.h"
#ifdef _MSC_VER
#include "Device.h"
#include "Variable.h"
#include "ResultRead.h"
#include "CompareResult.h"

// CVariable

STDMETHODIMP CVariable::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IVariable
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CVariable::get_Name(BSTR* Name)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Name && pVariableInfo_)
	{
		*Name = SysAllocString(stringutils::utf8_decode(pVariableInfo_->Name).c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CVariable::get_Index(LONG* Index)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Index && pVariableInfo_)
	{
		*Index = static_cast<LONG>(pVariableInfo_->Index);
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CVariable::get_Units(LONG* Units)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Units && pVariableInfo_)
	{
		*Units = pVariableInfo_->eUnits;
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CVariable::get_UnitsName(BSTR* UnitsName)
{
	HRESULT hRes{ E_INVALIDARG };
	if (UnitsName && pVariableInfo_ && pDeviceInstanceInfo_ && pDeviceInstanceInfo_->pDevType_)
	{
		CResultFileReader* pFileReader{ pDeviceInstanceInfo_->pDevType_->pFileReader_ };
		*UnitsName = SysAllocString(stringutils::utf8_decode(pFileReader->GetUnitsName(pVariableInfo_->eUnits)).c_str()) ;
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CVariable::get_Device(VARIANT* Device)
{
	HRESULT hRes{ E_INVALIDARG };
	if (SUCCEEDED(VariantClear(Device)) && pVariableInfo_ && pDeviceInstanceInfo_ && pDeviceInstanceInfo_->pDevType_)
	{

		CComObject<CDevice> *pDevice;
		if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
		{
			pDevice->SetDeviceInfo(pDeviceInstanceInfo_);
			pDevice->AddRef();
			Device->vt = VT_DISPATCH;
			Device->pdispVal = pDevice;
			hRes = S_OK;
		}
	}
	return hRes;
}


STDMETHODIMP CVariable::get_Multiplier(DOUBLE* Multiplier)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Multiplier && pVariableInfo_)
	{
		*Multiplier = pVariableInfo_->Multiplier;
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CVariable::get_Plot(VARIANT* Plot)
{
	HRESULT hRes{ E_INVALIDARG };
	if (pVariableInfo_ && 
		pDeviceInstanceInfo_ && 
		pDeviceInstanceInfo_->pDevType_ &&
		pDeviceInstanceInfo_->pDevType_->pFileReader_)
	{
		CResultFileReader* pFileReader{ pDeviceInstanceInfo_->pDevType_->pFileReader_ };
		if (Plot && SUCCEEDED(VariantClear(Plot)))
		{
			try
			{
				Plot->parray = pFileReader->CreateSafeArray(pFileReader->ReadChannel(
					pDeviceInstanceInfo_->pDevType_->eDeviceType, 
					pDeviceInstanceInfo_->GetId(0), 
					pVariableInfo_->Index)
				);

				if (Plot->parray)
				{
					Plot->vt = VT_R8 | VT_ARRAY;
					hRes = S_OK;
				}
			}
			catch (CFileReadException& ex)
			{
				Error(ex.whatw(), IID_IVariable, hRes);
			}
		}

		if (FAILED(hRes))
			if(Plot)
				VariantClear(Plot);
	}
	return hRes;
}

STDMETHODIMP CVariable::get_ChannelIndex(LONG* Index)
{
	HRESULT hRes{ S_OK };

	CResultFileReader* pFileReader{ pDeviceInstanceInfo_->pDevType_->pFileReader_ };

	if (Index && pVariableInfo_)
		*Index = static_cast<LONG>(pFileReader->GetChannelIndex(
			pDeviceInstanceInfo_->pDevType_->eDeviceType, 
			pDeviceInstanceInfo_->GetId(0), 
			pVariableInfo_->Index)
			);
	else
		hRes = E_INVALIDARG;

	return hRes;
}

STDMETHODIMP CVariable::Compare(VARIANT TimeSeries, VARIANT* CompareResult)
{
	HRESULT hRes{ E_FAIL };
	if (pVariableInfo_ &&
		pDeviceInstanceInfo_ &&
		pDeviceInstanceInfo_->pDevType_ &&
		pDeviceInstanceInfo_->pDevType_->pFileReader_)
	{
		CResultFileReader* pFileReader{ pDeviceInstanceInfo_->pDevType_->pFileReader_ };
		try
		{
			CComObject<CCompareResult>*pCompareResult;
			if (SUCCEEDED(VariantClear(CompareResult)))
			{
				if (SUCCEEDED(CComObject<CCompareResult>::CreateInstance(&pCompareResult)))
				{
					CComVariant vt{ pFileReader->CreateSafeArray(pFileReader->ReadChannel(
						pDeviceInstanceInfo_->pDevType_->eDeviceType,
						pDeviceInstanceInfo_->GetId(0),
						pVariableInfo_->Index)) };

					CResultRead::DoublePlot::Options opts;
					auto plot1{ CResultRead::ConstructFromPlot(vt) };
					auto plot2{ CResultRead::ConstructFromPlot(TimeSeries) };
					pCompareResult->results_ = plot1.Compare(plot2, opts);
					pCompareResult->AddRef();
					CompareResult->vt = VT_DISPATCH;
					CompareResult->pdispVal = pCompareResult;
					hRes = S_OK;
				}
			}
		}
		catch (CFileReadException& ex)
		{
			Error(ex.whatw(), IID_IVariable, hRes);
		}
		catch (const std::runtime_error& ex)
		{
			Error(ex.what(), IID_IResultRead, hRes);
		}
	}
	return hRes;
}
#endif