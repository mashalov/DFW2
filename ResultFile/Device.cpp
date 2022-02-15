// Device.cpp : Implementation of CDevice

#include "stdafx.h"
#include "Device.h"
#include "Devices.h"
#include "Variables.h"


// CDevice

STDMETHODIMP CDevice::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IDevice
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CDevice::get_Id(LONG* Id)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Id && pDeviceInfo_)
	{
		*Id = static_cast<LONG>(pDeviceInfo_->GetId(0));
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDevice::get_Name(BSTR* Name)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Name && pDeviceInfo_)
	{
		*Name = SysAllocString(stringutils::utf8_decode(pDeviceInfo_->Name).c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDevice::get_TypeName(BSTR* TypeName)
{
	HRESULT hRes{ E_INVALIDARG };
	if (TypeName && pDeviceInfo_)
	{
		*TypeName = SysAllocString(stringutils::utf8_decode(pDeviceInfo_->pDevType_->DevTypeName_).c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDevice::get_Type(LONG* Type)
{
	HRESULT hRes{ E_INVALIDARG };
	if (Type && pDeviceInfo_)
	{
		*Type = pDeviceInfo_->pDevType_->eDeviceType;
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CDevice::get_Children(VARIANT* Children)
{
	HRESULT hRes = E_INVALIDARG;
	CComObject<CDevices> *pChildrenCollection;
	if (Children && SUCCEEDED(VariantClear(Children)))
	{
		if (SUCCEEDED(CComObject<CDevices>::CreateInstance(&pChildrenCollection)))
		{
			pChildrenCollection->AddRef();
			Children->vt = VT_DISPATCH;
			Children->pdispVal = pChildrenCollection;

			const DEVTYPESET& devset{ pDeviceInfo_->pDevType_->pFileReader_->GetTypesSet() };
			const DeviceTypeInfo* pThisType{ pDeviceInfo_->pDevType_ };

			for (const auto& it : devset)
			{
				DeviceTypeInfo* pDevTypeInfo{ it };
				int nParentsCount{ pDevTypeInfo->DeviceParentIdsCount };
				if (nParentsCount && pDevTypeInfo->eDeviceType != pThisType->eDeviceType)
				{
					for (int i = 0; i < pDevTypeInfo->DevicesCount; i++)
					{
						DeviceInstanceInfo *pDevInfo = pDevTypeInfo->pDeviceInstances_.get() + i;

						for (int p = 0; p < nParentsCount; p++)
						{
							const DeviceLinkToParent *pLink = pDevInfo->GetParent(p);

							if (pLink->eParentType == pThisType->eDeviceType)
							{
								int ids{ 0 };
								for (; ids < pThisType->DeviceIdsCount; ids++)
								{
									if (pLink->Id == pDeviceInfo_->GetId(ids))
										break;
								}
								if (ids < pThisType->DeviceIdsCount)
								{
									CComObject<CDevice> *pDevice;
									if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
									{
										pDevice->SetDeviceInfo(it->pDeviceInstances_.get() + i);
										pDevice->AddRef();
										pChildrenCollection->Add(pDevice);
									}
								}
							}
						}
					}
				}
			}
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CRootDevice::get_Children(VARIANT* Children)
{
	HRESULT hRes = E_INVALIDARG;
	CComObject<CDevices> *pChildrenCollection;
	if (Children && SUCCEEDED(VariantClear(Children)))
	{
		if (SUCCEEDED(CComObject<CDevices>::CreateInstance(&pChildrenCollection)))
		{
			pChildrenCollection->AddRef();
			Children->vt = VT_DISPATCH;
			Children->pdispVal = pChildrenCollection;

			const DEVTYPESET& devset{ pDeviceInfo_->pDevType_->pFileReader_->GetTypesSet() };

			for (const auto& it : devset)
			{
				DeviceTypeInfo* pDevType{ it };

				if (!pDevType->DeviceParentIdsCount)
				{
					for (int i = 0; i < pDevType->DevicesCount; i++)
					{
						CComObject<CDevice> *pDevice;
						if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
						{
							pDevice->SetDeviceInfo(pDevType->pDeviceInstances_.get() + i);
							pDevice->AddRef();
							pChildrenCollection->Add(pDevice);
						}
					}
				}
				else if (pDevType->DeviceParentIdsCount == 1)
				{
					for (int i = 0; i < pDevType->DevicesCount; i++)
					{
						DeviceInstanceInfo* pInst{ pDevType->pDeviceInstances_.get() + i };
						const DeviceLinkToParent* pLink{ pInst->GetParent(0) };
						if(pLink && pLink->eParentType == 0 && pLink->Id == 0)
						{
							CComObject<CDevice> *pDevice;
							if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
							{
								pDevice->SetDeviceInfo(pInst);
								pDevice->AddRef();
								pChildrenCollection->Add(pDevice);
							}
						}
					}
				}
			}

			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CDevice::get_HasVariables(VARIANT_BOOL* HasVariables)
{
	HRESULT hRes{ E_INVALIDARG };
	if (HasVariables && pDeviceInfo_
			 		 && pDeviceInfo_->pDevType_)
	{
		*HasVariables = pDeviceInfo_->pDevType_->VarTypes_.size() > 0;
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CDevice::get_Variables(VARIANT* Variables)
{
	HRESULT hRes{ E_INVALIDARG };
	CComObject<CVariables> *pVariablesCollection;
	if (SUCCEEDED(VariantClear(Variables)) && pDeviceInfo_
										   && pDeviceInfo_->pDevType_)
	{
		if (SUCCEEDED(CComObject<CVariables>::CreateInstance(&pVariablesCollection)))
		{
			pVariablesCollection->AddRef();
			Variables->vt = VT_DISPATCH;
			Variables->pdispVal = pVariablesCollection;

			VARTYPESET& VarTypes{ pDeviceInfo_->pDevType_->VarTypes_ };

			for (const auto& it : VarTypes)
			{
				CComObject<CVariable> *pVariable;
				if (SUCCEEDED(CComObject<CVariable>::CreateInstance(&pVariable)))
				{
					pVariable->SetVariableInfo(&it, pDeviceInfo_);
					pVariable->AddRef();
					pVariablesCollection->Add(pVariable);
				}
			}
		
			hRes = S_OK;
		}
	}
	return hRes;
}
