
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
	return 4;
}

__declspec(dllexport) long GetOutputsCount(void)
{
	return 19;
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
	return 5;
}

__declspec(dllexport) long GetInternalsCount(void)
{
	return 22;
}

__declspec(dllexport) long GetBlocksDescriptions(BlockDescriptions *pBlockDescriptions)
{
	SetBlockDescription(pBlockDescriptions +     0, PBT_RELAYDELAYLOGIC ,     0, _T("") );
	SetBlockDescription(pBlockDescriptions +     1, PBT_RELAYDELAYLOGIC ,     1, _T("") );
	SetBlockDescription(pBlockDescriptions +     2, PBT_RELAYDELAYLOGIC ,     2, _T("") );
	SetBlockDescription(pBlockDescriptions +     3, PBT_RELAYDELAYLOGIC ,     3, _T("") );
	SetBlockDescription(pBlockDescriptions +     4, PBT_RELAYDELAYLOGIC ,     4, _T("") );

	return GetBlocksCount();
}

__declspec(dllexport) long GetBlockPinsCount(long nBlockIndex)
{
	const long PinsCount[5] = { 2, 2, 2, 2, 2 };

	if(nBlockIndex >= 0 && nBlockIndex < 5)
		return PinsCount[nBlockIndex];

	return -1;
}

__declspec(dllexport) long GetBlockPinsIndexes(long nBlockIndex, BLOCKPININDEX* pBlockPins)
{
	switch(nBlockIndex)
	{
	case 0:
		SetPinInfo(pBlockPins +     0,     6, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,     3, eVL_INTERNAL);
		break;

	case 1:
		SetPinInfo(pBlockPins +     0,     9, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,     6, eVL_INTERNAL);
		break;

	case 2:
		SetPinInfo(pBlockPins +     0,    13, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    10, eVL_INTERNAL);
		break;

	case 3:
		SetPinInfo(pBlockPins +     0,    16, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    13, eVL_INTERNAL);
		break;

	case 4:
		SetPinInfo(pBlockPins +     0,    20, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,    17, eVL_INTERNAL);
		break;

	}
	return GetBlockPinsCount(nBlockIndex);
}

__declspec(dllexport) long GetBlockParametersCount(long nBlockIndex)
{
	const long ParametersCount[5] = { 2, 2, 2, 2, 2 };

	if(nBlockIndex >= 0 && nBlockIndex < 5)
		return ParametersCount[nBlockIndex];

	return -1;
}


__declspec(dllexport) long GetSetPointsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo +     0,     0, _T("#vetv[3673,3904].Pb"), VARUNIT_NOTSET, 0);

	return GetSetPointsCount();
}

__declspec(dllexport) long GetInputsInfos(InputVarsInfo *pVarsInfo)
{
	SetInputVarsInfo(pVarsInfo +     3, _T("node[1114].bsh"), DEVTYPE_MODEL, _T("") );
	SetInputVarsInfo(pVarsInfo +     1, _T("vetv[3673,3904].Pb"), DEVTYPE_MODEL, _T("") );
	SetInputVarsInfo(pVarsInfo +     0, _T("vetv[3673,3904].Qb"), DEVTYPE_MODEL, _T("") );
	SetInputVarsInfo(pVarsInfo +     2, _T("vetv[3673,3904].Qe"), DEVTYPE_MODEL, _T("") );

	return GetInputsCount();
}

__declspec(dllexport) long GetConstantsInfos(ConstVarsInfo *pVarsInfo)
{

	return GetConstantsCount();
}

