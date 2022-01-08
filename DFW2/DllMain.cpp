#include "stdafx.h"
#ifdef _MSC_VER
#include "RastrImport.h"

using namespace DFW2;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) LONG __cdecl Run(IRastrPtr spRastr)
{
	try
	{
		CDynaModel::DynaModelParameters parameters;
		parameters.m_bLogToConsole = true;
		CDynaModel Network(parameters);
		Network.SetLogger(std::make_unique<CLoggerRastrWin>(spRastr));
		Network.SetProgress(std::make_unique<CProgressRastrWin>(spRastr));

		try
		{
			CRastrImport ri(spRastr);
			ri.GetData(Network);
			Network.RunTransient();
		}
		catch (_com_error& err)
		{
			Network.Log(DFW2MessageStatus::DFW2LOG_FATAL, fmt::format("Ошибка COM : {}", stringutils::utf8_encode(std::wstring(err.Description()))));
		}
		catch (const dfw2error& err)
		{
			Network.Log(DFW2MessageStatus::DFW2LOG_FATAL, fmt::format("Ошибка : {}", err.what()));
		}
}
	catch (const dfw2error& err)
	{
		std::cout << "Ошибка " << err.what() << std::endl;
	}
    return 0;
}
#else
#include "CompilerGCC.h"
extern "C" __attribute__((visibility("default"))) ICompiler * CompilerFactory()
{
    return new CCompilerGCC();
}

#endif
