// DeviceType.cpp : Implementation of CDeviceType

#include "stdafx.h"
#include "DeviceType.h"
#include "Devices.h"


// CDeviceType

STDMETHODIMP CDeviceType::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IDeviceType
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CDeviceType::get_Id(LONG* Id)
{
	HRESULT hRes = E_INVALIDARG;
	if (Id && m_pDevTypeInfo)
	{
		*Id = static_cast<LONG>(m_pDevTypeInfo->eDeviceType);
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDeviceType::get_Name(BSTR* Name)
{
	HRESULT hRes = E_INVALIDARG;
	if (Name && m_pDevTypeInfo)
	{
		*Name = SysAllocString(m_pDevTypeInfo->strDevTypeName.c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDeviceType::get_Devices(VARIANT* Devices)
{
	HRESULT hRes = E_INVALIDARG;
	if (Devices && m_pDevTypeInfo)
	{
		CComObject<CDevices> *pChildrenCollection;
		if (SUCCEEDED(VariantClear(Devices)))
		{
			if (SUCCEEDED(CComObject<CDevices>::CreateInstance(&pChildrenCollection)))
			{
				pChildrenCollection->AddRef();
				Devices->vt = VT_DISPATCH;
				Devices->pdispVal = pChildrenCollection;

				for (int i = 0; i < m_pDevTypeInfo->DevicesCount; i++)
				{
					CComObject<CDevice> *pDevice;
					if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
					{
						pDevice->SetDeviceInfo(m_pDevTypeInfo->m_pDeviceInstances + i);
						pDevice->AddRef();
						pChildrenCollection->Add(pDevice);
					}
				}
			}

			hRes = S_OK;
		}
	}
	return hRes;
}

