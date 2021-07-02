// Result.h : Declaration of the CResult

#pragma once
#ifdef _MSC_VER
#include "resource.h"       // main symbols
#include "ResultFile_i.h"
#include "Device.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

// CResult

class ATL_NO_VTABLE CResultRead :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CResultRead, &CLSID_ResultRead>,
	public ISupportErrorInfo,
	public IDispatchImpl<IResultRead, &IID_IResultRead, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	CResultFileReader m_ResultFileReader;
	DeviceTypeInfo m_DeviceTypeInfo;
public:
	CResultRead()
	{
		m_DeviceTypeInfo.m_pFileReader = &m_ResultFileReader;
		m_DeviceTypeInfo.DevicesCount = 1;
		m_DeviceTypeInfo.DeviceParentIdsCount = 0;
		m_DeviceTypeInfo.DeviceIdsCount = 1;
		m_DeviceTypeInfo.eDeviceType = 0;
		m_DeviceTypeInfo.AllocateData();
		m_DeviceTypeInfo.m_pDeviceInstances.get()->nIndex = 0;
		m_DeviceTypeInfo.m_pDeviceInstances.get()->SetId(0, 0);
		m_DeviceTypeInfo.m_pDeviceInstances.get()->Name = CDFW2Messages::m_cszResultRoot;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_RESULT)


BEGIN_COM_MAP(CResultRead)
	COM_INTERFACE_ENTRY(IResultRead)
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
		m_ResultFileReader.Close();
	}

public:
	STDMETHOD(get_Path)(BSTR* PathName);
	STDMETHOD(get_TimeCreated)(VARIANT* FileTime);
	STDMETHOD(get_Comment)(BSTR* Comment);
	STDMETHOD(get_TimeScale)(VARIANT* TimeScale);
	STDMETHOD(get_TimeStep)(VARIANT* TimeStep);
	STDMETHOD(get_Version)(LONG* Version);
	STDMETHOD(get_Root)(VARIANT* Root);
	STDMETHOD(get_Types)(VARIANT* Types);
	STDMETHOD(ExportCSV)(BSTR PathName);
	STDMETHOD(get_SlowVariables)(VARIANT *SlowVariables);
	STDMETHOD(GetPlotByIndex)(long nIndex, VARIANT *Plot);
	STDMETHOD(get_Points)(LONG *PointsCount);
	STDMETHOD(get_Channels)(LONG* ChannelsCount);
	STDMETHOD(GetPlot)(LONG DeviceType, LONG DeviceId, BSTR VariableName, VARIANT *Plot);
	STDMETHOD(Close)();
	STDMETHOD(get_UserComment)(BSTR* UserComment);
	STDMETHOD(put_UserComment)(BSTR UserComment);
	STDMETHOD(get_CompressionRatio)(DOUBLE *pRatio);
	void OpenFile(std::string_view PathName);
};

OBJECT_ENTRY_AUTO(__uuidof(ResultRead), CResultRead)
#endif