// ResultWrite.h : Declaration of the CResultWrite

#pragma once
#include "resource.h"       // main symbols
#include "ResultFile_i.h"
#include "DeviceTypeWrite.h"
#include "ResultFileWriter.h"


using namespace DFW2;



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CResultWrite

class ATL_NO_VTABLE CResultWrite :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CResultWrite, &CLSID_ResultWrite>,
	public ISupportErrorInfo,
	public IDispatchImpl<IResultWrite, &IID_IResultWrite, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	CResultFileWriter m_ResultFileWriter;
public:
	CResultWrite()
	{
	}

	~CResultWrite();

BEGIN_COM_MAP(CResultWrite)
	COM_INTERFACE_ENTRY(IResultWrite)
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

	STDMETHOD(put_Comment)(BSTR Comment);
	STDMETHOD(put_NoChangeTolerance)(DOUBLE Tolerance);
	STDMETHOD(WriteHeader)();
	STDMETHOD(AddVariableUnit)(LONG UnitId, BSTR UnitName);
	STDMETHOD(AddDeviceType)(LONG DeviceTypeId, BSTR DeviceTypeName, VARIANT* DeviceType);
	STDMETHOD(SetChannel)(LONG DeviceId, LONG DeviceType, LONG VarIndex, DOUBLE *ValuePtr, LONG ChannelIndex);
	STDMETHOD(WriteResults)(DOUBLE Time, DOUBLE Step);
	STDMETHOD(FlushChannels)();
	STDMETHOD(Close)();
	STDMETHOD(AddSlowVariable)(LONG DeviceTypeId, VARIANT DeviceIds, BSTR VariableName, DOUBLE Time, DOUBLE Value, DOUBLE PreviousValue, BSTR ChangeDescription);
	void CreateFile(std::string_view PathName);
};

