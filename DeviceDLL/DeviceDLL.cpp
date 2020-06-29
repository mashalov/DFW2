// DeviceDLL.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
//#include "DeviceDLLUtilities.h"
#include "DeviceDLL.h"

__declspec(dllexport) long GetTypesCount(void)
{
	return 2;
}

__declspec(dllexport) long GetTypes(long* pTypes)
{
	pTypes[0] = DEVTYPE_EXCCON;
	pTypes[1] = DEVTYPE_EXCCON_MUSTANG;
	return GetTypesCount();
}

__declspec(dllexport) long GetLinksCount(void)
{
	return 1;
}

__declspec(dllexport) long GetLinks(LinkType* pLink)
{

	SetLink(pLink + 0,
			eLINK_FROM,
			DEVTYPE_EXCITER,
			DLM_SINGLE,
			DPD_MASTER,
			_T("ExciterId"));

	return GetLinksCount();
}

__declspec(dllexport) const _TCHAR* GetDeviceTypeName(void)
{
	return _T("АРВ-СД пользовательская модель");
}

__declspec(dllexport) long Destroy(void)
{
	return 1;
}

__declspec(dllexport) long GetBlocksCount(void)
{
	return 6;
}

__declspec(dllexport) long GetBlocksDescriptions(BlockDescriptions *pBlockDescriptions)
{
	// Компилятор строит систему уравнений по дереву разбора (графики или алгебраической записи)

	// Часть блоков (с производными или дискретные) реализуются хостом (вне DLL). Назовем их хост-блоки
	// Такие блоки проще обрабатывать на хосте, так как они реализуются классами и легко поддерживать их
	// экземпляры, кроме то хосту логично обрабатывать изменение состояний и ZeroCrossing

	// DLL сообщает хосту, какие блокие ей нужны (GetBlocksDescriptions)

	// Система уравнений сортируется компилятором так, что хост-блоки оказываются вверху.
	// Внизу - уравнения скомпилированные в DLL и не требующие блоков

	// Уравнения в системе нумеруются с учетом количества уравнений в каждом блоке.
	// Блок может иметь более одного уравнения, поэтому нумерация уравнений сдвигается
	// Компилятор должен знать сколько уравнений в каждом хост-блоке
	// Адресация уравнений должна быть по их номерам внутри девайса
	// Общее количество уравнений - размер вектора переменных для девайса

	// Входы блоков хоста должны быть привязаны к переменным в векторе девайса или внешним переменным через PrimitiveVariables
	// DLL выдает список индексов входов для указанного блока, индекс соответствует номеру уравнения девайса
	// (GetBlockInputsCount/GetBlockInputsIndexes)
	// (подумать как обходится с внешними переменными)

	SetBlockDescription(pBlockDescriptions + 0, PBT_DERLAG,			1, _T("U derivative"));
	SetBlockDescription(pBlockDescriptions + 1, PBT_DERLAG,			2, _T("Su filter"));
	SetBlockDescription(pBlockDescriptions + 2, PBT_DERLAG,			3, _T("Su derivative"));
	SetBlockDescription(pBlockDescriptions + 3, PBT_DERLAG,			4, _T("Ifd derivative"));
	SetBlockDescription(pBlockDescriptions + 4, PBT_LAG,			5, _T("Presum lag"));
	SetBlockDescription(pBlockDescriptions + 5, PBT_LIMITEDLAG,		6, _T("Output lag"));

	return GetBlocksCount();
}

__declspec(dllexport) long GetBlockPinsCount(long nBlockIndex)
{
	// return -1 on error
	return 2;
}

