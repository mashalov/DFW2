// dllmain.h : Declaration of module class.

class CResultFileModule : public ATL::CAtlDllModuleT< CResultFileModule >
{
public :
	DECLARE_LIBID(LIBID_ResultFileLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_RESULTFILE, "{8261E2E9-BD01-4561-BCB3-13642506B84E}")
};

extern class CResultFileModule _AtlModule;
