// Variable.cpp : Implementation of CVariable

#include "stdafx.h"
#include "Device.h"
#include "Variable.h"



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
	HRESULT hRes = E_INVALIDARG;
	if (Name && m_pVariableInfo)
	{
		*Name = SysAllocString(m_pVariableInfo->Name.c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CVariable::get_Index(LONG* Index)
{
	HRESULT hRes = E_INVALIDARG;
	if (Index && m_pVariableInfo)
	{
		*Index = static_cast<LONG>(m_pVariableInfo->nIndex);
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CVariable::get_Units(LONG* Units)
{
	HRESULT hRes = E_INVALIDARG;
	if (Units && m_pVariableInfo)
	{
		*Units = m_pVariableInfo->eUnits;
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CVariable::get_UnitsName(BSTR* UnitsName)
{
	HRESULT hRes = E_INVALIDARG;
	if (UnitsName && m_pVariableInfo && m_pDeviceInstanceInfo && m_pDeviceInstanceInfo->m_pDevType)
	{
		CResultFileReader *pFileReader = m_pDeviceInstanceInfo->m_pDevType->m_pFileReader;
		*UnitsName = SysAllocString(pFileReader->GetUnitsName(m_pVariableInfo->eUnits));
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CVariable::get_Device(VARIANT* Device)
{
	HRESULT hRes = E_INVALIDARG;
	if (SUCCEEDED(VariantClear(Device)) && m_pVariableInfo && m_pDeviceInstanceInfo && m_pDeviceInstanceInfo->m_pDevType)
	{

		CComObject<CDevice> *pDevice;
		if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
		{
			pDevice->SetDeviceInfo(m_pDeviceInstanceInfo);
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
	HRESULT hRes = E_INVALIDARG;
	if (Multiplier && m_pVariableInfo)
	{
		*Multiplier = m_pVariableInfo->Multiplier;
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CVariable::get_Plot(VARIANT* Plot)
{
	HRESULT hRes = E_INVALIDARG;
	if (m_pVariableInfo && 
		m_pDeviceInstanceInfo && 
		m_pDeviceInstanceInfo->m_pDevType &&
		m_pDeviceInstanceInfo->m_pDevType->m_pFileReader)
	{
		CResultFileReader *pFileReader = m_pDeviceInstanceInfo->m_pDevType->m_pFileReader;
		if (Plot && SUCCEEDED(VariantClear(Plot)))
		{
			try
			{
				Plot->parray = pFileReader->CreateSafeArray(pFileReader->ReadChannel(m_pDeviceInstanceInfo->m_pDevType->eDeviceType, m_pDeviceInstanceInfo->GetId(0), m_pVariableInfo->nIndex));
				if (Plot->parray)
				{
					Plot->vt = VT_R8 | VT_ARRAY;
					hRes = S_OK;
				}
			}
			catch (CFileReadException& ex)
			{
				Error(ex.Message(), IID_IVariable, hRes);
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
	HRESULT hRes = S_OK;

	CResultFileReader *pFileReader = m_pDeviceInstanceInfo->m_pDevType->m_pFileReader;

	if (Index && m_pVariableInfo)
		*Index = static_cast<LONG>(pFileReader->GetChannelIndex(m_pDeviceInstanceInfo->m_pDevType->eDeviceType, m_pDeviceInstanceInfo->GetId(0), m_pVariableInfo->nIndex));
	else
		hRes = E_INVALIDARG;

	return hRes;
}