__declspec(dllexport) long GetBlockPinsIndexes(long nBlockIndex, BLOCKPININDEX* pBlockPins)
{
	// return -1 on error

	switch (nBlockIndex)
	{
	case 0:
		SetPinInfo(pBlockPins + 0, 0, eVL_INTERNAL);
		SetPinInfo(pBlockPins + 1, 15, eVL_INTERNAL);
		break;
	case 1:
		SetPinInfo(pBlockPins + 0, 3, eVL_INTERNAL);
		SetPinInfo(pBlockPins + 1, 0, eVL_EXTERNAL); // Su
		break;
	case 2:
		SetPinInfo(pBlockPins + 0, 6, eVL_INTERNAL);
		SetPinInfo(pBlockPins + 1, 0, eVL_EXTERNAL); // Su
		break;
	case 3:
		SetPinInfo(pBlockPins + 0, 9, eVL_INTERNAL);
		SetPinInfo(pBlockPins + 1, 1, eVL_EXTERNAL); // Ifd
		break;
	case 4:
		SetPinInfo(pBlockPins + 0, 12, eVL_INTERNAL);
		SetPinInfo(pBlockPins + 1, 20, eVL_INTERNAL);
		break;
	case 5:
		SetPinInfo(pBlockPins + 0, 13, eVL_INTERNAL);
		SetPinInfo(pBlockPins + 1, 22, eVL_INTERNAL);
		break;
	}

	return GetBlockPinsCount(nBlockIndex);
}

__declspec(dllexport) long GetBlockParametersCount(long nBlockIndex)
{
	const long Counts[6] = { 2, 2, 2, 2, 2, 4 };

	if (nBlockIndex >= 0 && nBlockIndex < 6)
		return Counts[nBlockIndex];

	return 0;
}

__declspec(dllexport) long GetBlockParametersValues(long nBlockIndex, BuildEquationsArgs *pArgs, double* pBlockParameters)
{
	switch (nBlockIndex)
	{
	case 0:
		pBlockParameters[0] = pArgs->pConsts[0]; // Tu1
		pBlockParameters[1] = 1.0;
		break;
	case 1:
		pBlockParameters[0] = pArgs->pConsts[11]; // Tf
		pBlockParameters[1] = pArgs->pConsts[11]; // Tf
		break;
	case 2:
		pBlockParameters[0] = pArgs->pConsts[12]; // T1f
		pBlockParameters[1] = 1.0;
		break;
	case 3:
		pBlockParameters[0] = pArgs->pConsts[8];  // T1if
		pBlockParameters[1] = 1.0;
		break;
	case 4:
		pBlockParameters[0] = pArgs->pConsts[6];  // Tbch
		pBlockParameters[1] = 1.0;
		break;
	case 5:
		pBlockParameters[0] = pArgs->pConsts[5];  // Trv
		pBlockParameters[1] = 1.0;
		pBlockParameters[2] = pArgs->pConsts[3]; // Urv_min
		pBlockParameters[3] = pArgs->pConsts[4]; // Urv_max
		break;
	}
	return GetBlockParametersCount(nBlockIndex);
}


__declspec(dllexport) long GetInputsCount(void)
{
	return 4;
}
__declspec(dllexport) long GetOutputsCount(void)
{
	return 1;
}
__declspec(dllexport) long GetSetPointsCount(void)
{
	return 1;
}

__declspec(dllexport) long GetConstantsCount(void)
{
	return 17;
}
__declspec(dllexport) long GetInternalsCount(void)
{
	return 9;
}


__declspec(dllexport) long GetSetPointsInfos(VarsInfo *pVarsInfo)
{
	_tcscpy_s(pVarsInfo->Name, CUSTOMDEVICEDLL_MAXNAME, _T("Vref"));
	return GetSetPointsCount();
}

__declspec(dllexport) long GetInputsInfos(InputVarsInfo *pVarsInfo)
{
	SetInputVarsInfo(pVarsInfo + 0, _T("S"), DEVTYPE_NODE, _T(""));
	SetInputVarsInfo(pVarsInfo + 1, _T("Eq"), DEVTYPE_GEN_1C, _T(""));
	SetInputVarsInfo(pVarsInfo + 2, _T("P"), DEVTYPE_UNKNOWN, _T("")); // generator current
	SetInputVarsInfo(pVarsInfo + 3, _T("V"), DEVTYPE_UNKNOWN, _T(""));
	return GetInputsCount();
}

