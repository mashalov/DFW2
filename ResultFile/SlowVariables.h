// SlowVariables.h : Declaration of the CSlowVariables

#pragma once
#include "resource.h"       // main symbols
#ifdef _MSC_VER
#include "ResultFile_i.h"
#include "SlowVariable.h"
#include "ResultFileReader.h"
#include "GenericCollection.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CVariables

class ATL_NO_VTABLE CSlowVariables:
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSlowVariables, &CLSID_SlowVariables>,
	public ISupportErrorInfo,
	public IDispatchImpl<CGenericCollection<ISlowVariables, CSlowVariable>, &IID_IVariables, &LIBID_ResultFileLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	CResultFileReader *pFileReader_;
public:
	CSlowVariables() : pFileReader_(nullptr)
	{
	}

	BEGIN_COM_MAP(CSlowVariables)
		COM_INTERFACE_ENTRY(ISlowVariables)
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
		CGenericCollection<ISlowVariables, CSlowVariable>::CollectionFinalRelease();
	}
public:
	void SetResultFileReader(CResultFileReader *pReader)
	{
		_ASSERTE(pReader);
		pFileReader_ = pReader;
	}
	STDMETHOD(Find)(LONG DeviceTypeId, VARIANT DeviceIds, BSTR VariableName, VARIANT *SlowVariable);

};

//OBJECT_ENTRY_AUTO(__uuidof(VariablesCollection), CVariablesCollection)
#endif
