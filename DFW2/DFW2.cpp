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

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	HANDLE hEvt = CreateEvent(NULL, TRUE, FALSE, _T("DFW2STOP"));
	SetEvent(hEvt);
	CloseHandle(hEvt);
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	//_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
	//_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(31658);
	//_CrtSetBreakAlloc(31657);
	//_CrtSetBreakAlloc(5821);

	/*
	GraphCycle<int, int> gc;
	GraphCycle<int, int>::CyclesType Cycles;
	gc.Test();
	*/

	SetConsoleCtrlHandler(HandlerRoutine,TRUE);

	CoInitialize(NULL);
	{
		CDynaModel Network;
		CRastrImport ri;
		try
		{
			ri.GetData(Network);
			Network.Run();
		}
		catch (dfw2error& err)
		{
			Network.Log(CDFW2Messages::DFW2LOG_FATAL, Cex(_T("Ошибка : %s"), err.uwhat()));
		}
	}
	_CrtDumpMemoryLeaks();

	return 0;
}

