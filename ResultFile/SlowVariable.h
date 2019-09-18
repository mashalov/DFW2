// SlowVariable.h : Declaration of the SlowVariable

#pragma once
#include "resource.h"       // main symbols
#include "ResultFile_i.h"
#include "ResultFileReader.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;
using namespace DFW2;


// CSlowVariable

class ATL_NO_VTABLE CSlowVariable :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSlowVariable, &CLSID_SlowVariable>,
	public ISupportErrorInfo,
	public IDispatchImpl<ISlowVariable, &IID_ISlowVariable, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	CSlowVariableItem *m_pItem;
public:
	CSlowVariable() : m_pItem(nullptr)
	{
	}

	BEGIN_COM_MAP(CSlowVariable)
		COM_INTERFACE_ENTRY(ISlowVariable)
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
	STDMETHOD(get_Name)(BSTR* Name);
	STDMETHOD(get_Plot)(VARIANT* Plot);
	STDMETHOD(get_DeviceType)(LONG* Type);
	STDMETHOD(get_DeviceIds)(VARIANT* Ids);
	void SetVariableInfo(CSlowVariableItem *pItem);
};
