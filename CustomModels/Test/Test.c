
#include "..\Headers\DeviceDLL.h"


__declspec(dllexport) const _TCHAR* GetDeviceTypeName(void)
{
	return _T("Automatic & scenario");
}

__declspec(dllexport) long Destroy(void)
{
	return 1;
}

__declspec(dllexport) long GetTypesCount(void)
{
	return 1;
}

__declspec(dllexport) long GetTypes(long* pTypes)
{
	pTypes[0] = DEVTYPE_AUTOMATIC;
	return GetTypesCount();
}

__declspec(dllexport) long GetLinksCount(void)
{
	return 1;
}

__declspec(dllexport) long GetLinks(LinkType* pLink)
{
	SetLink(pLink + 0, eLINK_FROM, DEVTYPE_MODEL, DLM_SINGLE, DPD_MASTER, _T(""));
	return GetLinksCount();
}

__declspec(dllexport) long GetInputsCount(void)
{
	return 3;
}

__declspec(dllexport) long GetOutputsCount(void)
{
	return 25;
}

__declspec(dllexport) long GetSetPointsCount(void)
{
	return 1;
}

__declspec(dllexport) long GetConstantsCount(void)
{
	return 0;
}

__declspec(dllexport) long GetBlocksCount(void)
{
	return 19;
}

__declspec(dllexport) long GetInternalsCount(void)
{
	return 45;
}

__declspec(dllexport) long GetBlocksDescriptions(BlockDescriptions *pBlockDescriptions)
{
	SetBlockDescription(pBlockDescriptions +     0, PBT_HIGHER          ,     0, _T("") );
	SetBlockDescription(pBlockDescriptions +     1, PBT_HIGHER          ,     1, _T("") );
	SetBlockDescription(pBlockDescriptions +     2, PBT_HIGHER          ,     2, _T("") );
	SetBlockDescription(pBlockDescriptions +     3, PBT_OR              ,     3, _T("") );
	SetBlockDescription(pBlockDescriptions +     4, PBT_HIGHER          ,     4, _T("") );
	SetBlockDescription(pBlockDescriptions +     5, PBT_LOWER           ,     5, _T("") );
	SetBlockDescription(pBlockDescriptions +     6, PBT_RELAYDELAY      ,     6, _T("") );
	SetBlockDescription(pBlockDescriptions +     7, PBT_RELAYDELAY      ,     7, _T("") );
	SetBlockDescription(pBlockDescriptions +     8, PBT_LIMITERCONST    ,     8, _T("") );
	SetBlockDescription(pBlockDescriptions +     9, PBT_RELAYDELAY      ,     9, _T("") );
	SetBlockDescription(pBlockDescriptions +    10, PBT_ABS             ,    10, _T("") );
	SetBlockDescription(pBlockDescriptions +    11, PBT_DEADBAND        ,    11, _T("") );
	SetBlockDescription(pBlockDescriptions +    12, PBT_RELAYDELAY      ,    12, _T("") );
	SetBlockDescription(pBlockDescriptions +    13, PBT_EXPAND          ,    13, _T("") );
	SetBlockDescription(pBlockDescriptions +    14, PBT_RELAYDELAY      ,    14, _T("") );
	SetBlockDescription(pBlockDescriptions +    15, PBT_SHRINK          ,    15, _T("") );
	SetBlockDescription(pBlockDescriptions +    16, PBT_RELAYDELAY      ,    16, _T("") );
	SetBlockDescription(pBlockDescriptions +    17, PBT_EXPAND          ,    17, _T("") );
	SetBlockDescription(pBlockDescriptions +    18, PBT_RELAYDELAY      ,    18, _T("") );

	return GetBlocksCount();
}

