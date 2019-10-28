// DeviceTypeWrite.cpp : Implementation of CDeviceTypeWrite

#include "stdafx.h"
#include "DeviceTypeWrite.h"


// CDeviceTypeWrite

STDMETHODIMP CDeviceTypeWrite::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IDeviceTypeWrite
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CDeviceTypeWrite::SetDeviceTypeMetrics(LONG DeviceIdsCount, LONG ParentIdsCount, LONG DevicesCount)
{
	HRESULT hRes = E_FAIL;
	if (m_pDeviceTypeInfo)
	{
		m_pDeviceTypeInfo->DeviceIdsCount = DeviceIdsCount;
		m_pDeviceTypeInfo->DevicesCount = DevicesCount;
		m_pDeviceTypeInfo->DeviceParentIdsCount = ParentIdsCount;
		m_pDeviceTypeInfo->AllocateData();
		m_pDeviceInstanceInfo = m_pDeviceTypeInfo->m_pDeviceInstances.get();
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDeviceTypeWrite::AddDeviceTypeVariable(BSTR VariableName, LONG UnitId, DOUBLE Multiplier)
{
	HRESULT hRes = E_FAIL;
	if (m_pDeviceTypeInfo)
	{
		CResultFileReader::VariableTypeInfo VarTypeInfo;
		VarTypeInfo.eUnits = UnitId;
		VarTypeInfo.Multiplier = Multiplier;
		VarTypeInfo.Name = VariableName;
		VarTypeInfo.nIndex = m_pDeviceTypeInfo->m_VarTypes.size();
		if (m_pDeviceTypeInfo->m_VarTypes.insert(VarTypeInfo).second)
		{
			m_pDeviceTypeInfo->m_VarTypesList.push_back(VarTypeInfo);
			hRes = S_OK;
		}
		else
			Error(Cex(CDFW2Messages::m_cszDuplicatedVariableName, VarTypeInfo.Name.c_str(), m_pDeviceTypeInfo->strDevTypeName.c_str()), IID_IDeviceTypeWrite, hRes);
	}
	return hRes;
}

long CDeviceTypeWrite::GetParameterByIndex(VARIANT& vt, long nIndex)
{
	long nResult = -1;
	
	if (m_pDeviceTypeInfo)
	{
		if ((vt.vt & VT_ARRAY) && (vt.vt & VT_I4) && SafeArrayGetDim(vt.parray) == 1)
		{
			int nElement;
			if (SUCCEEDED(SafeArrayGetElement(vt.parray, &nIndex, &nElement)))
			{
				nResult = nElement;
			}
			else
				throw CFileWriteException(NULL, CDFW2Messages::m_cszWrongParameter);
		}
		else if (!nIndex)
		{
			if (SUCCEEDED(VariantChangeType(&vt, &vt, 0, VT_I4)))
				nResult = vt.lVal;
			else
				throw CFileWriteException(NULL, CDFW2Messages::m_cszWrongParameter);
		}
		else
			throw CFileWriteException(NULL, CDFW2Messages::m_cszWrongParameter);
	}
	else
		throw CFileWriteException(NULL, CDFW2Messages::m_cszWrongParameter);

	return nResult;
}

STDMETHODIMP CDeviceTypeWrite::AddDevice(BSTR DeviceName, VARIANT DeviceIds, VARIANT ParentIds, VARIANT ParentTypes)
{
	HRESULT hRes = E_FAIL;

	try
	{
		if (m_pDeviceTypeInfo)
		{
			ptrdiff_t nIndex = m_pDeviceInstanceInfo - m_pDeviceTypeInfo->m_pDeviceInstances.get();
			if (nIndex < m_pDeviceTypeInfo->DevicesCount)
			{
				m_pDeviceInstanceInfo->nIndex = nIndex;
				m_pDeviceInstanceInfo->Name = DeviceName;

				for (int i = 0; i < m_pDeviceTypeInfo->DeviceIdsCount; i++)
					m_pDeviceInstanceInfo->SetId(i, GetParameterByIndex(DeviceIds, i));

				for (int i = 0; i < m_pDeviceTypeInfo->DeviceParentIdsCount; i++)
					m_pDeviceInstanceInfo->SetParent(i, GetParameterByIndex(ParentTypes, i), GetParameterByIndex(ParentIds, i));

				m_pDeviceInstanceInfo++;
				hRes = S_OK;
			}
			else
				Error(Cex(CDFW2Messages::m_cszDuplicatedVariableUnit, m_pDeviceInstanceInfo - m_pDeviceTypeInfo->m_pDeviceInstances.get(), m_pDeviceTypeInfo->DevicesCount), IID_IDeviceTypeWrite, hRes);
		}
	}
	catch (CFileException &ex)
	{
		Error(ex.Message(), IID_IDeviceTypeWrite, hRes);
	}
	return hRes;
}

void CDeviceTypeWrite::SetDeviceTypeInfo(CResultFileReader::DeviceTypeInfo *pDeviceTypeInfo)
{
	m_pDeviceTypeInfo = pDeviceTypeInfo;
}