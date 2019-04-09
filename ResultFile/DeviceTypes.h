// DeviceTypes.h : Declaration of the CDeviceTypes

#pragma once
#include "resource.h"       // main symbols
#include "ResultFile_i.h"
#include "DeviceType.h"
#include "GenericCollection.h"
#include "ResultFileReader.h"

using namespace DFW2;



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CDeviceTypes

class ATL_NO_VTABLE CDeviceTypes :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDeviceTypes, &CLSID_DeviceTypes>,
	public ISupportErrorInfo,
	public IDispatchImpl<CGenericCollection<IDeviceTypes, CDeviceType>, &IID_IDeviceTypes, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	const CResultFileReader::DEVTYPESET *m_pDevTypeSet;

public:
	CDeviceTypes()
	{
	}

	void SetDeviceTypesInfo(const CResultFileReader::DEVTYPESET *pDevTypeSet)
	{
		m_pDevTypeSet = pDevTypeSet;
	}

BEGIN_COM_MAP(CDeviceTypes)
	COM_INTERFACE_ENTRY(IDeviceTypes)
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
		CGenericCollection<IDeviceTypes, CDeviceType>::CollectionFinalRelease();
	}

public:



};

