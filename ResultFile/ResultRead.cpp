// Result.cpp : Implementation of CResult

#include "stdafx.h"
#include "ResultRead.h"
#include "Device.h"
#include "DeviceTypes.h"
#include "SlowVariables.h"
#include "CSVWriter.h"


// CResult

STDMETHODIMP CResultRead::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IResultRead
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP CResultRead::get_Path(BSTR* PathName)
{
	HRESULT hRes = E_INVALIDARG;
	try
	{
		if (PathName)
		{
			*PathName = SysAllocString(stringutils::utf8_decode(m_ResultFileReader.GetFilePath()).c_str());
			hRes = S_OK;
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.Message(), IID_IResultRead, hRes);
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_Comment(BSTR* Comment)
{
	HRESULT hRes = E_INVALIDARG;
	try
	{
		if (Comment)
		{
			*Comment = SysAllocString(stringutils::utf8_decode(m_ResultFileReader.GetComment()).c_str());
			hRes = S_OK;
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.Message(), IID_IResultRead, hRes);
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_Version(LONG* Version)
{
	HRESULT hRes = E_INVALIDARG;
	try
	{
		if (Version)
		{
			*Version = m_ResultFileReader.GetVersion();
			hRes = S_OK;
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.Message(), IID_IResultRead, hRes);
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_TimeScale(VARIANT* TimeScale)
{
	HRESULT hRes = E_INVALIDARG;
	SAFEARRAY *pSA(nullptr);
	try
	{
		if (TimeScale && SUCCEEDED(VariantClear(TimeScale)))
		{
			size_t nPointsCount = m_ResultFileReader.GetPointsCount();
			SAFEARRAYBOUND sabounds = { static_cast<ULONG>(nPointsCount), 0 };

			pSA = SafeArrayCreate(VT_R8, 1, &sabounds);
			if (pSA)
			{
				void *pData;
				if (SUCCEEDED(SafeArrayAccessData(pSA, &pData)))
				{
					m_ResultFileReader.GetTimeScale(static_cast<double*>(pData), nPointsCount);
					if (SUCCEEDED(SafeArrayUnaccessData(pSA)))
					{
						TimeScale->vt = VT_R8 | VT_ARRAY;
						TimeScale->parray = pSA;
						hRes = S_OK;
					}
				}
			}
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.Message(), IID_IResultRead, hRes);
	}
	
	if (FAILED(hRes) && pSA)
		SafeArrayDestroy(pSA);

	return hRes;
}

STDMETHODIMP CResultRead::get_TimeStep(VARIANT* TimeStep)
{
	HRESULT hRes = E_INVALIDARG;
	if (TimeStep && SUCCEEDED(VariantClear(TimeStep)))
	{
		try
		{
			TimeStep->parray = m_ResultFileReader.CreateSafeArray(m_ResultFileReader.GetTimeStep());
			if (TimeStep->parray)
			{
				TimeStep->vt = VT_R8 | VT_ARRAY;
				hRes = S_OK;
			}
		}
		catch (CFileReadException& ex)
		{
			Error(ex.Message(), IID_IResultRead, hRes);
		}
	}

	if (FAILED(hRes))
		if(TimeStep)
			VariantClear(TimeStep);

	return hRes;
}


STDMETHODIMP CResultRead::get_TimeCreated(VARIANT* FileTime)
{
	HRESULT hRes = E_INVALIDARG;
	try
	{
		if (FileTime && SUCCEEDED(VariantClear(FileTime)))
		{
			FileTime->vt = VT_DATE;
			FileTime->date = m_ResultFileReader.GetFileTime();
			hRes = S_OK;
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.Message(), IID_IResultRead, hRes);
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_Root(VARIANT* Root)
{
	HRESULT hRes = E_INVALIDARG;
	CComObject<CRootDevice> *pRootDevice;
	if (SUCCEEDED(VariantClear(Root)))
	{
		if (SUCCEEDED(CComObject<CRootDevice>::CreateInstance(&pRootDevice)))
		{
			pRootDevice->SetDeviceInfo(m_DeviceTypeInfo.m_pDeviceInstances.get());
			pRootDevice->AddRef();
			Root->vt = VT_DISPATCH;
			Root->pdispVal = pRootDevice;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_Types(VARIANT* Types)
{
	HRESULT hRes = E_INVALIDARG;
	CComObject<CDeviceTypes> *pDeviceTypes;
	if (SUCCEEDED(VariantClear(Types)))
	{
		if (SUCCEEDED(CComObject<CDeviceTypes>::CreateInstance(&pDeviceTypes)))
		{
			const CResultFileReader::DEVTYPESET& devset = m_ResultFileReader.GetTypesSet();
			CResultFileReader::DEVTYPEITRCONST it = devset.begin();

			pDeviceTypes->SetDeviceTypesInfo(&devset);

			for (; it != devset.end(); it++)
			{
				CComObject<CDeviceType> *pDeviceType;
				if (SUCCEEDED(CComObject<CDeviceType>::CreateInstance(&pDeviceType)))
				{
					pDeviceType->SetDeviceTypeInfo(*it);
					pDeviceType->AddRef();
					pDeviceTypes->Add(pDeviceType);
				}
			}

			pDeviceTypes->AddRef();
			Types->vt = VT_DISPATCH;
			Types->pdispVal = pDeviceTypes;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CResultRead::ExportCSV(BSTR PathName)
{
	HRESULT hRes = E_FAIL;
	try
	{
		CCSVWriter CSVWriter(m_ResultFileReader);
		if (CSVWriter.WriteCSV(stringutils::utf8_encode(PathName)))
			hRes = S_OK;
	}

	catch (CFileReadException& ex)
	{
		Error(ex.Message(), IID_IResultRead, hRes);
	}

	return hRes;
}


void CResultRead::OpenFile(std::string_view PathName)
{
	m_ResultFileReader.Close();
	m_ResultFileReader.OpenFile(PathName);
}

STDMETHODIMP CResultRead::GetPlot(LONG DeviceType, LONG DeviceId, BSTR VariableName, VARIANT *Plot)
{
	HRESULT hRes = E_INVALIDARG;
	if (Plot && SUCCEEDED(VariantClear(Plot)))
	{
		try
		{
			Plot->parray = m_ResultFileReader.CreateSafeArray(m_ResultFileReader.ReadChannel(DeviceType, DeviceId, stringutils::utf8_encode(VariableName)));
			if (Plot->parray)
			{
				Plot->vt = VT_R8 | VT_ARRAY;
				hRes = S_OK;
			}
		}
		catch (CFileReadException& ex)
		{
			Error(ex.Message(), IID_IResultRead, hRes);
		}
	}

	if (FAILED(hRes))
		if(Plot)
			VariantClear(Plot);

	return hRes;
}

STDMETHODIMP CResultRead::GetPlotByIndex(long nIndex, VARIANT *Plot)
{
	HRESULT hRes = E_INVALIDARG;

	if (Plot && SUCCEEDED(VariantClear(Plot)))
	{
		try
		{
			Plot->parray = m_ResultFileReader.CreateSafeArray(m_ResultFileReader.ReadChannel(nIndex));
			if (Plot->parray)
			{
				Plot->vt = VT_R8 | VT_ARRAY;
				hRes = S_OK;
			}
		}
		catch (CFileReadException& ex)
		{
			Error(ex.Message(), IID_IResultRead, hRes);
		}
	}

	if (FAILED(hRes))
		if(Plot)
			VariantClear(Plot);

	return hRes;
}

STDMETHODIMP CResultRead::get_SlowVariables(VARIANT *SlowVariables)
{
	HRESULT hRes = E_INVALIDARG;
	CComObject<CSlowVariables> *pSlowVariables;
	if (SUCCEEDED(VariantClear(SlowVariables)))
	{
		if (SUCCEEDED(CComObject<CSlowVariables>::CreateInstance(&pSlowVariables)))
		{
			const CSlowVariablesSet& slowset = m_ResultFileReader.GetSlowVariables();
			CSlowVariablesSet::const_iterator it = slowset.begin();

			for (; it != slowset.end(); it++)
			{
				CComObject<CSlowVariable> *pSlowVariable;
				if (SUCCEEDED(CComObject<CSlowVariable>::CreateInstance(&pSlowVariable)))
				{
					pSlowVariable->SetVariableInfo(*it);
					pSlowVariable->AddRef();
					pSlowVariables->Add(pSlowVariable);
				}
			}

			pSlowVariables->AddRef();
			SlowVariables->vt = VT_DISPATCH;
			SlowVariables->pdispVal = pSlowVariables;
			hRes = S_OK;
		}
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_Points(LONG *PointsCount)
{
	HRESULT hRes = S_OK;

	if (PointsCount)
		*PointsCount = static_cast<LONG>(m_ResultFileReader.GetPointsCount());
	else
		hRes = E_INVALIDARG;

	return hRes;
}

STDMETHODIMP CResultRead::get_Channels(LONG* ChannelsCount)
{
	HRESULT hRes = S_OK;

	if (ChannelsCount)
		*ChannelsCount = static_cast<LONG>(m_ResultFileReader.GetChannelsCount());
	else
		hRes = E_INVALIDARG;

	return hRes;
}

STDMETHODIMP CResultRead::Close()
{
	m_ResultFileReader.Close();
	return S_OK;
}

STDMETHODIMP CResultRead::get_UserComment(BSTR* UserComment)
{
	HRESULT hRes = E_INVALIDARG;
	if (UserComment)
	{
		*UserComment = SysAllocString(stringutils::utf8_decode(m_ResultFileReader.GetUserComment()).c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_CompressionRatio(DOUBLE *pRatio)
{
	HRESULT hRes = E_INVALIDARG;
	if (pRatio)
	{
		*pRatio = m_ResultFileReader.GetCompressionRatio();
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CResultRead::put_UserComment(BSTR UserComment)
{
	HRESULT hRes = E_FAIL;
	try
	{
		m_ResultFileReader.SetUserComment(stringutils::utf8_encode(UserComment));
		hRes = S_OK;
	}
	catch (CFileReadException& ex)
	{
		Error(ex.Message(), IID_IResultRead, hRes);
	}
	return hRes;
}

