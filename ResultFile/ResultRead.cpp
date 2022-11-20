// Result.cpp : Implementation of CResult

#include "stdafx.h"
#include "ResultRead.h"
#include "Device.h"
#include "DeviceTypes.h"
#include "SlowVariables.h"
#include "CSVWriter.h"
#include "CompareResult.h"

// CResult

std::string GetHRESULT(HRESULT hr)
{
	return fmt::format("HRESULT = {:#0x}", static_cast<unsigned>(hr));
}

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
			*PathName = SysAllocString(stringutils::utf8_decode(ResultFileReader_.GetFilePath()).c_str());
			hRes = S_OK;
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.whatw(), IID_IResultRead, hRes);
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
			*Comment = SysAllocString(stringutils::utf8_decode(ResultFileReader_.GetComment()).c_str());
			hRes = S_OK;
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.whatw(), IID_IResultRead, hRes);
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
			*Version = ResultFileReader_.GetVersion();
			hRes = S_OK;
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.whatw(), IID_IResultRead, hRes);
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
			size_t nPointsCount{ ResultFileReader_.GetPointsCount() };
			SAFEARRAYBOUND sabounds = { static_cast<ULONG>(nPointsCount), 0 } ;

			pSA = SafeArrayCreate(VT_R8, 1, &sabounds);
			if (pSA)
			{
				void *pData {nullptr};
				if (SUCCEEDED(SafeArrayAccessData(pSA, &pData)))
				{
					ResultFileReader_.GetTimeScale(static_cast<double*>(pData), nPointsCount);
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
		Error(ex.whatw(), IID_IResultRead, hRes);
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
			TimeStep->parray = ResultFileReader_.CreateSafeArray(ResultFileReader_.GetTimeStep());
			if (TimeStep->parray)
			{
				TimeStep->vt = VT_R8 | VT_ARRAY;
				hRes = S_OK;
			}
		}
		catch (CFileReadException& ex)
		{
			Error(ex.whatw(), IID_IResultRead, hRes);
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
			FileTime->date = ResultFileReader_.GetFileTime();
			hRes = S_OK;
		}
	}
	catch (CFileReadException& ex)
	{
		Error(ex.whatw(), IID_IResultRead, hRes);
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_Root(VARIANT* Root)
{
	HRESULT hRes{ E_INVALIDARG };
	CComObject<CRootDevice> *pRootDevice;
	if (SUCCEEDED(VariantClear(Root)))
	{
		if (SUCCEEDED(CComObject<CRootDevice>::CreateInstance(&pRootDevice)))
		{
			pRootDevice->SetDeviceInfo(DeviceTypeInfo_.pDeviceInstances_.get());
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
	HRESULT hRes{ E_INVALIDARG };
	CComObject<CDeviceTypes> *pDeviceTypes;
	if (SUCCEEDED(VariantClear(Types)))
	{
		if (SUCCEEDED(CComObject<CDeviceTypes>::CreateInstance(&pDeviceTypes)))
		{
			const DEVTYPESET& devset = ResultFileReader_.GetTypesSet();

			pDeviceTypes->SetDeviceTypesInfo(&devset);

			for (const auto& it : devset)
			{
				CComObject<CDeviceType> *pDeviceType;
				if (SUCCEEDED(CComObject<CDeviceType>::CreateInstance(&pDeviceType)))
				{
					pDeviceType->SetDeviceTypeInfo(it);
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
	HRESULT hRes{ E_FAIL };
	try
	{
		CCSVWriter CSVWriter(ResultFileReader_);
		if (CSVWriter.WriteCSV(stringutils::utf8_encode(PathName)))
			hRes = S_OK;
	}

	catch (CFileReadException& ex)
	{
		Error(ex.whatw(), IID_IResultRead, hRes);
	}

	return hRes;
}


void CResultRead::OpenFile(std::string_view PathName)
{
	ResultFileReader_.Close();
	ResultFileReader_.OpenFile(PathName);
}

STDMETHODIMP CResultRead::GetPlot(LONG DeviceType, LONG DeviceId, BSTR VariableName, VARIANT *Plot)
{
	HRESULT hRes = E_INVALIDARG;
	if (Plot && SUCCEEDED(VariantClear(Plot)))
	{
		try
		{
			Plot->parray = ResultFileReader_.CreateSafeArray(ResultFileReader_.ReadChannel(DeviceType, DeviceId, stringutils::utf8_encode(VariableName)));
			if (Plot->parray)
			{
				Plot->vt = VT_R8 | VT_ARRAY;
				hRes = S_OK;
			}
		}
		catch (CFileReadException& ex)
		{
			Error(ex.whatw(), IID_IResultRead, hRes);
		}
	}

	if (FAILED(hRes))
		if(Plot)
			VariantClear(Plot);

	return hRes;
}

STDMETHODIMP CResultRead::GetPlotByIndex(long Index, VARIANT *Plot)
{
	HRESULT hRes = E_INVALIDARG;

	if (Plot && SUCCEEDED(VariantClear(Plot)))
	{
		try
		{
			Plot->parray = ResultFileReader_.CreateSafeArray(ResultFileReader_.ReadChannel(Index));
			if (Plot->parray)
			{
				Plot->vt = VT_R8 | VT_ARRAY;
				hRes = S_OK;
			}
		}
		catch (CFileReadException& ex)
		{
			Error(ex.whatw(), IID_IResultRead, hRes);
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
			const CSlowVariablesSet& slowset{ ResultFileReader_.GetSlowVariables() };
			CSlowVariablesSet::const_iterator it{ slowset.begin() };

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
	HRESULT hRes{ S_OK };

	if (PointsCount)
		*PointsCount = static_cast<LONG>(ResultFileReader_.GetPointsCount());
	else
		hRes = E_INVALIDARG;

	return hRes;
}

STDMETHODIMP CResultRead::get_Channels(LONG* ChannelsCount)
{
	HRESULT hRes{ S_OK };

	if (ChannelsCount)
		*ChannelsCount = static_cast<LONG>(ResultFileReader_.GetChannelsCount());
	else
		hRes = E_INVALIDARG;

	return hRes;
}

STDMETHODIMP CResultRead::Close()
{
	ResultFileReader_.Close();
	return S_OK;
}

STDMETHODIMP CResultRead::get_UserComment(BSTR* UserComment)
{
	HRESULT hRes{ E_INVALIDARG };
	if (UserComment)
	{
		*UserComment = SysAllocString(stringutils::utf8_decode(ResultFileReader_.GetUserComment()).c_str());
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CResultRead::get_CompressionRatio(DOUBLE *pRatio)
{
	HRESULT hRes{ E_INVALIDARG };
	if (pRatio)
	{
		*pRatio = ResultFileReader_.GetCompressionRatio();
		hRes = S_OK;
	}
	return hRes;
}

STDMETHODIMP CResultRead::put_UserComment(BSTR UserComment)
{
	HRESULT hRes{ E_FAIL };
	try
	{
		ResultFileReader_.SetUserComment(stringutils::utf8_encode(UserComment));
		hRes = S_OK;
	}
	catch (const CFileWriteException& ex)
	{
		Error(ex.whatw(), IID_IResultRead, hRes);
	}
	return hRes;
}


STDMETHODIMP CResultRead::Compare(VARIANT TimeSeries1, VARIANT TimeSeries2, VARIANT* CompareResult)
{
	HRESULT hRes{ E_INVALIDARG };
	CComObject<CCompareResult>* pCompareResult;
	
	try
	{
		auto plot1{ ConstructFromPlot(TimeSeries1) };
		auto plot2{ ConstructFromPlot(TimeSeries2) };
		if (SUCCEEDED(VariantClear(CompareResult)))
		{
			if (SUCCEEDED(CComObject<CCompareResult>::CreateInstance(&pCompareResult)))
			{
				DoublePlot::Options opts;
				pCompareResult->results_ = plot1.Compare(plot2, opts);
				pCompareResult->AddRef();
				CompareResult->vt = VT_DISPATCH;
				CompareResult->pdispVal = pCompareResult;
				hRes = S_OK;
			}
		}
	}
	catch (const std::runtime_error& ex)
	{
		Error(ex.what(), IID_IResultRead, hRes);
	}
	return hRes;
}


CResultRead::DoublePlot CResultRead::ConstructFromPlot(const VARIANT& Input) const
{
	if (Input.vt != (VT_ARRAY | VT_R8))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - array has wrong type {}", Input.vt));
	if (auto nDims{ SafeArrayGetDim(Input.parray) }; nDims != 2)
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - array must have 2 dimensions {}", nDims));
	LONG lBound{ 0 }, uBound{ 0 };
	HRESULT hr{ SafeArrayGetLBound(Input.parray, 1, &lBound) };
	if (FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - cannot get low bound {}", GetHRESULT(hr)));
	hr = SafeArrayGetUBound(Input.parray, 1, &uBound);
	if (FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - cannot get high bound {}", GetHRESULT(hr)));

	double* pData{ nullptr };

	uBound -= lBound - 1;

	hr = SafeArrayAccessData(Input.parray, (void**)&pData);

	if (FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - SafeArrayAccessData failed {}", GetHRESULT(hr)));

	DoublePlot plot(uBound, pData + uBound, pData);

	hr = SafeArrayUnaccessData(Input.parray);

	if (FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - SafeArrayUnaccessData failed {}", GetHRESULT(hr)));

	return plot;
}