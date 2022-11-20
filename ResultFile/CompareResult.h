#pragma once
#include "resource.h"       // main symbols

#ifdef _MSC_VER
#include "ResultFile_i.h"
#endif

#include "IMinMax.h"
#include "CompareResults/TimeSeries.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



#ifdef _MSC_VER
using namespace ATL;

class ATL_NO_VTABLE CCompareResult:
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCompareResult, &CLSID_CompareResults>,
	public ISupportErrorInfo,
	public IDispatchImpl<ICompareResult, &IID_ICompareResult, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
public:
	BEGIN_COM_MAP(CCompareResult)
		COM_INTERFACE_ENTRY(ICompareResult)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
	END_COM_MAP()

	timeseries::TimeSeries<double, double>::CompareResult results_;

	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:

	STDMETHOD(get_Left)(VARIANT* Left);
	STDMETHOD(get_Right)(VARIANT* Right);
	STDMETHOD(get_Max)(VARIANT* Max);
	STDMETHOD(get_Min)(VARIANT* Min);
	STDMETHOD(get_Average)(double* Average);
	STDMETHOD(get_SquaredDiff)(double* StdDev);
};

#endif

