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
	//_CrtSetBreakAlloc(202);

	SetConsoleCtrlHandler(HandlerRoutine,TRUE);


		using GraphType = GraphCycle<int>;
		using NodeType = GraphType::GraphNodeBase;
		using EdgeType = GraphType::GraphEdgeBase;

		int nodes[] = {1, 2, 3, 4 };
		int edges[] = {1, 2, 2, 3, 3, 4, 4, 1, 1,3};
		unique_ptr<NodeType[]> pGraphNodes = make_unique<NodeType[]>(_countof(nodes));
		unique_ptr<EdgeType[]> pGraphEdges = make_unique<EdgeType[]>(_countof(edges) / 2);
		NodeType *pNode = pGraphNodes.get();
		GraphType gc;

		for (int *p = nodes ; p < _countof(nodes) + nodes; p++, pNode++)
			gc.AddNode(pNode->SetId(*p));

		EdgeType *pEdge = pGraphEdges.get();
		for (int *p = edges ; p < _countof(edges) + edges; p+= 2, pEdge++)
			gc.AddEdge(pEdge->SetIds(*p,  *(p+1)));

		gc.GenerateCycles();


	CoInitialize(NULL);
	{
		CDynaModel Network;
		CRastrImport ri;
		ri.GetData(Network);
		Network.Run();
	}
	_CrtDumpMemoryLeaks();

	return 0;
}