__declspec(dllexport) long GetConstantsInfos(ConstVarsInfo *pVarsInfo)
{
	SetConstantVarInfo(pVarsInfo + 0, _T("T1u"),     0.0, 0.0, 0.04,	DEVTYPE_UNKNOWN,	CVF_ZEROTODEFAULT);
	SetConstantVarInfo(pVarsInfo + 1, _T("Ku"),      0.0, 0.0, 50.0,	DEVTYPE_UNKNOWN, 0);
	SetConstantVarInfo(pVarsInfo + 2, _T("Ku1"),     0.0, 0.0, 5.0,		DEVTYPE_UNKNOWN, 0);
	SetConstantVarInfo(pVarsInfo + 3, _T("Urv_min"), 0.0, 0.0, -10.0,	DEVTYPE_UNKNOWN, 0);
	SetConstantVarInfo(pVarsInfo + 4, _T("Urv_max"), 0.0, 0.0, 10.0,	DEVTYPE_UNKNOWN, 0);
	SetConstantVarInfo(pVarsInfo + 5, _T("Trv"),	 0.0, 0.0, 0.04,	DEVTYPE_UNKNOWN,	CVF_ZEROTODEFAULT);
	SetConstantVarInfo(pVarsInfo + 6, _T("Tbch"),	 0.0, 0.0, 0.07,	DEVTYPE_UNKNOWN,	CVF_ZEROTODEFAULT);
	SetConstantVarInfo(pVarsInfo + 7, _T("Kif1"),	 0.0, 0.0, 1.0,		DEVTYPE_UNKNOWN, 0);
	SetConstantVarInfo(pVarsInfo + 8, _T("T1if"),	 0.0, 0.0, 0.04,	DEVTYPE_UNKNOWN, CVF_ZEROTODEFAULT);
	SetConstantVarInfo(pVarsInfo + 9, _T("Kf1"),	 0.0, 0.0, 1.0,		DEVTYPE_UNKNOWN, 0);
	SetConstantVarInfo(pVarsInfo + 10, _T("Kf"),	 0.0, 0.0, 1.0,		DEVTYPE_UNKNOWN, 0);
	SetConstantVarInfo(pVarsInfo + 11, _T("Tf"),	 0.0, 0.0, 0.9,		DEVTYPE_UNKNOWN,	CVF_ZEROTODEFAULT);
	SetConstantVarInfo(pVarsInfo + 12, _T("T1f"),	 0.0, 0.0, 0.04,	DEVTYPE_UNKNOWN,	CVF_ZEROTODEFAULT);
	SetConstantVarInfo(pVarsInfo + 13, _T("Xcomp"),  0.0, 0.0, 0.0,		DEVTYPE_UNKNOWN,	0);
	SetConstantVarInfo(pVarsInfo + 14, _T("Unom"),	 0.0, 0.0, 0.0,		DEVTYPE_GEN_1C,	0);
	SetConstantVarInfo(pVarsInfo + 15, _T("Eqnom"),  0.0, 0.0, 0.0,		DEVTYPE_GEN_1C,	0);
	SetConstantVarInfo(pVarsInfo + 16, _T("Eqe"),    0.0, 0.0, 0.0,		DEVTYPE_GEN_1C,	0);
	return GetConstantsCount();
}

__declspec(dllexport) long GetOutputsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo, 13, _T("Uf"), VARUNIT_PU, true);
	return GetOutputsCount();
}

