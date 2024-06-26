﻿// Result2.h : Declaration of the CResult2

#pragma once

#ifdef _MSC_VER
#include "resource.h"       // main symbols

#include "ResultFile_i.h"



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CResult2

class ATL_NO_VTABLE CResult :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CResult, &CLSID_Result>,
	public ISupportErrorInfo,
	public IDispatchImpl<IResult, &IID_IResult, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CResult()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_RESULT)


BEGIN_COM_MAP(CResult)
	COM_INTERFACE_ENTRY(IResult)
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
	STDMETHOD(Load)(BSTR PathName, VARIANT *ResultRead);
	STDMETHOD(Create)(BSTR PathName, VARIANT *ResultWrite);


};

OBJECT_ENTRY_AUTO(__uuidof(Result), CResult)

#endif
