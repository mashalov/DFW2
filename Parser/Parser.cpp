// Parser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "AutoCompiler.h"

int _tmain(int argc, _TCHAR* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	//_crtBreakAlloc = 27802;

	{
		bool bRes = true;

		CCompilerLogger Logger;
		CAutoCompiler cac(Logger);

		/*
        cac.AddStarter(CAutoStarterItem(CAutoItem(0, 1,_T("name1"),_T("V/1000 + S4 + A1")),CCompilerModelLink(0,_T("node"),_T("1123"),_T("vras"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 2, _T("name2"), _T("V/1000")), CCompilerModelLink(0, _T("node"), _T("1125"), _T("vras"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 3, _T("name3"), _T("sqrt(S1^2+S2^2)/S4^2")), CCompilerModelLink(0, _T("node"), _T("1125"), _T("vras"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 4, _T("name4"), _T("derlag(V,0.45,BASE+3)")), CCompilerModelLink(0, _T("node"), _T("1128"), _T("vras"))));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 1, _T("Logic1"), _T("derlag(S1+S2,0.4,1.0) + derlag(node[15].pn,0.004,1.0)")), _T("A1,A2"), _T("4"), 1));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 2, _T("Logic2"), _T("L1+S2")), _T("A1,A2"), _T("2*5"), 1));
		cac.AddAction(CAutoActionItem(CAutoItem(0, 1, _T("Action"), _T("V+V")), CCompilerModelLink(0, _T("vetv"), _T("2,3,4"), _T("sta")),1,1,1));
        */

		/*
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 1, _T("Напряжение узла"), _T("")), CCompilerModelLink(0, _T("node"), _T("1"), _T("vras"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 2, _T("Активная мощность"), _T("")), CCompilerModelLink(0, _T("vetv"), _T("1,2"), _T("pl_ip"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 3, _T("Рективная мощность"), _T("")), CCompilerModelLink(0, _T("vetv"), _T("1,2"), _T("pl_iq"))));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 1, _T("Ток"), _T("sqrt(3)*sqrt(S2^2+S3^2)/S1*1000")), _T(""), _T("0"), 1));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 2, _T("Реле"), _T("L1-5000")), _T("A1"), _T("0.03"), 1));
		cac.AddAction(CAutoActionItem(CAutoItem(0, 1, _T("Откл ветви"), _T("105")), CCompilerModelLink(0, _T("vetv"), _T("1,2"), _T("sta")), 1, 1, 1));
        */
		
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 1, _T("Напряжение узла"), _T("5*exp(-t/10)*sin(t*15)")), CCompilerModelLink(0, _T("node"), _T("1018"), _T("V"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 2, _T("Активная мощность"), _T("V + S1")), CCompilerModelLink(0, _T("vetv"), _T("1214,1114"), _T("Pb"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 3, _T("Рективная мощность"), _T("")), CCompilerModelLink(0, _T("vetv"), _T("1214,1114 "), _T("Qb"))));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 1, _T("Ток"), _T("sqrt(3*S2^2+S3^2)-116")), _T(""), _T("0"), 1));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 2, _T("Реле"), _T("1+S1+S2+55")), _T("A1"), _T("0"), 1));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 3, _T("Ограничитель"), _T("limit(L2,-5,5)")), _T(""), _T("0"), 1));
		cac.AddAction(CAutoActionItem(CAutoItem(0, 1, _T("Откл ветви"), _T("105 + LT1")), CCompilerModelLink(0, _T("vetv"), _T("1111,1646, 2"), _T("sta")), 1, 1, 1));

		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 4, _T("Выдержка"), _T("V>10.15")), CCompilerModelLink(0, _T("node"), _T("1018 "), _T("V"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 5, _T("Выдержка"), _T("BASE")), CCompilerModelLink(0, _T("node"), _T("1018 "), _T("V"))));

		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 13, _T("sin1"), _T("sin(t*15)")), CCompilerModelLink(0, _T("node"), _T("1018 "), _T("V"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 12, _T("bitz"), _T("S13>0.5|S17>0.99")), CCompilerModelLink(0, _T("node"), _T("1018 "), _T("V"))));

		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 17, _T("sin2"), _T("2*sin(t*20)")), CCompilerModelLink(0, _T("node"), _T("1018 "), _T("V"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 18, _T("Comp"), _T("S12>S17")), CCompilerModelLink(0, _T("node"), _T("1018 "), _T("V"))));
		cac.AddStarter(CAutoStarterItem(CAutoItem(0, 19, _T("Comp"), _T("S13<S17")), CCompilerModelLink(0, _T("node"), _T("1018 "), _T("V"))));

		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 5, _T("Deadband"), _T("deadband(abs(S1),3.5)")), _T(""), _T("0.1"), 1));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 6, _T("Deadband2"), _T("expand(LT5,0.03)")), _T(""), _T("0.02"), 1));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 7, _T("Deadband3"), _T("shrink(L6,0.02)")), _T(""), _T("0.02"), 1));
		cac.AddLogic(CAutoLogicItem(CAutoItem(0, 8, _T("Deadband4"), _T("expand(L7,0.04)")), _T(""), _T("0.02"), 1));


		bRes = cac.Generate();

		_ASSERTE(bRes);

		CExpressionParserRules rps;
		rps.Process(_T("relay(0,S1^2-S2^2,1)"));
		wstring str;
		rps.Infix(str);

		str = _T("");
	}

	return 0;
}