__declspec(dllexport) long GetBlockPinsCount(long nBlockIndex)
{
	const long PinsCount[19] = { 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

	if(nBlockIndex >= 0 && nBlockIndex < 19)
		return PinsCount[nBlockIndex];

	return -1;
}

__declspec(dllexport) long GetBlockPinsIndexes(long nBlockIndex, BLOCKPININDEX* pBlockPins)
{
	switch(nBlockIndex)
	{
	case 0:
		SetPinInfo(pBlockPins +     0,     9, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,     7, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     2,     8, eVL_INTERNAL);
		break;

	case 1:
		SetPinInfo(pBlockPins +     0,    12, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    17, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     2,    11, eVL_INTERNAL);
		break;

	case 2:
		SetPinInfo(pBlockPins +     0,    14, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    20, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     2,    13, eVL_INTERNAL);
		break;

	case 3:
		SetPinInfo(pBlockPins +     0,    15, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    12, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     2,    14, eVL_INTERNAL);
		break;

	case 4:
		SetPinInfo(pBlockPins +     0,    21, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    15, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     2,    20, eVL_INTERNAL);
		break;

	case 5:
		SetPinInfo(pBlockPins +     0,    22, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    17, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     2,    20, eVL_INTERNAL);
		break;

	case 6:
		SetPinInfo(pBlockPins +     0,    31, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    28, eVL_INTERNAL);
		break;

	case 7:
		SetPinInfo(pBlockPins +     0,    35, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    32, eVL_INTERNAL);
		break;

	case 8:
		SetPinInfo(pBlockPins +     0,    38, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    32, eVL_INTERNAL);
		break;

	case 9:
		SetPinInfo(pBlockPins +     0,    41, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    38, eVL_INTERNAL);
		break;

	case 10:
		SetPinInfo(pBlockPins +     0,    42, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,     4, eVL_INTERNAL);
		break;

	case 11:
		SetPinInfo(pBlockPins +     0,    44, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    42, eVL_INTERNAL);
		break;

	case 12:
		SetPinInfo(pBlockPins +     0,    47, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    44, eVL_INTERNAL);
		break;

	case 13:
		SetPinInfo(pBlockPins +     0,    49, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    47, eVL_INTERNAL);
		break;

	case 14:
		SetPinInfo(pBlockPins +     0,    52, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    49, eVL_INTERNAL);
		break;

	case 15:
		SetPinInfo(pBlockPins +     0,    54, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    49, eVL_INTERNAL);
		break;

	case 16:
		SetPinInfo(pBlockPins +     0,    57, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    54, eVL_INTERNAL);
		break;

	case 17:
		SetPinInfo(pBlockPins +     0,    59, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    54, eVL_INTERNAL);
		break;

	case 18:
		SetPinInfo(pBlockPins +     0,    62, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    59, eVL_INTERNAL);
		break;

	}
	return GetBlockPinsCount(nBlockIndex);
}

__declspec(dllexport) long GetBlockParametersCount(long nBlockIndex)
{
	const long ParametersCount[19] = { 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 1, 2, 1, 2, 1, 2, 1, 2 };

	if(nBlockIndex >= 0 && nBlockIndex < 19)
		return ParametersCount[nBlockIndex];

	return -1;
}


__declspec(dllexport) long GetSetPointsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo +     0,     0, _T("#node[1018].V"), VARUNIT_NOTSET, 0);

	return GetSetPointsCount();
}

__declspec(dllexport) long GetInputsInfos(InputVarsInfo *pVarsInfo)
{
	SetInputVarsInfo(pVarsInfo +     2, _T("node[1018].V"), DEVTYPE_MODEL, _T("") );
	SetInputVarsInfo(pVarsInfo +     0, _T("vetv[1214,1114].Pb"), DEVTYPE_MODEL, _T("") );
	SetInputVarsInfo(pVarsInfo +     1, _T("vetv[1214,1114].Qb"), DEVTYPE_MODEL, _T("") );

	return GetInputsCount();
}

__declspec(dllexport) long GetConstantsInfos(ConstVarsInfo *pVarsInfo)
{

	return GetConstantsCount();
}

