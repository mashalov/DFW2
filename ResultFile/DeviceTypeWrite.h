// DeviceTypeWrite.h : Declaration of the CDeviceTypeWrite

#pragma once
#include "resource.h"       // main symbols
#include "ResultFile_i.h"
#include "ResultFileReader.h"



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;
using namespace DFW2;


// CDeviceTypeWrite

class ATL_NO_VTABLE CDeviceTypeWrite :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDeviceTypeWrite, &CLSID_DeviceTypeWrite>,
	public ISupportErrorInfo,
	public IDispatchImpl<IDeviceTypeWrite, &IID_IDeviceTypeWrite, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	CResultFileReader::DeviceTypeInfo *m_pDeviceTypeInfo;
	CResultFileReader::DeviceInstanceInfo *m_pDeviceInstanceInfo;
	long GetParameterByIndex(VARIANT& vt, long nIndex);
public:
	CDeviceTypeWrite() : m_pDeviceTypeInfo(NULL)
	{
	}

BEGIN_COM_MAP(CDeviceTypeWrite)
	COM_INTERFACE_ENTRY(IDeviceTypeWrite)
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

	STDMETHOD(SetDeviceTypeMetrics)(LONG DeviceIdsCount, LONG ParentIdsCount, LONG DevicesCount);
	STDMETHOD(AddDeviceTypeVariable)(BSTR VariableName, LONG UnitId, DOUBLE Multiplier);
	STDMETHOD(AddDevice)(BSTR DeviceName, VARIANT DeviceIds, VARIANT ParentIds, VARIANT ParentTypes);
	void SetDeviceTypeInfo(CResultFileReader::DeviceTypeInfo *pDeviceTypeInfo);
	

};