__declspec(dllexport) long GetInternalsInfos(VarsInfo *pVarsInfo)
{
	SetVarInfo(pVarsInfo + 0, 14, _T("Xcomp"), VARUNIT_PU, true);
	SetVarInfo(pVarsInfo + 1, 15, _T("Sum8"), VARUNIT_PU, true);
	SetVarInfo(pVarsInfo + 2, 16, _T("Ku"), VARUNIT_PU, true);
	SetVarInfo(pVarsInfo + 3, 17, _T("Ku1"), VARUNIT_PU, true);
	SetVarInfo(pVarsInfo + 4, 18, _T("Kf"), VARUNIT_PU, true);
	SetVarInfo(pVarsInfo + 5, 19, _T("Kf1"), VARUNIT_PU, true);
	SetVarInfo(pVarsInfo + 6, 20, _T("Sum13"), VARUNIT_PU, true);
	SetVarInfo(pVarsInfo + 7, 21, _T("Kif1"), VARUNIT_PU, true);
	SetVarInfo(pVarsInfo + 8, 22, _T("Sum15"), VARUNIT_PU, true);
	return GetInternalsCount();
}

__declspec(dllexport) long BuildEquations(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLSETELEMENT SetElement = pArgs->pFnSetElement;
	// dXcomp / dXcomp
	(SetElement)(pm, 14, 14, 1.0, false);
	// dXcomp / dIg
	(SetElement)(pm, 14, pArgs->pExternals[2].nIndex, -pArgs->pConsts[13], false);

	//dSum8 / dSum8
	(SetElement)(pm, 15, 15, 1.0, false);
	//dSum8 / Vg
	(SetElement)(pm, 15, pArgs->pExternals[3].nIndex, 1.0, false);
	//dSum8 / dXcomp
	(SetElement)(pm, 15, 14, -1.0, false);

	//dKu / dKu
	(SetElement)(pm, 16, 16, 1.0, false);
	//dKu / dSum8
	(SetElement)(pm, 16, 15, -pArgs->pConsts[1], false);

	//dKu1 / dKu1
	(SetElement)(pm, 17, 17, 1.0, false);
	//dKu1 / dU derivative
	(SetElement)(pm, 17, 0, -pArgs->pConsts[2], false);

	//dKf / dKf
	(SetElement)(pm, 18, 18, 1.0, false);
	//dKu1 / dSu filter
	(SetElement)(pm, 18, 3, -pArgs->pConsts[10], false);

	//dKf1 / dKf1
	(SetElement)(pm, 19, 19, 1.0, false);
	//dKu1 / dSu derivative
	(SetElement)(pm, 19, 6, -pArgs->pConsts[9], false);

	//dSum13 / dSum13
	(SetElement)(pm, 20, 20, 1.0, false);
	//dSum13 / dKf
	(SetElement)(pm, 20, 18, -1.0, false);
	//dSum13 / dKf1
	(SetElement)(pm, 20, 19, -1.0, false);

	//dKif1 / dKif1
	(SetElement)(pm, 21, 21, 1.0, false);
	//dKif1 / dIfd derivative
	(SetElement)(pm, 21, 9, -pArgs->pConsts[7], false);

	//dSum15 / dSum15
	(SetElement)(pm, 22, 22, 1.0, false);
	//dSum15 / dPresum lag
	(SetElement)(pm, 22, 12, -1.0, false);
	//dSum15 / dKu1
	(SetElement)(pm, 22, 17, -1.0, false);
	//dSum15 / dKu
	(SetElement)(pm, 22, 16, -1.0, false);
	//dSum15 / dKif1
	(SetElement)(pm, 22, 21, 1.0, false);

	return GetInternalsCount();
}
__declspec(dllexport) long BuildRightHand(BuildEquationsArgs *pArgs)
{
	BuildEquationsObjects *pm = &pArgs->BuildObjects;
	MDLSETFUNCTION SetFunction = pArgs->pFnSetFunction;

	// e14 - Ig * Xcomp
	(SetFunction)(pm, 14, pArgs->pEquations[14].Value - pArgs->pConsts[13] * (*pArgs->pExternals[2].pValue));
	// e15 - Vref + Vg - e14
	(SetFunction)(pm, 15, pArgs->pEquations[15].Value - pArgs->pSetPoints[0] + (*pArgs->pExternals[3].pValue) - pArgs->pEquations[14].Value);
	// e16 - Ku * e15
	(SetFunction)(pm, 16, pArgs->pEquations[16].Value - pArgs->pConsts[1] * pArgs->pEquations[15].Value);
	// e17 - Ku1 * e0
	(SetFunction)(pm, 17, pArgs->pEquations[17].Value - pArgs->pConsts[2] * pArgs->pEquations[0].Value);
	// e18 - Kf * e3
	(SetFunction)(pm, 18, pArgs->pEquations[18].Value - pArgs->pConsts[10] * pArgs->pEquations[3].Value);
	// e19 - Kf1 * e6
	(SetFunction)(pm, 19, pArgs->pEquations[19].Value - pArgs->pConsts[9] * pArgs->pEquations[6].Value);
	// e20 - e18 - e19
	(SetFunction)(pm, 20, pArgs->pEquations[20].Value - pArgs->pEquations[18].Value - pArgs->pEquations[19].Value);
	// e21 - Kif1 * e9
	(SetFunction)(pm, 21, pArgs->pEquations[21].Value - pArgs->pConsts[7] * pArgs->pEquations[9].Value);
	// e22 - e12 - e17 - e16 + e21
	(SetFunction)(pm, 22, pArgs->pEquations[22].Value - pArgs->pEquations[12].Value - pArgs->pEquations[17].Value - pArgs->pEquations[16].Value + pArgs->pEquations[21].Value);

	return GetInternalsCount();
}
__declspec(dllexport) long BuildDerivatives(BuildEquationsArgs *pArgs)
{
	return GetInternalsCount();
}

