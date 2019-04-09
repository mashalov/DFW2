// dllmain.h : Declaration of module class.

class CCompilerModule : public ATL::CAtlDllModuleT< CCompilerModule >
{
public :
	DECLARE_LIBID(LIBID_CompilerLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_COMPILER, "{C1ADF372-02B5-4DFA-B548-E6180B38EC56}")
};

extern class CCompilerModule _AtlModule;
