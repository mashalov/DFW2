// DFW2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DynaModel.h"
#include "RastrImport.h"
#include "GraphCycle.h"


using namespace DFW2;
/*
struct CrtBreakAllocSetter {
	CrtBreakAllocSetter() {
		_crtBreakAlloc = 211;
	}
};

CrtBreakAllocSetter g_crtBreakAllocSetter;

*/


std::list<CDynaModel*> networks;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	for (auto&& nw : networks)
		nw->StopProcess();
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	//_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
	//_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(260215);
	//_CrtSetBreakAlloc(31657);
	//_CrtSetBreakAlloc(236965);
	
	//_CrtSetBreakAlloc(1229197);

	/*
	GraphCycle<int, int> gc;
	GraphCycle<int, int>::CyclesType Cycles;
	gc.Test();
	*/
		
	if(SUCCEEDED(CoInitialize(NULL)))
	{

		try
		{
			CDynaModel::DynaModelParameters parameters;
			CDynaModel Network(parameters);

			SetConsoleCtrlHandler(HandlerRoutine, TRUE);
			try
			{
				CRastrImport ri;
				networks.push_back(&Network);
				//Network.DeSerialize(Network.Platform().ResultFile("serialization.json"));
				ri.GetData(Network);
				//Network.Serialize(Network.Platform().ResultFile("lf_test.json"));
				Network.Serialize(Network.Platform().ResultFile("lf_7ku.json")); 
				//Network.RunLoadFlow();
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

		//Network.Serialize("c:\\tmp\\lf.json");
		SetConsoleCtrlHandler(NULL, TRUE);
		networks.clear();
		CoUninitialize();
	}
	_CrtDumpMemoryLeaks();

	return 0;
}

