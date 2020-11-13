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
	HRESULT hRes = E_INVALIDARG;
	if (Id && m_pDeviceInfo)
	{
		*Id = static_cast<LONG>(m_pDeviceInfo->GetId(0));
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDevice::get_Name(BSTR* Name)
{
	HRESULT hRes = E_INVALIDARG;
	if (Name && m_pDeviceInfo)
	{
		*Name = SysAllocString(m_pDeviceInfo->Name.c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDevice::get_TypeName(BSTR* TypeName)
{
	HRESULT hRes = E_INVALIDARG;
	if (TypeName && m_pDeviceInfo)
	{
		*TypeName = SysAllocString(m_pDeviceInfo->m_pDevType->strDevTypeName.c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDevice::get_Type(LONG* Type)
{
	HRESULT hRes = E_INVALIDARG;
	if (Type && m_pDeviceInfo)
	{
		*Type = m_pDeviceInfo->m_pDevType->eDeviceType;
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

			const CResultFileReader::DEVTYPESET& devset = m_pDeviceInfo->m_pDevType->m_pFileReader->GetTypesSet();
			CResultFileReader::DEVTYPEITRCONST it = devset.begin();
			CResultFileReader::DeviceTypeInfo *pThisType = m_pDeviceInfo->m_pDevType;

			for (; it != devset.end(); it++)
			{
				CResultFileReader::DeviceTypeInfo *pDevTypeInfo = *it;
				int nParentsCount = pDevTypeInfo->DeviceParentIdsCount;
				if (nParentsCount && pDevTypeInfo->eDeviceType != pThisType->eDeviceType)
				{
					for (int i = 0; i < pDevTypeInfo->DevicesCount; i++)
					{
						CResultFileReader::DeviceInstanceInfo *pDevInfo = pDevTypeInfo->m_pDeviceInstances.get() + i;

						for (int p = 0; p < nParentsCount; p++)
						{
							const CResultFileReader::DeviceLinkToParent *pLink = pDevInfo->GetParent(p);

							if (pLink->m_eParentType == pThisType->eDeviceType)
							{
								int ids = 0;
								for (; ids < pThisType->DeviceIdsCount; ids++)
								{
									if (pLink->m_nId == m_pDeviceInfo->GetId(ids))
										break;
								}
								if (ids < pThisType->DeviceIdsCount)
								{
									CComObject<CDevice> *pDevice;
									if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
									{
										pDevice->SetDeviceInfo((*it)->m_pDeviceInstances.get() + i);
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

			const CResultFileReader::DEVTYPESET& devset = m_pDeviceInfo->m_pDevType->m_pFileReader->GetTypesSet();
			CResultFileReader::DEVTYPEITRCONST it = devset.begin();

			for (; it != devset.end(); it++)
			{
				CResultFileReader::DeviceTypeInfo *pDevType = *it;

				if (!pDevType->DeviceParentIdsCount)
				{
					for (int i = 0; i < pDevType->DevicesCount; i++)
					{
						CComObject<CDevice> *pDevice;
						if (SUCCEEDED(CComObject<CDevice>::CreateInstance(&pDevice)))
						{
							pDevice->SetDeviceInfo(pDevType->m_pDeviceInstances.get() + i);
							pDevice->AddRef();
							pChildrenCollection->Add(pDevice);
						}
					}
				}
				else if (pDevType->DeviceParentIdsCount == 1)
				{
					for (int i = 0; i < pDevType->DevicesCount; i++)
					{
						CResultFileReader::DeviceInstanceInfo *pInst = pDevType->m_pDeviceInstances.get() + i;
						const CResultFileReader::DeviceLinkToParent *pLink = pInst->GetParent(0);
						if(pLink && pLink->m_eParentType == 0 && pLink->m_nId == 0)
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
	HRESULT hRes = E_INVALIDARG;
	if (HasVariables && m_pDeviceInfo
			 		 && m_pDeviceInfo->m_pDevType)
	{
		*HasVariables = m_pDeviceInfo->m_pDevType->m_VarTypes.size() > 0;
		hRes = S_OK;
	}
	return hRes;
}


STDMETHODIMP CDevice::get_Variables(VARIANT* Variables)
{
	HRESULT hRes = E_INVALIDARG;
	CComObject<CVariables> *pVariablesCollection;
	if (SUCCEEDED(VariantClear(Variables)) && m_pDeviceInfo 
										   && m_pDeviceInfo->m_pDevType)
	{
		if (SUCCEEDED(CComObject<CVariables>::CreateInstance(&pVariablesCollection)))
		{
			pVariablesCollection->AddRef();
			Variables->vt = VT_DISPATCH;
			Variables->pdispVal = pVariablesCollection;

			CResultFileReader::VARTYPESET& VarTypes= m_pDeviceInfo->m_pDevType->m_VarTypes;
			CResultFileReader::VARTYPEITRCONST it = VarTypes.begin();

			for (; it != VarTypes.end(); it++)
			{
				CComObject<CVariable> *pVariable;
				if (SUCCEEDED(CComObject<CVariable>::CreateInstance(&pVariable)))
				{
					pVariable->SetVariableInfo(&*it, m_pDeviceInfo);
					pVariable->AddRef();
					pVariablesCollection->Add(pVariable);
				}
			}
		
			hRes = S_OK;
		}
	}
	return hRes;
}