extern "C" __declspec(dllexport) long DeviceInit(BuildEquationsArgs *pArgs)
{
	pArgs->pConsts[1]  *= pArgs->pConsts[15] / pArgs->pConsts[14];
	pArgs->pConsts[2]  *= 0.72 * pArgs->pConsts[15] / pArgs->pConsts[14];
	pArgs->pConsts[7]  *= 0.2;
	pArgs->pConsts[10] *= 1.3 * pArgs->pConsts[15] / 2.0 / M_PI * pArgs->Omega;
	pArgs->pConsts[9]  *= 0.5 * pArgs->pConsts[15] / 2.0 / M_PI * pArgs->Omega;
	pArgs->pConsts[3] = pArgs->pConsts[3] * pArgs->pConsts[15] - pArgs->pConsts[16];
	pArgs->pConsts[4] = pArgs->pConsts[4] * pArgs->pConsts[15] - pArgs->pConsts[16];

	pArgs->pSetPoints[0] = *pArgs->pExternals[3].pValue;

	return GetInternalsCount();
}


extern "C" __declspec(dllexport) long ProcessDiscontinuity(BuildEquationsArgs *pArgs)
{
	pArgs->pEquations[14].Value = pArgs->pConsts[13] * (*pArgs->pExternals[2].pValue);
	pArgs->pEquations[15].Value = pArgs->pSetPoints[0] - (*pArgs->pExternals[3].pValue) + pArgs->pEquations[14].Value;
	pArgs->pEquations[16].Value = pArgs->pConsts[1] * pArgs->pEquations[15].Value;
	pArgs->pEquations[17].Value = pArgs->pConsts[2] * pArgs->pEquations[0].Value;
	pArgs->pEquations[18].Value = pArgs->pConsts[10] * pArgs->pEquations[3].Value;
	pArgs->pEquations[19].Value = pArgs->pConsts[9] * pArgs->pEquations[6].Value;
	pArgs->pEquations[20].Value = pArgs->pEquations[18].Value + pArgs->pEquations[19].Value;
	pArgs->pEquations[21].Value = pArgs->pConsts[7] * pArgs->pEquations[9].Value;
	pArgs->pEquations[22].Value = pArgs->pEquations[12].Value + pArgs->pEquations[17].Value + pArgs->pEquations[16].Value - pArgs->pEquations[21].Value;
	return GetInternalsCount();
}
