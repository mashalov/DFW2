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
		m_pDeviceTypeInfo->SetDeviceTypeMetrics(DeviceIdsCount, ParentIdsCount, DevicesCount);
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CDeviceTypeWrite::AddDeviceTypeVariable(BSTR VariableName, LONG UnitId, DOUBLE Multiplier)
{
	HRESULT hRes = E_FAIL;
	if (m_pDeviceTypeInfo)
	{
		try
		{
			m_pDeviceTypeInfo->AddDeviceTypeVariable(stringutils::utf8_encode(VariableName), UnitId, Multiplier);
			hRes = S_OK;
		}
		catch (const dfw2error& er)
		{
			Error(er.whatw(), IID_IDeviceTypeWrite, hRes);
		}
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
				throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);
		}
		else if (!nIndex)
		{
			if (SUCCEEDED(VariantChangeType(&vt, &vt, 0, VT_I4)))
				nResult = vt.lVal;
			else
				throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);
		}
		else
			throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);
	}
	else
		throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);

	return nResult;
}

STDMETHODIMP CDeviceTypeWrite::AddDevice(BSTR DeviceName, VARIANT DeviceIds, VARIANT ParentIds, VARIANT ParentTypes)
{
	HRESULT hRes = E_FAIL;

	if (m_pDeviceTypeInfo)
	{
		try
		{
			auto GetVariantVec = [](VARIANT& vt) -> std::vector<ptrdiff_t>
			{
				std::vector<ptrdiff_t> retVec;
				if ((vt.vt & VT_ARRAY) && (vt.vt & VT_I4) && SafeArrayGetDim(vt.parray) == 1)
				{
					long* pData(nullptr);
					long lBound;
					long uBound;

					if(FAILED(SafeArrayAccessData(vt.parray,(void**)&pData)))
						throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);

					if(FAILED(SafeArrayGetLBound(vt.parray,1,&lBound)))
						throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);

					if (FAILED(SafeArrayGetUBound(vt.parray, 1, &uBound)))
						throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);

					for (long ix = lBound; ix <= uBound; ix++)
						retVec.push_back(pData[ix]);

					if (FAILED(SafeArrayUnaccessData(vt.parray)))
						throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);
				}
				else
				{
					if (SUCCEEDED(VariantChangeType(&vt, &vt, 0, VT_I4)))
						retVec.push_back(vt.lVal);
					else
						throw CFileWriteException(CDFW2Messages::m_cszWrongParameter);
				}

				return retVec;
			};

			m_pDeviceTypeInfo->AddDevice(stringutils::utf8_encode(DeviceName), 
				GetVariantVec(DeviceIds),
				GetVariantVec(ParentIds),
				GetVariantVec(ParentTypes));

			hRes = S_OK;
		}
		catch (const dfw2error& er)
		{
			Error(er.whatw(), IID_IDeviceTypeWrite, hRes);
		}
	}

	return hRes;
}

void CDeviceTypeWrite::SetDeviceTypeInfo(DeviceTypeInfo* pDeviceTypeInfo)
{
	m_pDeviceTypeInfo = pDeviceTypeInfo;
}

