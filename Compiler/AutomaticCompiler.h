// AutomaticCompiler.h : Declaration of the CAutomaticCompiler

#pragma once
#include "resource.h"       // main symbols
#include "Compiler_i.h"
#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

#include "Parser/AutoCompiler.h"
using namespace ATL;


// CAutomaticCompiler

class ATL_NO_VTABLE CAutomaticCompiler :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAutomaticCompiler, &CLSID_AutomaticCompiler>,
	public ISupportErrorInfo,
	public IDispatchImpl<IAutomaticCompiler, &IID_IAutomaticCompiler, &LIBID_CompilerLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
protected:
	std::wstring strCompilerLogTag = cszLCCCompiler;

	CCompilerLogger m_CompilerLogger;
	std::unique_ptr<CAutoCompiler> m_pAutoCompiler;
	std::wstring m_strCompilerRoot;
	std::wstring m_strModelRoot;
	std::wstring m_strModelDLLName;
public:
	CAutomaticCompiler()
	{
#ifdef _WIN64
		m_strCompilerRoot = _T("c:\\lcc64");
#else
		m_strCompilerRoot	= _T("c:\\lcc");
#endif
		m_strModelRoot		= _T("c:\\tmp\\\\\\CustomModels\\");
		m_strModelDLLName	= _T("autodll");
	}

DECLARE_REGISTRY_RESOURCEID(IDR_AUTOMATICCOMPILER)


BEGIN_COM_MAP(CAutomaticCompiler)
	COM_INTERFACE_ENTRY(IAutomaticCompiler)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		m_pAutoCompiler = std::make_unique<CAutoCompiler>(m_CompilerLogger);
		return S_OK;
	}

	void FinalRelease()
	{

	}

	static const _TCHAR *cszLCCCompiler;
	static const _TCHAR *cszCompileCreatePipeFailed;
	static const _TCHAR *cszCompileCreateProcessFailed;

public:
	STDMETHOD(AddStarter)(LONG Type, LONG Id, BSTR Name, BSTR Expression, LONG LinkType, BSTR ObjectClass, BSTR ObjectKey, BSTR ObjectProp);
	STDMETHOD(AddLogic)(LONG Type, LONG Id, BSTR Name, BSTR Expression, BSTR Actions, BSTR DelayExpression, LONG OutputMode);
	STDMETHOD(AddAction)(LONG Type, LONG Id, BSTR Name, BSTR Expression, LONG LinkType, BSTR ObjectClass, BSTR ObjectKey, BSTR ObjectProp, LONG ActionGroup, LONG OutputMode, LONG RunsCount);
	STDMETHOD(Compile)();
	STDMETHOD(get_Log)(VARIANT* LogArray);
};

OBJECT_ENTRY_AUTO(__uuidof(AutomaticCompiler), CAutomaticCompiler)
