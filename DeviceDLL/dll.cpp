
#include "stdafx.h"
#include "DeviceDLL.h"


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
	return 8;
}

__declspec(dllexport) long GetSetPointsCount(void)
{
	return 0;
}

__declspec(dllexport) long GetConstantsCount(void)
{
	return 0;
}

__declspec(dllexport) long GetBlocksCount(void)
{
	return 2;
}

__declspec(dllexport) long GetInternalsCount(void)
{
	return 15;
}

__declspec(dllexport) long GetBlocksDescriptions(BlockDescriptions *pBlockDescriptions)
{

	SetBlockDescription(pBlockDescriptions +     0, PBT_RELAY           ,     0, _T("") );
	SetBlockDescription(pBlockDescriptions +     1, PBT_RELAY           ,     1, _T("") );

	return GetBlocksCount();
}

__declspec(dllexport) long GetBlockPinsCount(long nBlockIndex)
{
	const long PinsCount[2] = { 2, 2 };

	if(nBlockIndex >= 0 && nBlockIndex < 2)
		return PinsCount[nBlockIndex];

	return -1;
}

__declspec(dllexport) long GetBlockPinsIndexes(long nBlockIndex, BLOCKPININDEX* pBlockPins)
{
	switch(nBlockIndex)
	{
	case 0:
		SetPinInfo(pBlockPins +     0,     8, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,     5, eVL_INTERNAL);
		break;

	case 1:
		SetPinInfo(pBlockPins +     0,    15, eVL_INTERNAL);
		SetPinInfo(pBlockPins +     1,     9, eVL_INTERNAL);
		break;

	}
	return GetBlockPinsCount(nBlockIndex);
}

__declspec(dllexport) long GetBlockParametersCount(long nBlockIndex)
{
	const long ParametersCount[2] = { 2, 2 };

	if(nBlockIndex >= 0 && nBlockIndex < 2)
		return ParametersCount[nBlockIndex];

	return -1;
}


__declspec(dllexport) long GetSetPointsInfos(VarsInfo *pVarsInfo)
{

	return GetSetPointsCount();
}

__declspec(dllexport) long GetInputsInfos(InputVarsInfo *pVarsInfo)
{
	SetInputVarsInfo(pVarsInfo +     0, _T("node[1018].V"), DEVTYPE_MODEL, _T("") );
	SetInputVarsInfo(pVarsInfo +     1, _T("vetv[1111, 1646,1].pl_iq"), DEVTYPE_MODEL, _T("") );
	SetInputVarsInfo(pVarsInfo +     2, _T("vetv[1111,1646, 1].pl_ip"), DEVTYPE_MODEL, _T("") );

	return GetInputsCount();
}

__declspec(dllexport) long GetConstantsInfos(ConstVarsInfo *pVarsInfo)
{

	return GetConstantsCount();
}

