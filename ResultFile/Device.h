﻿// Device.h : Declaration of the CDevice

#pragma once
#include "resource.h"       // main symbols

#ifdef _MSC_VER
#include "ResultFile_i.h"
#endif

#include "ResultFileReader.h"

using namespace DFW2;



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



#ifdef _MSC_VER
using namespace ATL;
// CDevice

class ATL_NO_VTABLE CDevice :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDevice, &CLSID_Device>,
	public ISupportErrorInfo,
	public IDispatchImpl<IDevice, &IID_IDevice, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	DeviceInstanceInfo* pDeviceInfo_ = nullptr;
public:

	void SetDeviceInfo(DeviceInstanceInfo* pDeviceInfo)
	{
		pDeviceInfo_ = pDeviceInfo;
	}

BEGIN_COM_MAP(CDevice)
	COM_INTERFACE_ENTRY(IDevice)
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

	STDMETHOD(get_Id)(LONG* Id);
	STDMETHOD(get_Name)(BSTR* Name);
	STDMETHOD(get_Type)(LONG* Type);
	STDMETHOD(get_Children)(VARIANT* Children);
	STDMETHOD(get_Variables)(VARIANT* Variables);
	STDMETHOD(get_TypeName)(BSTR* TypeName);
	STDMETHOD(get_HasVariables)(VARIANT_BOOL* HasVariables);


};

class CRootDevice : public CDevice
{
public:
	STDMETHOD(get_Children)(VARIANT* Children);
};

#endif

//OBJECT_ENTRY_AUTO(__uuidof(Device), CDevice)