__declspec(dllexport) long GetOutputsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo +     0,    21, _T("A3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     1,    22, _T("A4"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     2,    23, _T("A5"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     3,    24, _T("A6"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     4,    25, _T("A7"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     5,    26, _T("A8"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     6,     1, _T("A9"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     7,     3, _T("L1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     8,     6, _T("L2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     9,    10, _T("L3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    10,    13, _T("L4"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    11,    17, _T("L5"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    12,     6, _T("LT1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    13,     9, _T("LT2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    14,    13, _T("LT3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    15,    16, _T("LT4"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    16,    20, _T("LT5"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    17,     0, _T("S1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    18,     2, _T("S2"), VARUNIT_NOTSET, 1);

	return GetOutputsCount();
}

__declspec(dllexport) long GetInternalsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo +     0,     0, _T("S1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     1,     1, _T("A9"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     2,     2, _T("S2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     3,     3, _T("L1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     4,     4, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     5,     5, _T("0.55"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     6,     7, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     7,     8, _T("0.15"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     8,    10, _T("L3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     9,    11, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    10,    12, _T("0.5"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    11,    14, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    12,    15, _T("0.1"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    13,    17, _T("L5"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    14,    18, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    15,    19, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    16,    21, _T("A3"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    17,    22, _T("A4"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    18,    23, _T("A5"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    19,    24, _T("A6"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    20,    25, _T("A7"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +    21,    26, _T("A8"), VARUNIT_NOTSET, 1);

	return GetInternalsCount();
}

__declspec(dllexport) long BuildEquations(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLSETELEMENT SetElement = pArgs->pFnSetElement;

	// #vetv[3673,3904].Pb + vetv[3673,3904].Qb by vetv[3673,3904].Qb
	if(pArgs->pExternals[0].nIndex != DFW2_NON_STATE_INDEX)
	(SetElement)(pm,     0, pArgs->pExternals[0].nIndex             , -(1), 0 );
	// #vetv[3673,3904].Pb + vetv[3673,3904].Qb / #vetv[3673,3904].Pb + vetv[3673,3904].Qb;
	(SetElement)(pm,     0, 0                                       , 1.0, 0);
	// External variable vetv[3673,3904].Qe
	if(pArgs->pExternals[2].nIndex != DFW2_NON_STATE_INDEX)
	(SetElement)(pm,     1, pArgs->pExternals[2].nIndex             , -(1), 0 );
	// vetv[3673,3904].Qe / vetv[3673,3904].Qe;
	(SetElement)(pm,     1, 1                                       , 1.0, 0);
	// 2.0 + vetv[3673,3904].Qe by vetv[3673,3904].Qe
	(SetElement)(pm,     2, 1                                       , -(1), 0 );
	// 2.0 + vetv[3673,3904].Qe / 2.0 + vetv[3673,3904].Qe;
	(SetElement)(pm,     2, 2                                       , 1.0, 0);
	// 1 / 1;
	(SetElement)(pm,     3, 3                                       , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,     4, 4                                       , 1.0, 0);
	// 0.55 / 0.55;
	(SetElement)(pm,     5, 5                                       , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,     7, 7                                       , 1.0, 0);
	// 0.15 / 0.15;
	(SetElement)(pm,     8, 8                                       , 1.0, 0);
	// 1 / 1;
	(SetElement)(pm,    10, 10                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    11, 11                                      , 1.0, 0);
	// 0.5 / 0.5;
	(SetElement)(pm,    12, 12                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    14, 14                                      , 1.0, 0);
	// 0.1 / 0.1;
	(SetElement)(pm,    15, 15                                      , 1.0, 0);
	// External variable node[1114].bsh
	if(pArgs->pExternals[3].nIndex != DFW2_NON_STATE_INDEX)
	(SetElement)(pm,    17, pArgs->pExternals[3].nIndex             , -(1), 0 );
	// node[1114].bsh / node[1114].bsh;
	(SetElement)(pm,    17, 17                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    18, 18                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    19, 19                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    21, 21                                      , 1.0, 0);
	// 1 / 1;
	(SetElement)(pm,    22, 22                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    23, 23                                      , 1.0, 0);
	// 1E-7 / 1E-7;
	(SetElement)(pm,    24, 24                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    25, 25                                      , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    26, 26                                      , 1.0, 0);

	return GetInternalsCount();
}

__declspec(dllexport) long BuildRightHand(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLSETFUNCTION SetFunction = pArgs->pFnSetFunction;

	// #vetv[3673,3904].Pb + vetv[3673,3904].Qb
	(SetFunction)(pm,     0, pArgs->pEquations[0] - (pArgs->pSetPoints[0] + *pArgs->pExternals[0].pValue));
	// vetv[3673,3904].Qe
	(SetFunction)(pm,     1, pArgs->pEquations[1] - (*pArgs->pExternals[2].pValue));
	// 2.0 + vetv[3673,3904].Qe
	(SetFunction)(pm,     2, pArgs->pEquations[2] - (2.0 + pArgs->pEquations[1]));
	// 1
	(SetFunction)(pm,     3, pArgs->pEquations[3] - (1));
	// 0
	(SetFunction)(pm,     4, pArgs->pEquations[4] - (0));
	// 0.55
	(SetFunction)(pm,     5, pArgs->pEquations[5] - (0.55));
	// 6 alrelay(1, 0, 0.55) in host block 
	// 0
	(SetFunction)(pm,     7, pArgs->pEquations[7] - (0));
	// 0.15
	(SetFunction)(pm,     8, pArgs->pEquations[8] - (0.15));
	// 9 alrelay(LT1, 0, 0.15) in host block 
	// 1
	(SetFunction)(pm,    10, pArgs->pEquations[10] - (1));
	// 0
	(SetFunction)(pm,    11, pArgs->pEquations[11] - (0));
	// 0.5
	(SetFunction)(pm,    12, pArgs->pEquations[12] - (0.5));
	// 13 alrelay(1, 0, 0.5) in host block 
	// 0
	(SetFunction)(pm,    14, pArgs->pEquations[14] - (0));
	// 0.1
	(SetFunction)(pm,    15, pArgs->pEquations[15] - (0.1));
	// 16 alrelay(LT3, 0, 0.1) in host block 
	// node[1114].bsh
	(SetFunction)(pm,    17, pArgs->pEquations[17] - (*pArgs->pExternals[3].pValue));
	// 0
	(SetFunction)(pm,    18, pArgs->pEquations[18] - (0));
	// 0
	(SetFunction)(pm,    19, pArgs->pEquations[19] - (0));
	// 20 alrelay(node[1114].bsh, 0, 0) in host block 
	// 0
	(SetFunction)(pm,    21, pArgs->pEquations[21] - (0));
	// 1
	(SetFunction)(pm,    22, pArgs->pEquations[22] - (1));
	// 0
	(SetFunction)(pm,    23, pArgs->pEquations[23] - (0));
	// 1E-7
	(SetFunction)(pm,    24, pArgs->pEquations[24] - (1E-7));
	// 0
	(SetFunction)(pm,    25, pArgs->pEquations[25] - (0));
	// 0
	(SetFunction)(pm,    26, pArgs->pEquations[26] - (0));

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
	case 0:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0.55;
		break;

	case 1:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0.15;
		break;

	case 2:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0.5;
		break;

	case 3:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0.1;
		break;

	case 4:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = 0;
		break;

	}

	return GetBlockParametersCount(nBlockIndex);
}


__declspec(dllexport) long DeviceInit(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLINITBLOCK InitBlock = pArgs->pFnInitBlock;

	pArgs->pSetPoints[0] = *pArgs->pExternals[1].pValue;
	// pArgs->pSetPoints[0] + *pArgs->pExternals[0].pValue
	pArgs->pEquations[0] = pArgs->pSetPoints[0] + *pArgs->pExternals[0].pValue;
	// *pArgs->pExternals[2].pValue
	pArgs->pEquations[1] = *pArgs->pExternals[2].pValue;
	// 2.0 + *pArgs->pExternals[2].pValue
	pArgs->pEquations[2] = 2.0 + pArgs->pEquations[1];
	// 1
	pArgs->pEquations[3] = 1;
	// 0
	pArgs->pEquations[4] = 0;
	// 0.55
	pArgs->pEquations[5] = 0.55;
	// PBT_RELAYDELAYLOGIC 0	// pArgs->pEquations[6] = init_alrelay(pArgs->pEquations[3], pArgs->pEquations[4], pArgs->pEquations[5]);
	(*InitBlock)(pm,0);
	// 0
	pArgs->pEquations[7] = 0;
	// 0.15
	pArgs->pEquations[8] = 0.15;
	// PBT_RELAYDELAYLOGIC 1	// pArgs->pEquations[9] = init_alrelay(pArgs->pEquations[6], pArgs->pEquations[7], pArgs->pEquations[8]);
	(*InitBlock)(pm,1);
	// 1
	pArgs->pEquations[10] = 1;
	// 0
	pArgs->pEquations[11] = 0;
	// 0.5
	pArgs->pEquations[12] = 0.5;
	// PBT_RELAYDELAYLOGIC 2	// pArgs->pEquations[13] = init_alrelay(pArgs->pEquations[10], pArgs->pEquations[11], pArgs->pEquations[12]);
	(*InitBlock)(pm,2);
	// 0
	pArgs->pEquations[14] = 0;
	// 0.1
	pArgs->pEquations[15] = 0.1;
	// PBT_RELAYDELAYLOGIC 3	// pArgs->pEquations[16] = init_alrelay(pArgs->pEquations[13], pArgs->pEquations[14], pArgs->pEquations[15]);
	(*InitBlock)(pm,3);
	// *pArgs->pExternals[3].pValue
	pArgs->pEquations[17] = *pArgs->pExternals[3].pValue;
	// 0
	pArgs->pEquations[18] = 0;
	// 0
	pArgs->pEquations[19] = 0;
	// PBT_RELAYDELAYLOGIC 4	// pArgs->pEquations[20] = init_alrelay(pArgs->pEquations[17], pArgs->pEquations[18], pArgs->pEquations[19]);
	(*InitBlock)(pm,4);
	// 0
	pArgs->pEquations[21] = 0;
	// 1
	pArgs->pEquations[22] = 1;
	// 0
	pArgs->pEquations[23] = 0;
	// 1E-7
	pArgs->pEquations[24] = 1E-7;
	// 0
	pArgs->pEquations[25] = 0;
	// 0
	pArgs->pEquations[26] = 0;

	return GetInternalsCount();
}


__declspec(dllexport) long ProcessDiscontinuity(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLPROCESSBLOCKDISCONTINUITY ProcessBlockDiscontinuity = pArgs->pFnProcessBlockDiscontinuity;

	// pArgs->pSetPoints[0] + *pArgs->pExternals[0].pValue
	pArgs->pEquations[0] = pArgs->pSetPoints[0] + *pArgs->pExternals[0].pValue;
	// *pArgs->pExternals[2].pValue
	pArgs->pEquations[1] = *pArgs->pExternals[2].pValue;
	// 2.0 + *pArgs->pExternals[2].pValue
	pArgs->pEquations[2] = 2.0 + pArgs->pEquations[1];
	// 1
	pArgs->pEquations[3] = 1;
	// 0
	pArgs->pEquations[4] = 0;
	// 0.55
	pArgs->pEquations[5] = 0.55;
	// PBT_RELAYDELAYLOGIC 0
	(*ProcessBlockDiscontinuity)(pm,0);
	// 0
	pArgs->pEquations[7] = 0;
	// 0.15
	pArgs->pEquations[8] = 0.15;
	// PBT_RELAYDELAYLOGIC 1
	(*ProcessBlockDiscontinuity)(pm,1);
	// 1
	pArgs->pEquations[10] = 1;
	// 0
	pArgs->pEquations[11] = 0;
	// 0.5
	pArgs->pEquations[12] = 0.5;
	// PBT_RELAYDELAYLOGIC 2
	(*ProcessBlockDiscontinuity)(pm,2);
	// 0
	pArgs->pEquations[14] = 0;
	// 0.1
	pArgs->pEquations[15] = 0.1;
	// PBT_RELAYDELAYLOGIC 3
	(*ProcessBlockDiscontinuity)(pm,3);
	// *pArgs->pExternals[3].pValue
	pArgs->pEquations[17] = *pArgs->pExternals[3].pValue;
	// 0
	pArgs->pEquations[18] = 0;
	// 0
	pArgs->pEquations[19] = 0;
	// PBT_RELAYDELAYLOGIC 4
	(*ProcessBlockDiscontinuity)(pm,4);
	// 0
	pArgs->pEquations[21] = 0;
	// 1
	pArgs->pEquations[22] = 1;
	// 0
	pArgs->pEquations[23] = 0;
	// 1E-7
	pArgs->pEquations[24] = 1E-7;
	// 0
	pArgs->pEquations[25] = 0;
	// 0
	pArgs->pEquations[26] = 0;

	return GetInternalsCount();
}