__declspec(dllexport) long GetOutputsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo +     0,    16, _T("A1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     1,     5, _T("L1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     2,     9, _T("L2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     3,     8, _T("LT1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     4,    15, _T("LT2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     5,     0, _T("S1"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     6,     1, _T("S2"), VARUNIT_NOTSET, 1);
	SetVarInfo(pVarsInfo +     7,     2, _T("S3"), VARUNIT_NOTSET, 1);

	return GetOutputsCount();
}

__declspec(dllexport) long GetInternalsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo +     0,     0, _T("pow(S1, -1)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     1,     1, _T("pow(S2, 2)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     2,     2, _T("pow(S3, 2)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     3,     3, _T("pow(S2, 2) + pow(S3, 2)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     4,     4, _T("pow(pow(S2, 2) + pow(S3, 2), 0.5)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     5,     5, _T("1732.0508075688772 * pow(S1, -1) * pow(pow(S2, 2) + pow(S3, 2), 0.5)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     6,     6, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     7,     7, _T("0.80000000000000004"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     8,    10, _T("0"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +     9,    11, _T("S1 + S2"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    10,    12, _T("-1 * S2"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    11,    13, _T("S1 + -1 * S2"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    12,    14, _T("(S1 + S2) * (S1 + -1 * S2)"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    13,    16, _T("105 + LT1"), VARUNIT_NOTSET, 0);
	SetVarInfo(pVarsInfo +    14,     9, _T("13 + A1"), VARUNIT_NOTSET, 0);

	return GetInternalsCount();
}

__declspec(dllexport) long BuildEquations(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLSETELEMENT SetElement = pArgs->pFnSetElement;

	// pow(S1, -1) / S1;
	(SetElement)(pm,     0, pArgs->pExternals[0].nIndex             , -(-1 * pow(*pArgs->pExternals[0].pValue, -2)), 0 );
	// pow(S1, -1) / pow(S1, -1);
	(SetElement)(pm,     0, 0                                       , 1.0, 0);
	// pow(S2, 2) / S2;
	(SetElement)(pm,     1, pArgs->pExternals[1].nIndex             , -(2 * *pArgs->pExternals[1].pValue), 0 );
	// pow(S2, 2) / pow(S2, 2);
	(SetElement)(pm,     1, 1                                       , 1.0, 0);
	// pow(S3, 2) / S3;
	(SetElement)(pm,     2, pArgs->pExternals[2].nIndex             , -(2 * *pArgs->pExternals[2].pValue), 0 );
	// pow(S3, 2) / pow(S3, 2);
	(SetElement)(pm,     2, 2                                       , 1.0, 0);
	// pow(S2, 2) + pow(S3, 2) / pow(S2, 2);
	(SetElement)(pm,     3, 1                                       , -(1), 0 );
	// pow(S2, 2) + pow(S3, 2) / pow(S3, 2);
	(SetElement)(pm,     3, 2                                       , -(1), 0 );
	// pow(S2, 2) + pow(S3, 2) / pow(S2, 2) + pow(S3, 2);
	(SetElement)(pm,     3, 3                                       , 1.0, 0);
	// pow(pow(S2, 2) + pow(S3, 2), 0.5) / pow(S2, 2) + pow(S3, 2);
	(SetElement)(pm,     4, 3                                       , -(0.5 * pow(pArgs->pEquations[3], -0.5)), 0 );
	// pow(pow(S2, 2) + pow(S3, 2), 0.5) / pow(pow(S2, 2) + pow(S3, 2), 0.5);
	(SetElement)(pm,     4, 4                                       , 1.0, 0);
	// 1732.0508075688772 * pow(S1, -1) * pow(pow(S2, 2) + pow(S3, 2), 0.5) / pow(S1, -1);
	(SetElement)(pm,     5, 0                                       , -(pArgs->pEquations[4] * 1732.0508075688772), 0 );
	// 1732.0508075688772 * pow(S1, -1) * pow(pow(S2, 2) + pow(S3, 2), 0.5) / pow(pow(S2, 2) + pow(S3, 2), 0.5);
	(SetElement)(pm,     5, 4                                       , -(pArgs->pEquations[0] * 1732.0508075688772), 0 );
	// 1732.0508075688772 * pow(S1, -1) * pow(pow(S2, 2) + pow(S3, 2), 0.5) / 1732.0508075688772 * pow(S1, -1) * pow(pow(S2, 2) + pow(S3, 2), 0.5);
	(SetElement)(pm,     5, 5                                       , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,     6, 6                                       , 1.0, 0);
	// 0.80000000000000004 / 0.80000000000000004;
	(SetElement)(pm,     7, 7                                       , 1.0, 0);
	// 0 / 0;
	(SetElement)(pm,    10, 10                                      , 1.0, 0);
	// S1 + S2 / S1;
	(SetElement)(pm,    11, pArgs->pExternals[0].nIndex             , -(1), 0 );
	// S1 + S2 / S2;
	(SetElement)(pm,    11, pArgs->pExternals[1].nIndex             , -(1), 0 );
	// S1 + S2 / S1 + S2;
	(SetElement)(pm,    11, 11                                      , 1.0, 0);
	// -1 * S2 / S2;
	(SetElement)(pm,    12, pArgs->pExternals[1].nIndex             , -(-1), 0 );
	// -1 * S2 / -1 * S2;
	(SetElement)(pm,    12, 12                                      , 1.0, 0);
	// S1 + -1 * S2 / S1;
	(SetElement)(pm,    13, pArgs->pExternals[0].nIndex             , -(1), 0 );
	// S1 + -1 * S2 / -1 * S2;
	(SetElement)(pm,    13, 12                                      , -(1), 0 );
	// S1 + -1 * S2 / S1 + -1 * S2;
	(SetElement)(pm,    13, 13                                      , 1.0, 0);
	// (S1 + S2) * (S1 + -1 * S2) / S1 + S2;
	(SetElement)(pm,    14, 11                                      , -(pArgs->pEquations[13]), 0 );
	// (S1 + S2) * (S1 + -1 * S2) / S1 + -1 * S2;
	(SetElement)(pm,    14, 13                                      , -(pArgs->pEquations[11]), 0 );
	// (S1 + S2) * (S1 + -1 * S2) / (S1 + S2) * (S1 + -1 * S2);
	(SetElement)(pm,    14, 14                                      , 1.0, 0);
	// 105 + LT1 / LT1;
	(SetElement)(pm,    16, 8                                       , -(1), 0 );
	// 105 + LT1 / 105 + LT1;
	(SetElement)(pm,    16, 16                                      , 1.0, 0);
	// 13 + A1 / A1;
	(SetElement)(pm,     9, 16                                      , -(1), 0 );
	// 13 + A1 / 13 + A1;
	(SetElement)(pm,     9, 9                                       , 1.0, 0);

	return GetInternalsCount();
}

__declspec(dllexport) long BuildRightHand(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLSETFUNCTION SetFunction = pArgs->pFnSetFunction;

	// pow(S1, -1)
	(SetFunction)(pm,     0, pArgs->pEquations[0] - (pow(*pArgs->pExternals[0].pValue, -1)));
	// pow(S2, 2)
	(SetFunction)(pm,     1, pArgs->pEquations[1] - (pow(*pArgs->pExternals[1].pValue, 2)));
	// pow(S3, 2)
	(SetFunction)(pm,     2, pArgs->pEquations[2] - (pow(*pArgs->pExternals[2].pValue, 2)));
	// pow(S2, 2) + pow(S3, 2)
	(SetFunction)(pm,     3, pArgs->pEquations[3] - (pArgs->pEquations[1] + pArgs->pEquations[2]));
	// pow(pow(S2, 2) + pow(S3, 2), 0.5)
	(SetFunction)(pm,     4, pArgs->pEquations[4] - (pow(pArgs->pEquations[3], 0.5)));
	// 1732.0508075688772 * pow(S1, -1) * pow(pow(S2, 2) + pow(S3, 2), 0.5)
	(SetFunction)(pm,     5, pArgs->pEquations[5] - (1732.0508075688772 * pArgs->pEquations[0] * pArgs->pEquations[4]));
	// 0
	(SetFunction)(pm,     6, pArgs->pEquations[6] - (0));
	// 0.80000000000000004
	(SetFunction)(pm,     7, pArgs->pEquations[7] - (0.80000000000000004));
	// 8 relay(1732.0508075688772 * pow(S1, -1) * pow(pow(S2, 2) + pow(S3, 2), 0.5), 0, 0.80000000000000004) in host block 
	// 0
	(SetFunction)(pm,    10, pArgs->pEquations[10] - (0));
	// S1 + S2
	(SetFunction)(pm,    11, pArgs->pEquations[11] - (*pArgs->pExternals[0].pValue + *pArgs->pExternals[1].pValue));
	// -1 * S2
	(SetFunction)(pm,    12, pArgs->pEquations[12] - (-1 * *pArgs->pExternals[1].pValue));
	// S1 + -1 * S2
	(SetFunction)(pm,    13, pArgs->pEquations[13] - (*pArgs->pExternals[0].pValue + pArgs->pEquations[12]));
	// (S1 + S2) * (S1 + -1 * S2)
	(SetFunction)(pm,    14, pArgs->pEquations[14] - (pArgs->pEquations[11] * pArgs->pEquations[13]));
	// 105 + LT1
	(SetFunction)(pm,    16, pArgs->pEquations[16] - (105 + pArgs->pEquations[8]));
	// 13 + A1
	(SetFunction)(pm,     9, pArgs->pEquations[9] - (13 + pArgs->pEquations[16]));
	// 15 relay(13 + A1, 0, (S1 + S2) * (S1 + -1 * S2)) in host block 

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
		pBlockParameters[1] = 0.80000000000000004;
		break;

	case 1:
		pBlockParameters[0] = 0;
		pBlockParameters[1] = (*pArgs->pExternals[0].pValue + *pArgs->pExternals[1].pValue) * (*pArgs->pExternals[0].pValue + -1 * *pArgs->pExternals[1].pValue);
		break;

	}

	return GetBlockParametersCount(nBlockIndex);
}


__declspec(dllexport) long DeviceInit(BuildEquationsArgs *pArgs)
{
	// pow(*pArgs->pExternals[0].pValue, -1)
	pArgs->pEquations[0] = pow(*pArgs->pExternals[0].pValue, -1);
	// pow(*pArgs->pExternals[1].pValue, 2)
	pArgs->pEquations[1] = pow(*pArgs->pExternals[1].pValue, 2);
	// pow(*pArgs->pExternals[2].pValue, 2)
	pArgs->pEquations[2] = pow(*pArgs->pExternals[2].pValue, 2);
	// pow(*pArgs->pExternals[1].pValue, 2) + pow(*pArgs->pExternals[2].pValue, 2)
	pArgs->pEquations[3] = pArgs->pEquations[1] + pArgs->pEquations[2];
	// pow(pow(*pArgs->pExternals[1].pValue, 2) + pow(*pArgs->pExternals[2].pValue, 2), 0.5)
	pArgs->pEquations[4] = pow(pArgs->pEquations[3], 0.5);
	// 1732.0508075688772 * pow(*pArgs->pExternals[0].pValue, -1) * pow(pow(*pArgs->pExternals[1].pValue, 2) + pow(*pArgs->pExternals[2].pValue, 2), 0.5)
	pArgs->pEquations[5] = 1732.0508075688772 * pArgs->pEquations[0] * pArgs->pEquations[4];
	// 0
	pArgs->pEquations[6] = 0;
	// 0.80000000000000004
	pArgs->pEquations[7] = 0.80000000000000004;
	// relay(1732.0508075688772 * pow(*pArgs->pExternals[0].pValue, -1) * pow(pow(*pArgs->pExternals[1].pValue, 2) + pow(*pArgs->pExternals[2].pValue, 2), 0.5), 0, 0.80000000000000004)
	pArgs->pEquations[8] = init_relay(pArgs->pEquations[5], pArgs->pEquations[6], pArgs->pEquations[7]);
	// 0
	pArgs->pEquations[10] = 0;
	// *pArgs->pExternals[0].pValue + *pArgs->pExternals[1].pValue
	pArgs->pEquations[11] = *pArgs->pExternals[0].pValue + *pArgs->pExternals[1].pValue;
	// -1 * *pArgs->pExternals[1].pValue
	pArgs->pEquations[12] = -1 * *pArgs->pExternals[1].pValue;
	// *pArgs->pExternals[0].pValue + -1 * *pArgs->pExternals[1].pValue
	pArgs->pEquations[13] = *pArgs->pExternals[0].pValue + pArgs->pEquations[12];
	// (*pArgs->pExternals[0].pValue + *pArgs->pExternals[1].pValue) * (*pArgs->pExternals[0].pValue + -1 * *pArgs->pExternals[1].pValue)
	pArgs->pEquations[14] = pArgs->pEquations[11] * pArgs->pEquations[13];
	// 105 + LT1
	pArgs->pEquations[16] = 105 + pArgs->pEquations[8];
	// 13 + pArgs->pEquations[16]
	pArgs->pEquations[9] = 13 + pArgs->pEquations[16];
	// relay(13 + pArgs->pEquations[16], 0, (*pArgs->pExternals[0].pValue + *pArgs->pExternals[1].pValue) * (*pArgs->pExternals[0].pValue + -1 * *pArgs->pExternals[1].pValue))
	pArgs->pEquations[15] = init_relay(pArgs->pEquations[9], pArgs->pEquations[10], pArgs->pEquations[14]);

	return GetInternalsCount();
}


__declspec(dllexport) long ProcessDiscontinuity(BuildEquationsArgs* pArgs)
{
	return GetInternalsCount();
}
