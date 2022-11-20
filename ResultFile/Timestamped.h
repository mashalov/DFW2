#pragma once
#include "resource.h"       // main symbols

#ifdef _MSC_VER
#include "ResultFile_i.h"
#endif

#include "CompareResults/Plot.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



#ifdef _MSC_VER
using namespace ATL;

class ATL_NO_VTABLE CTimestamped:
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTimestamped, &CLSID_Timestamped>,
	public ISupportErrorInfo,
	public IDispatchImpl<ITimestamped, &IID_ITimestamped, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	PointDifference difference_;
public:

	BEGIN_COM_MAP(CTimestamped)
		COM_INTERFACE_ENTRY(ITimestamped)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
	END_COM_MAP()

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

	STDMETHOD(get_Time)(double* Time);
	STDMETHOD(get_Value)(double* Value);
};

#endif

