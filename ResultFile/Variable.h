// Variable.h : Declaration of the CVariable

#pragma once
#include "resource.h"       // main symbols
#ifdef _MSC_VER
#include "ResultFile_i.h"
#endif
#include "ResultFileReader.h"



#ifdef _MSC_VER

using namespace DFW2;

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

// CVariable

class ATL_NO_VTABLE CVariable :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVariable, &CLSID_Variable>,
	public ISupportErrorInfo,
	public IDispatchImpl<IVariable, &IID_IVariable, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	const VariableTypeInfo* m_pVariableInfo = nullptr;
	DeviceInstanceInfo* m_pDeviceInstanceInfo = nullptr;
public:

	void SetVariableInfo(const VariableTypeInfo* pVariableInfo, DeviceInstanceInfo *pDeviceInstanceInfo)
	{
		m_pVariableInfo = pVariableInfo;
		m_pDeviceInstanceInfo = pDeviceInstanceInfo;
	}

BEGIN_COM_MAP(CVariable)
	COM_INTERFACE_ENTRY(IVariable)
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

	STDMETHOD(get_Index)(LONG* Index);
	STDMETHOD(get_Name)(BSTR* Name);
	STDMETHOD(get_Units)(LONG* Units);
	STDMETHOD(get_Multiplier)(DOUBLE* Multiplier);
	STDMETHOD(get_Plot)(VARIANT* Plot);
	STDMETHOD(get_UnitsName)(BSTR* UnitsName);
	STDMETHOD(get_Device)(VARIANT* Device);
	STDMETHOD(get_ChannelIndex)(LONG* Index);
};
#endif
