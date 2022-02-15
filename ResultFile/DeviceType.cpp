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
	HRESULT hRes{ E_INVALIDARG };
	if (Id && pDevTypeInfo_)
	{
		*Id = static_cast<LONG>(pDevTypeInfo_->eDeviceType);
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDeviceType::get_Name(BSTR* Name)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Name && pDevTypeInfo_)
	{
		*Name = SysAllocString(stringutils::utf8_decode(pDevTypeInfo_->DevTypeName_).c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDeviceType::get_Devices(VARIANT* Devices)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Devices && pDevTypeInfo_)
	{
		CComObject<CDevices> *pChildrenCollection;
		if (SUCCEEDED(VariantClear(Devices)))
		{
			if (SUCCEEDED(CComObject<CDevices>::CreateInstance(&pChildrenCollection)))
			{
				pChildrenCollection->AddRef();
				Devices->vt = VT_DISPATCH;
				Devices->pdispVal = pChildrenCollection;

				for (int i = 0; i < pDevTypeInfo_->DevicesCount; i++)
				{
					CComObject<CDevice> *pDevice;
					if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
					{
						pDevice->SetDeviceInfo(pDevTypeInfo_->pDeviceInstances_.get() + i);
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