__declspec(dllexport) long GetOutputsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo +     0,    63, _T("A1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     1,    28, _T("L1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     2,    32, _T("L2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     3,    38, _T("L3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     4,    44, _T("L5"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     5,    49, _T("L6"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     6,    54, _T("L7"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     7,    59, _T("L8"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     8,    31, _T("LT1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     9,    35, _T("LT2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    10,    41, _T("LT3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    11,    47, _T("LT5"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    12,    52, _T("LT6"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    13,    57, _T("LT7"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    14,    62, _T("LT8"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    15,     4, _T("S1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    16,    15, _T("S12"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    17,    17, _T("S13"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    18,    20, _T("S17"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    19,    21, _T("S18"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    20,    22, _T("S19"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    21,     5, _T("S2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    22,     6, _T("S3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    23,     9, _T("S4"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    24,    10, _T("S5"), VARUNIT_NOTSET, 1);

	return GetOutputsCount();
}

__declspec(dllexport) long GetInternalsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo +     0,     0, _T("15.0 * t"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     1,     1, _T("sin(15.0 * t)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     2,     2, _T("-0.10000000000000001 * t"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     3,     3, _T("exp(-0.10000000000000001 * t)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     4,     4, _T("S1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     5,     5, _T("S2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     6,     6, _T("S3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     7,     7, _T("node[1018].V"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     8,     8, _T("10.15"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     9,    10, _T("#node[1018].V"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    10,    11, _T("0.5"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    11,    13, _T("0.99"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    12,    16, _T("15.0 * t"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    13,    17, _T("S13"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    14,    18, _T("20.0 * t"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    15,    19, _T("sin(20.0 * t)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    16,    20, _T("S17"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    17,    23, _T("pow(S2, 2)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    18,    24, _T("3.0 * pow(S2, 2)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    19,    25, _T("pow(S3, 2)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    20,    26, _T("3.0 * pow(S2, 2) + pow(S3, 2)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    21,    27, _T("pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    22,    28, _T("L1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    23,    29, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    24,    30, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    25,    32, _T("L2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    26,    33, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    27,    34, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    28,    36, _T("-5.0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    29,    37, _T("5"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    30,    39, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    31,    40, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    32,    43, _T("3.5"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    33,    45, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    34,    46, _T("0.1"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    35,    48, _T("0.03"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    36,    50, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    37,    51, _T("0.02"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    38,    53, _T("0.02"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    39,    55, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    40,    56, _T("0.02"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    41,    58, _T("0.04"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    42,    60, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    43,    61, _T("0.02"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    44,    63, _T("A1"), VARUNIT_NOTSET, 1);

	return GetInternalsCount();
}

__declspec(dllexport) long BuildEquations(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLSETELEMENT SetElement = pArgs->pFnSetElement;

	// 15.0 * t / 15.0 * t;
	(SetElement)(pm,     0, 0                                       , 1.0, 0);
	// sin(15.0 * t) by 15.0 * t
	(SetElement)(pm,     1, 0                                       , -(cos(pArgs->pEquations[0])), 0 );
	// sin(15.0 * t) / sin(15.0 * t);
	(SetElement)(pm,     1, 1                                       , 1.0, 0);
	// -0.10000000000000001 * t / -0.10000000000000001 * t;
	(SetElement)(pm,     2, 2                                       , 1.0, 0);
	// exp(-0.10000000000000001 * t) by -0.10000000000000001 * t
	(SetElement)(pm,     3, 2                                       , -(exp(pArgs->pEquations[2])), 0 );
	// exp(-0.10000000000000001 * t) / exp(-0.10000000000000001 * t);
	(SetElement)(pm,     3, 3                                       , 1.0, 0);
	// 5.0 * sin(15.0 * t) * exp(-0.10000000000000001 * t) by sin(15.0 * t)
	(SetElement)(pm,     4, 1                                       , -(pArgs->pEquations[3] * 5.0), 0 );
	// 5.0 * sin(15.0 * t) * exp(-0.10000000000000001 * t) by exp(-0.10000000000000001 * t)
	(SetElement)(pm,     4, 3                                       , -(pArgs->pEquations[1] * 5.0), 0 );
	// 5.0 * sin(15.0 * t) * exp(-0.10000000000000001 * t) / 5.0 * sin(15.0 * t) * exp(-0.10000000000000001 * t);
	(SetElement)(pm,     4, 4                                       , 1.0, 0);
	// S1 + vetv[1214,1114].Pb by S1
	(SetElement)(pm,     5, 4                                       , -(1), 0 );
	// S1 + vetv[1214,1114].Pb by vetv[1214,1114].Pb
	(SetElement)(pm,     5, pArgs->pExternals[0].nIndex             , -(1), 0 );
	// S1 + vetv[1214,1114].Pb / S1 + vetv[1214,1114].Pb;
	(SetElement)(pm,     5, 5                                       , 1.0, 0);
	// External variable vetv[1214,1114].Qb
	(SetElement)(pm,     6, pArgs->pExternals[1].nIndex             , -(1), 0 );
	// vetv[1214,1114].Qb / vetv[1214,1114].Qb;
	(SetElement)(pm,     6, 6                                       , 1.0, 0);
	// External variable node[1018].V
	(SetElement)(pm,     7, pArgs->pExternals[2].nIndex             , -(1), 0 );
	// node[1018].V / node[1018].V;
	(SetElement)(pm,     7, 7                                       , 1.0, 0);
	// 10.15 / 10.15;
	(SetElement)(pm,     8, 8                                       , 1.0, 0);
	// #node[1018].V / #node[1018].V;
	(SetElement)(pm,    10, 10                                      , 1.0, 0);
	// 0.5 / 0.5;
	(SetElement)(pm,    11, 11                                      , 1.0, 0);
	// 0.99 / 0.99;
	(SetElement)(pm,    13, 13                                      , 1.0, 0);
	// 15.0 * t / 15.0 * t;
	(SetElement)(pm,    16, 16                                      , 1.0, 0);
	// sin(15.0 * t) by 15.0 * t
	(SetElement)(pm,    17, 16                                      , -(cos(pArgs->pEquations[16])), 0 );
	// sin(15.0 * t) / sin(15.0 * t);
	(SetElement)(pm,    17, 17                                      , 1.0, 0);
	// 20.0 * t / 20.0 * t;
	(SetElement)(pm,    18, 18                                      , 1.0, 0);
	// sin(20.0 * t) by 20.0 * t
	(SetElement)(pm,    19, 18                                      , -(cos(pArgs->pEquations[18])), 0 );
	// sin(20.0 * t) / sin(20.0 * t);
	(SetElement)(pm,    19, 19                                      , 1.0, 0);
	// 2.0 * sin(20.0 * t) by sin(20.0 * t)
	(SetElement)(pm,    20, 19                                      , -(2.0), 0 );
	// 2.0 * sin(20.0 * t) / 2.0 * sin(20.0 * t);
	(SetElement)(pm,    20, 20                                      , 1.0, 0);
	// pow(S2, 2) by S2
	(SetElement)(pm,    23, 5                                       , -(2 * pArgs->pEquations[5]), 0 );
	// pow(S2, 2) / pow(S2, 2);
	(SetElement)(pm,    23, 23                                      , 1.0, 0);
	// 3.0 * pow(S2, 2) by pow(S2, 2)
	(SetElement)(pm,    24, 23                                      , -(3.0), 0 );
	// 3.0 * pow(S2, 2) / 3.0 * pow(S2, 2);
	(SetElement)(pm,    24, 24                                      , 1.0, 0);
	// pow(S3, 2) by S3
	(SetElement)(pm,    25, 6                                       , -(2 * pArgs->pEquations[6]), 0 );
	// pow(S3, 2) / pow(S3, 2);
	(SetElement)(pm,    25, 25                                      , 1.0, 0);
	// 3.0 * pow(S2, 2) + pow(S3, 2) by 3.0 * pow(S2, 2)
	(SetElement)(pm,    26, 24                                      , -(1), 0 );
	// 3.0 * pow(S2, 2) + pow(S3, 2) by pow(S3, 2)
	(SetElement)(pm,    26, 25                                      , -(1), 0 );
	// 3.0 * pow(S2, 2) + pow(S3, 2) / 3.0 * pow(S2, 2) + pow(S3, 2);
	(SetElement)(pm,    26, 26                                      , 1.0, 0);
	// pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5) by 3.0 * pow(S2, 2) + pow(S3, 2)
	(SetElement)(pm,    27, 26                                      , -(0.5 * pow(pArgs->pEquations[26], -0.5)), 0 );
	// pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5) / pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5);
	(SetElement)(pm,    27, 27                                      , 1.0, 0);
	// -116.0 + pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5) by pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5)
	(SetElement)(pm,    28, 27                                      , -(1), 0 );
	// -116.0 + pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5) / -116.0 + pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5);
	(SetElement)(pm,    28, 28                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    29, 29                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    30, 30                                      , 1.0, 0);
	// 56.0 + S1 + S2 by S1
	(SetElement)(pm,    32, 4                                       , -(1), 0 );
	// 56.0 + S1 + S2 by S2
	(SetElement)(pm,    32, 5                                       , -(1), 0 );
	// 56.0 + S1 + S2 / 56.0 + S1 + S2;
	(SetElement)(pm,    32, 32                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    33, 33                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    34, 34                                      , 1.0, 0);
	// -5.0 / -5.0;
	(SetElement)(pm,    36, 36                                      , 1.0, 0);
	// 5 / 5;
	(SetElement)(pm,    37, 37                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    39, 39                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    40, 40                                      , 1.0, 0);
	// 3.5 / 3.5;
	(SetElement)(pm,    43, 43                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    45, 45                                      , 1.0, 0);
	// 0.1 / 0.1;
	(SetElement)(pm,    46, 46                                      , 1.0, 0);
	// 0.03 / 0.03;
	(SetElement)(pm,    48, 48                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    50, 50                                      , 1.0, 0);
	// 0.02 / 0.02;
	(SetElement)(pm,    51, 51                                      , 1.0, 0);
	// 0.02 / 0.02;
	(SetElement)(pm,    53, 53                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    55, 55                                      , 1.0, 0);
	// 0.02 / 0.02;
	(SetElement)(pm,    56, 56                                      , 1.0, 0);
	// 0.04 / 0.04;
	(SetElement)(pm,    58, 58                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    60, 60                                      , 1.0, 0);
	// 0.02 / 0.02;
	(SetElement)(pm,    61, 61                                      , 1.0, 0);
	// 105.0 + LT1 by LT1
	(SetElement)(pm,    63, 31                                      , -(1), 0 );
	// 105.0 + LT1 / 105.0 + LT1;
	(SetElement)(pm,    63, 63                                      , 1.0, 0);

	return GetInternalsCount();
}

__declspec(dllexport) long BuildRightHand(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLSETFUNCTION SetFunction = pArgs->pFnSetFunction;

	// 15.0 * t
	(SetFunction)(pm,     0, pArgs->pEquations[0] - (15.0 * pArgs->t));
	// sin(15.0 * t)
	(SetFunction)(pm,     1, pArgs->pEquations[1] - (sin(pArgs->pEquations[0])));
	// -0.10000000000000001 * t
	(SetFunction)(pm,     2, pArgs->pEquations[2] - (-0.10000000000000001 * pArgs->t));
	// exp(-0.10000000000000001 * t)
	(SetFunction)(pm,     3, pArgs->pEquations[3] - (exp(pArgs->pEquations[2])));
	// 5.0 * sin(15.0 * t) * exp(-0.10000000000000001 * t)
	(SetFunction)(pm,     4, pArgs->pEquations[4] - (5.0 * pArgs->pEquations[1] * pArgs->pEquations[3]));
	// S1 + vetv[1214,1114].Pb
	(SetFunction)(pm,     5, pArgs->pEquations[5] - (pArgs->pEquations[4] + *pArgs->pExternals[0].pValue));
	// vetv[1214,1114].Qb
	(SetFunction)(pm,     6, pArgs->pEquations[6] - (*pArgs->pExternals[1].pValue));
	// node[1018].V
	(SetFunction)(pm,     7, pArgs->pEquations[7] - (*pArgs->pExternals[2].pValue));
	// 10.15
	(SetFunction)(pm,     8, pArgs->pEquations[8] - (10.15));
	// 9 node[1018].V>10.15 in host block 
	// #node[1018].V
	(SetFunction)(pm,    10, pArgs->pEquations[10] - (pArgs->pSetPoints[0]));
	// 0.5
	(SetFunction)(pm,    11, pArgs->pEquations[11] - (0.5));
	// 0.99
	(SetFunction)(pm,    13, pArgs->pEquations[13] - (0.99));
	// 15.0 * t
	(SetFunction)(pm,    16, pArgs->pEquations[16] - (15.0 * pArgs->t));
	// sin(15.0 * t)
	(SetFunction)(pm,    17, pArgs->pEquations[17] - (sin(pArgs->pEquations[16])));
	// 12 S13>0.5 in host block 
	// 20.0 * t
	(SetFunction)(pm,    18, pArgs->pEquations[18] - (20.0 * pArgs->t));
	// sin(20.0 * t)
	(SetFunction)(pm,    19, pArgs->pEquations[19] - (sin(pArgs->pEquations[18])));
	// 2.0 * sin(20.0 * t)
	(SetFunction)(pm,    20, pArgs->pEquations[20] - (2.0 * pArgs->pEquations[19]));
	// 14 S17>0.99 in host block 
	// 15 S13>0.5|S17>0.99 in host block 
	// 21 S12>S17 in host block 
	// 22 S13<S17 in host block 
	// pow(S2, 2)
	(SetFunction)(pm,    23, pArgs->pEquations[23] - (pow(pArgs->pEquations[5], 2)));
	// 3.0 * pow(S2, 2)
	(SetFunction)(pm,    24, pArgs->pEquations[24] - (3.0 * pArgs->pEquations[23]));
	// pow(S3, 2)
	(SetFunction)(pm,    25, pArgs->pEquations[25] - (pow(pArgs->pEquations[6], 2)));
	// 3.0 * pow(S2, 2) + pow(S3, 2)
	(SetFunction)(pm,    26, pArgs->pEquations[26] - (pArgs->pEquations[24] + pArgs->pEquations[25]));
	// pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5)
	(SetFunction)(pm,    27, pArgs->pEquations[27] - (pow(pArgs->pEquations[26], 0.5)));
	// -116.0 + pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5)
	(SetFunction)(pm,    28, pArgs->pEquations[28] - (-116.0 + pArgs->pEquations[27]));
	// 0
	(SetFunction)(pm,    29, pArgs->pEquations[29] - (0));
	// 0
	(SetFunction)(pm,    30, pArgs->pEquations[30] - (0));
	// 31 relay(-116.0 + pow(3.0 * pow(S2, 2) + pow(S3, 2), 0.5), 0, 0) in host block 
	// 56.0 + S1 + S2
	(SetFunction)(pm,    32, pArgs->pEquations[32] - (56.0 + pArgs->pEquations[4] + pArgs->pEquations[5]));
	// 0
	(SetFunction)(pm,    33, pArgs->pEquations[33] - (0));
	// 0
	(SetFunction)(pm,    34, pArgs->pEquations[34] - (0));
	// 35 relay(56.0 + S1 + S2, 0, 0) in host block 
	// -5.0
	(SetFunction)(pm,    36, pArgs->pEquations[36] - (-5.0));
	// 5
	(SetFunction)(pm,    37, pArgs->pEquations[37] - (5));
	// 38 limit(L2, -5.0, 5) in host block 
	// 0
	(SetFunction)(pm,    39, pArgs->pEquations[39] - (0));
	// 0
	(SetFunction)(pm,    40, pArgs->pEquations[40] - (0));
	// 41 relay(limit(L2, -5.0, 5), 0, 0) in host block 
	// 42 abs(S1) in host block 
	// 3.5
	(SetFunction)(pm,    43, pArgs->pEquations[43] - (3.5));
	// 44 deadband(abs(S1), 3.5) in host block 
	// 0
	(SetFunction)(pm,    45, pArgs->pEquations[45] - (0));
	// 0.1
	(SetFunction)(pm,    46, pArgs->pEquations[46] - (0.1));
	// 47 relay(deadband(abs(S1), 3.5), 0, 0.1) in host block 
	// 0.03
	(SetFunction)(pm,    48, pArgs->pEquations[48] - (0.03));
	// 49 expand(LT5, 0.03) in host block 
	// 0
	(SetFunction)(pm,    50, pArgs->pEquations[50] - (0));
	// 0.02
	(SetFunction)(pm,    51, pArgs->pEquations[51] - (0.02));
	// 52 relay(expand(LT5, 0.03), 0, 0.02) in host block 
	// 0.02
	(SetFunction)(pm,    53, pArgs->pEquations[53] - (0.02));
	// 54 shrink(L6, 0.02) in host block 
	// 0
	(SetFunction)(pm,    55, pArgs->pEquations[55] - (0));
	// 0.02
	(SetFunction)(pm,    56, pArgs->pEquations[56] - (0.02));
	// 57 relay(shrink(L6, 0.02), 0, 0.02) in host block 
	// 0.04
	(SetFunction)(pm,    58, pArgs->pEquations[58] - (0.04));
	// 59 expand(L7, 0.04) in host block 
	// 0
	(SetFunction)(pm,    60, pArgs->pEquations[60] - (0));
	// 0.02
	(SetFunction)(pm,    61, pArgs->pEquations[61] - (0.02));
	// 62 relay(expand(L7, 0.04), 0, 0.02) in host block 
	// 105.0 + LT1
	(SetFunction)(pm,    63, pArgs->pEquations[63] - (105.0 + pArgs->pEquations[31]));

	return GetInternalsCount();
}

__declspec(dllexport) long BuildDerivatives(BuildEquationsArgs* pArgs)
{
	return GetInternalsCount();
}

__declspec(dllexport) long GetBlockParametersValues(long nBlockIndex, BuildEquationsArgs *pArgs, double* pBlockParameters)
{
	switch(nBlockIndex)
	{
	case 6:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0;
		break;

	case 7:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0;
		break;

	case 8:
		pBlockParameters[0] = -5.0;
		pBlockParameters[1] = 5;
		break;

	case 9:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0;
		break;

	case 11:
		pBlockParameters[0] = 3.5;
		break;

	case 12:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0.1;
		break;

	case 13:
		pBlockParameters[0] = 0.03;
		break;

	case 14:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0.02;
		break;

	case 15:
		pBlockParameters[0] = 0.02;
		break;

	case 16:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0.02;
		break;

	case 17:
		pBlockParameters[0] = 0.04;
		break;

	case 18:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0.02;
		break;

	}

	return GetBlockParametersCount(nBlockIndex);
}


__declspec(dllexport) long DeviceInit(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLINITBLOCK InitBlock = pArgs->pFnInitBlock;

	// 15.0 * pArgs->t
	pArgs->pEquations[0] = 15.0 * pArgs->t;
	// sin(15.0 * pArgs->t)
	pArgs->pEquations[1] = sin(pArgs->pEquations[0]);
	// -0.10000000000000001 * pArgs->t
	pArgs->pEquations[2] = -0.10000000000000001 * pArgs->t;
	// exp(-0.10000000000000001 * pArgs->t)
	pArgs->pEquations[3] = exp(pArgs->pEquations[2]);
	// 5.0 * sin(15.0 * pArgs->t) * exp(-0.10000000000000001 * pArgs->t)
	pArgs->pEquations[4] = 5.0 * pArgs->pEquations[1] * pArgs->pEquations[3];
	// pArgs->pEquations[4] + *pArgs->pExternals[0].pValue
	pArgs->pEquations[5] = pArgs->pEquations[4] + *pArgs->pExternals[0].pValue;
	// *pArgs->pExternals[1].pValue
	pArgs->pEquations[6] = *pArgs->pExternals[1].pValue;
	// *pArgs->pExternals[2].pValue
	pArgs->pEquations[7] = *pArgs->pExternals[2].pValue;
	// 10.15
	pArgs->pEquations[8] = 10.15;
	// PBT_HIGHER 0	// pArgs->pEquations[9] = pArgs->pEquations[7]>pArgs->pEquations[8];
	(*InitBlock)(pm,0);
	// pArgs->pSetPoints[0]
	pArgs->pEquations[10] = pArgs->pSetPoints[0];
	// 0.5
	pArgs->pEquations[11] = 0.5;
	// 0.99
	pArgs->pEquations[13] = 0.99;
	// 15.0 * t
	pArgs->pEquations[16] = 15.0 * pArgs->t;
	// sin(15.0 * t)
	pArgs->pEquations[17] = sin(pArgs->pEquations[16]);
	// PBT_HIGHER 1	// pArgs->pEquations[12] = pArgs->pEquations[17]>pArgs->pEquations[11];
	(*InitBlock)(pm,1);
	// 20.0 * t
	pArgs->pEquations[18] = 20.0 * pArgs->t;
	// sin(20.0 * t)
	pArgs->pEquations[19] = sin(pArgs->pEquations[18]);
	// 2.0 * sin(20.0 * t)
	pArgs->pEquations[20] = 2.0 * pArgs->pEquations[19];
	// PBT_HIGHER 2	// pArgs->pEquations[14] = pArgs->pEquations[20]>pArgs->pEquations[13];
	(*InitBlock)(pm,2);
	// PBT_OR 3	// pArgs->pEquations[15] = init_or(pArgs->pEquations[12],pArgs->pEquations[14]);
	(*InitBlock)(pm,3);
	// PBT_HIGHER 4	// pArgs->pEquations[21] = pArgs->pEquations[15]>pArgs->pEquations[20];
	(*InitBlock)(pm,4);
	// PBT_LOWER 5	// pArgs->pEquations[22] = pArgs->pEquations[17]<pArgs->pEquations[20];
	(*InitBlock)(pm,5);
	// pow(pArgs->pEquations[5], 2)
	pArgs->pEquations[23] = pow(pArgs->pEquations[5], 2);
	// 3.0 * pow(pArgs->pEquations[5], 2)
	pArgs->pEquations[24] = 3.0 * pArgs->pEquations[23];
	// pow(pArgs->pEquations[6], 2)
	pArgs->pEquations[25] = pow(pArgs->pEquations[6], 2);
	// 3.0 * pow(pArgs->pEquations[5], 2) + pow(pArgs->pEquations[6], 2)
	pArgs->pEquations[26] = pArgs->pEquations[24] + pArgs->pEquations[25];
	// pow(3.0 * pow(pArgs->pEquations[5], 2) + pow(pArgs->pEquations[6], 2), 0.5)
	pArgs->pEquations[27] = pow(pArgs->pEquations[26], 0.5);
	// -116.0 + pow(3.0 * pow(pArgs->pEquations[5], 2) + pow(pArgs->pEquations[6], 2), 0.5)
	pArgs->pEquations[28] = -116.0 + pArgs->pEquations[27];
	// 0
	pArgs->pEquations[29] = 0;
	// 0
	pArgs->pEquations[30] = 0;
	// PBT_RELAYDELAY 6	// pArgs->pEquations[31] = init_relay(pArgs->pEquations[28], pArgs->pEquations[29], pArgs->pEquations[30]);
	(*InitBlock)(pm,6);
	// 56.0 + pArgs->pEquations[4] + pArgs->pEquations[5]
	pArgs->pEquations[32] = 56.0 + pArgs->pEquations[4] + pArgs->pEquations[5];
	// 0
	pArgs->pEquations[33] = 0;
	// 0
	pArgs->pEquations[34] = 0;
	// PBT_RELAYDELAY 7	// pArgs->pEquations[35] = init_relay(pArgs->pEquations[32], pArgs->pEquations[33], pArgs->pEquations[34]);
	(*InitBlock)(pm,7);
	// -5.0
	pArgs->pEquations[36] = -5.0;
	// 5
	pArgs->pEquations[37] = 5;
	// PBT_LIMITERCONST 8	// pArgs->pEquations[38] = init_limit(pArgs->pEquations[32], pArgs->pEquations[36], pArgs->pEquations[37]);
	(*InitBlock)(pm,8);
	// 0
	pArgs->pEquations[39] = 0;
	// 0
	pArgs->pEquations[40] = 0;
	// PBT_RELAYDELAY 9	// pArgs->pEquations[41] = init_relay(pArgs->pEquations[38], pArgs->pEquations[39], pArgs->pEquations[40]);
	(*InitBlock)(pm,9);
	// PBT_ABS 10	// pArgs->pEquations[42] = init_abs(pArgs->pEquations[4]);
	(*InitBlock)(pm,10);
	// 3.5
	pArgs->pEquations[43] = 3.5;
	// PBT_DEADBAND 11	// pArgs->pEquations[44] = init_deadband(pArgs->pEquations[42], pArgs->pEquations[43]);
	(*InitBlock)(pm,11);
	// 0
	pArgs->pEquations[45] = 0;
	// 0.1
	pArgs->pEquations[46] = 0.1;
	// PBT_RELAYDELAY 12	// pArgs->pEquations[47] = init_relay(pArgs->pEquations[44], pArgs->pEquations[45], pArgs->pEquations[46]);
	(*InitBlock)(pm,12);
	// 0.03
	pArgs->pEquations[48] = 0.03;
	// PBT_EXPAND 13	// pArgs->pEquations[49] = init_expand(pArgs->pEquations[47], pArgs->pEquations[48]);
	(*InitBlock)(pm,13);
	// 0
	pArgs->pEquations[50] = 0;
	// 0.02
	pArgs->pEquations[51] = 0.02;
	// PBT_RELAYDELAY 14	// pArgs->pEquations[52] = init_relay(pArgs->pEquations[49], pArgs->pEquations[50], pArgs->pEquations[51]);
	(*InitBlock)(pm,14);
	// 0.02
	pArgs->pEquations[53] = 0.02;
	// PBT_SHRINK 15	// pArgs->pEquations[54] = init_shrink(pArgs->pEquations[49], pArgs->pEquations[53]);
	(*InitBlock)(pm,15);
	// 0
	pArgs->pEquations[55] = 0;
	// 0.02
	pArgs->pEquations[56] = 0.02;
	// PBT_RELAYDELAY 16	// pArgs->pEquations[57] = init_relay(pArgs->pEquations[54], pArgs->pEquations[55], pArgs->pEquations[56]);
	(*InitBlock)(pm,16);
	// 0.04
	pArgs->pEquations[58] = 0.04;
	// PBT_EXPAND 17	// pArgs->pEquations[59] = init_expand(pArgs->pEquations[54], pArgs->pEquations[58]);
	(*InitBlock)(pm,17);
	// 0
	pArgs->pEquations[60] = 0;
	// 0.02
	pArgs->pEquations[61] = 0.02;
	// PBT_RELAYDELAY 18	// pArgs->pEquations[62] = init_relay(pArgs->pEquations[59], pArgs->pEquations[60], pArgs->pEquations[61]);
	(*InitBlock)(pm,18);
	// 105.0 + pArgs->pEquations[31]
	pArgs->pEquations[63] = 105.0 + pArgs->pEquations[31];

	return GetInternalsCount();
}


__declspec(dllexport) long ProcessDiscontinuity(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLPROCESSBLOCKDISCONTINUITY ProcessBlockDiscontinuity = pArgs->pFnProcessBlockDiscontinuity;

	// 15.0 * pArgs->t
	pArgs->pEquations[0] = 15.0 * pArgs->t;
	// sin(15.0 * pArgs->t)
	pArgs->pEquations[1] = sin(pArgs->pEquations[0]);
	// -0.10000000000000001 * pArgs->t
	pArgs->pEquations[2] = -0.10000000000000001 * pArgs->t;
	// exp(-0.10000000000000001 * pArgs->t)
	pArgs->pEquations[3] = exp(pArgs->pEquations[2]);
	// 5.0 * sin(15.0 * pArgs->t) * exp(-0.10000000000000001 * pArgs->t)
	pArgs->pEquations[4] = 5.0 * pArgs->pEquations[1] * pArgs->pEquations[3];
	// pArgs->pEquations[4] + *pArgs->pExternals[0].pValue
	pArgs->pEquations[5] = pArgs->pEquations[4] + *pArgs->pExternals[0].pValue;
	// *pArgs->pExternals[1].pValue
	pArgs->pEquations[6] = *pArgs->pExternals[1].pValue;
	// *pArgs->pExternals[2].pValue
	pArgs->pEquations[7] = *pArgs->pExternals[2].pValue;
	// 10.15
	pArgs->pEquations[8] = 10.15;
	// PBT_HIGHER 0
	(*ProcessBlockDiscontinuity)(pm,0);
	// pArgs->pSetPoints[0]
	pArgs->pEquations[10] = pArgs->pSetPoints[0];
	// 0.5
	pArgs->pEquations[11] = 0.5;
	// 0.99
	pArgs->pEquations[13] = 0.99;
	// 15.0 * t
	pArgs->pEquations[16] = 15.0 * pArgs->t;
	// sin(15.0 * t)
	pArgs->pEquations[17] = sin(pArgs->pEquations[16]);
	// PBT_HIGHER 1
	(*ProcessBlockDiscontinuity)(pm,1);
	// 20.0 * t
	pArgs->pEquations[18] = 20.0 * pArgs->t;
	// sin(20.0 * t)
	pArgs->pEquations[19] = sin(pArgs->pEquations[18]);
	// 2.0 * sin(20.0 * t)
	pArgs->pEquations[20] = 2.0 * pArgs->pEquations[19];
	// PBT_HIGHER 2
	(*ProcessBlockDiscontinuity)(pm,2);
	// PBT_OR 3
	(*ProcessBlockDiscontinuity)(pm,3);
	// PBT_HIGHER 4
	(*ProcessBlockDiscontinuity)(pm,4);
	// PBT_LOWER 5
	(*ProcessBlockDiscontinuity)(pm,5);
	// pow(pArgs->pEquations[5], 2)
	pArgs->pEquations[23] = pow(pArgs->pEquations[5], 2);
	// 3.0 * pow(pArgs->pEquations[5], 2)
	pArgs->pEquations[24] = 3.0 * pArgs->pEquations[23];
	// pow(pArgs->pEquations[6], 2)
	pArgs->pEquations[25] = pow(pArgs->pEquations[6], 2);
	// 3.0 * pow(pArgs->pEquations[5], 2) + pow(pArgs->pEquations[6], 2)
	pArgs->pEquations[26] = pArgs->pEquations[24] + pArgs->pEquations[25];
	// pow(3.0 * pow(pArgs->pEquations[5], 2) + pow(pArgs->pEquations[6], 2), 0.5)
	pArgs->pEquations[27] = pow(pArgs->pEquations[26], 0.5);
	// -116.0 + pow(3.0 * pow(pArgs->pEquations[5], 2) + pow(pArgs->pEquations[6], 2), 0.5)
	pArgs->pEquations[28] = -116.0 + pArgs->pEquations[27];
	// 0
	pArgs->pEquations[29] = 0;
	// 0
	pArgs->pEquations[30] = 0;
	// PBT_RELAYDELAY 6
	(*ProcessBlockDiscontinuity)(pm,6);
	// 56.0 + pArgs->pEquations[4] + pArgs->pEquations[5]
	pArgs->pEquations[32] = 56.0 + pArgs->pEquations[4] + pArgs->pEquations[5];
	// 0
	pArgs->pEquations[33] = 0;
	// 0
	pArgs->pEquations[34] = 0;
	// PBT_RELAYDELAY 7
	(*ProcessBlockDiscontinuity)(pm,7);
	// -5.0
	pArgs->pEquations[36] = -5.0;
	// 5
	pArgs->pEquations[37] = 5;
	// PBT_LIMITERCONST 8
	(*ProcessBlockDiscontinuity)(pm,8);
	// 0
	pArgs->pEquations[39] = 0;
	// 0
	pArgs->pEquations[40] = 0;
	// PBT_RELAYDELAY 9
	(*ProcessBlockDiscontinuity)(pm,9);
	// PBT_ABS 10
	(*ProcessBlockDiscontinuity)(pm,10);
	// 3.5
	pArgs->pEquations[43] = 3.5;
	// PBT_DEADBAND 11
	(*ProcessBlockDiscontinuity)(pm,11);
	// 0
	pArgs->pEquations[45] = 0;
	// 0.1
	pArgs->pEquations[46] = 0.1;
	// PBT_RELAYDELAY 12
	(*ProcessBlockDiscontinuity)(pm,12);
	// 0.03
	pArgs->pEquations[48] = 0.03;
	// PBT_EXPAND 13
	(*ProcessBlockDiscontinuity)(pm,13);
	// 0
	pArgs->pEquations[50] = 0;
	// 0.02
	pArgs->pEquations[51] = 0.02;
	// PBT_RELAYDELAY 14
	(*ProcessBlockDiscontinuity)(pm,14);
	// 0.02
	pArgs->pEquations[53] = 0.02;
	// PBT_SHRINK 15
	(*ProcessBlockDiscontinuity)(pm,15);
	// 0
	pArgs->pEquations[55] = 0;
	// 0.02
	pArgs->pEquations[56] = 0.02;
	// PBT_RELAYDELAY 16
	(*ProcessBlockDiscontinuity)(pm,16);
	// 0.04
	pArgs->pEquations[58] = 0.04;
	// PBT_EXPAND 17
	(*ProcessBlockDiscontinuity)(pm,17);
	// 0
	pArgs->pEquations[60] = 0;
	// 0.02
	pArgs->pEquations[61] = 0.02;
	// PBT_RELAYDELAY 18
	(*ProcessBlockDiscontinuity)(pm,18);
	// 105.0 + pArgs->pEquations[31]
	pArgs->pEquations[63] = 105.0 + pArgs->pEquations[31];

	return GetInternalsCount();
}

