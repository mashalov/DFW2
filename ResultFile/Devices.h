﻿// Devices.h : Declaration of the CDevices

#pragma once
#include "resource.h"       // main symbols
#include "Device.h"
#include "GenericCollection.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CDevices

class ATL_NO_VTABLE CDevices:
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDevices, &CLSID_Devices>,
	public ISupportErrorInfo,
	public IDispatchImpl<CGenericCollection<IDevices, CDevice> , &IID_IDevices, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CDevices()
	{
	}

BEGIN_COM_MAP(CDevices)
	COM_INTERFACE_ENTRY(IDevices)
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
		CollectionFinalRelease();
	}

public:
};

//OBJECT_ENTRY_AUTO(__uuidof(DeviceCollection), CDeviceCollection)
