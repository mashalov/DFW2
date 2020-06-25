#include "pch.h"
#include "CustomDevice.h"

using namespace DFW2;

CCustomDevice::CCustomDevice()
{
	CDeviceContainerPropertiesBase props;
	GetDeviceProperties(props);
	m_Consts.resize(props.m_ConstVarMap.size());
	m_Variables.resize(props.m_VarMap.size());
	m_ExtVariables.resize(props.m_ExtVarMap.size());
	m_Primitives = 
	{ 
		{PBT_RELAYDELAYLOGIC, _T("L1")}, 
		{PBT_RELAYDELAYLOGIC, _T("L2")}, 
		{PBT_RELAYDELAYLOGIC, _T("L3")},
		{PBT_RELAYDELAYLOGIC, _T("L4")},
	};
}

void CCustomDevice::BuildRightHand(CCustomDeviceData& CustomDeviceData)
{

	// 1
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[0], m_Variables[0] - 1.0);
	// 0
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[1], m_Variables[1] - 0.0);
	// 0.55
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[2], m_Variables[2] - 0.55);
	// 3 alrelay(1, 0, 0.55) in host block 
	// 0
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[4], m_Variables[4] - 0.0);
	// 0.15
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[5], m_Variables[5] - 0.15);
	// 6 alrelay(LT1, 0, 0.15) in host block 
	// 1
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[7], m_Variables[7] - 1.0);
	// 0
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[8], m_Variables[8] - 1.0);
	// 0.5
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[9], m_Variables[9] - 0.0);
	// 10 alrelay(1, 0, 0.5) in host block 
	// 0
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[11], m_Variables[11] - 0.0);
	// 0.1
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[12], m_Variables[12] - 0.1);
	// 13 alrelay(LT3, 0, 0.1) in host block 
	// 0
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[14], m_Variables[14] - 0.0);
	// 1
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[15], m_Variables[15] - 1.0);
	// 0
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[16], m_Variables[16] - 0.0);
	// 1E-7
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[17], m_Variables[16] - 1E-7);
	// 0
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[18], m_Variables[18] - 0.0);
	// 0
	(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, m_Variables[19], m_Variables[19] - 0.0);
}

void CCustomDevice::BuildEquations(CCustomDeviceData& CustomDeviceData)
{
	//for (const auto& var : m_Variables)
	//	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, var, var, 1.0);
	// 1 / 1;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[0], m_Variables[0], 1.0);
	// 0 / 0;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[1], m_Variables[1], 1.0);
	// 0.55 / 0.55;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[2], m_Variables[2], 1.0);
	// 0 / 0;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[4], m_Variables[4], 1.0);
	// 0.15 / 0.15;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[5], m_Variables[5], 1.0);
	// 1 / 1;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[7], m_Variables[7], 1.0);
	// 0 / 0;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[8], m_Variables[8], 1.0);
	// 0.5 / 0.5;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[9], m_Variables[9], 1.0);
	// 0 / 0;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[10], m_Variables[10], 1.0);
	// 0.1 / 0.1;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[12], m_Variables[12], 1.0);
	// 0 / 0;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[14], m_Variables[14], 1.0);
	// 1 / 1;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[15], m_Variables[15], 1.0);
	// 0 / 0;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[16], m_Variables[16], 1.0);
	// 1E-7 / 1E-7;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[17], m_Variables[17], 1.0);
	// 0 / 0;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[18], m_Variables[18], 1.0);
	// 0 / 0;
	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, m_Variables[19], m_Variables[19], 1.0);
}

void CCustomDevice::BuildDerivatives(CCustomDeviceData& CustomDeviceData)
{

}

eDEVICEFUNCTIONSTATUS CCustomDevice::Init(CCustomDeviceData& CustomDeviceData)
{
	eDEVICEFUNCTIONSTATUS eRes = eDEVICEFUNCTIONSTATUS::DFS_OK;
	// 1
	m_Variables[0] = 1.0;
	// 0
	m_Variables[1] = 0.0;
	// 0.55
	m_Variables[2] = 0.55;
	// PBT_RELAYDELAYLOGIC 0	// pArgs->pEquations[3].Value = init_alrelay(pArgs->pEquations[0].Value, pArgs->pEquations[1].Value, pArgs->pEquations[2].Value);
	(CustomDeviceData.pFnInitPrimitive)(CustomDeviceData, 0);
	// 0
	m_Variables[4] = 0.0;
	// 0.15
	m_Variables[5] = 0.15;
	// PBT_RELAYDELAYLOGIC 1	// pArgs->pEquations[6].Value = init_alrelay(pArgs->pEquations[3].Value, pArgs->pEquations[4].Value, pArgs->pEquations[5].Value);
	(CustomDeviceData.pFnInitPrimitive)(CustomDeviceData, 1);
	// 1
	m_Variables[7] = 1.0;
	// 0
	m_Variables[8] = 0.0;
	// 0.5
	m_Variables[9] = 0.5;
	// PBT_RELAYDELAYLOGIC 2	// pArgs->pEquations[10].Value = init_alrelay(pArgs->pEquations[7].Value, pArgs->pEquations[8].Value, pArgs->pEquations[9].Value);
	(CustomDeviceData.pFnInitPrimitive)(CustomDeviceData, 2);
	// 0
	m_Variables[11] = 0.0;
	// 0.1
	m_Variables[12] = 0.1;
	// PBT_RELAYDELAYLOGIC 3	// pArgs->pEquations[13].Value = init_alrelay(pArgs->pEquations[10].Value, pArgs->pEquations[11].Value, pArgs->pEquations[12].Value);
	(CustomDeviceData.pFnInitPrimitive)(CustomDeviceData, 3);
	// 0
	m_Variables[14] = 0.0;
	// 1
	m_Variables[15] = 1.0;
	// 0
	m_Variables[16] = 0.0;
	// 1E-7
	m_Variables[17] = 1E-7;
	// 0
	m_Variables[18] = 0.0;
	// 0
	m_Variables[19] = 0.0;

	return eRes;
}

eDEVICEFUNCTIONSTATUS CCustomDevice::ProcessDiscontinuity(CCustomDeviceData& CustomDeviceData)
{
	eDEVICEFUNCTIONSTATUS eRes = eDEVICEFUNCTIONSTATUS::DFS_OK;

	// 1
	m_Variables[0] = 1.0;
	// 0
	m_Variables[1] = 0.0;
	// 0.55
	m_Variables[2] = 0.55;
	// PBT_RELAYDELAYLOGIC 0	// pArgs->pEquations[3].Value = init_alrelay(pArgs->pEquations[0].Value, pArgs->pEquations[1].Value, pArgs->pEquations[2].Value);
	(CustomDeviceData.pFnProcPrimDisco)(CustomDeviceData, 0);
	// 0
	m_Variables[4] = 0.0;
	// 0.15
	m_Variables[5] = 0.15;
	// PBT_RELAYDELAYLOGIC 1	// pArgs->pEquations[6].Value = init_alrelay(pArgs->pEquations[3].Value, pArgs->pEquations[4].Value, pArgs->pEquations[5].Value);
	(CustomDeviceData.pFnProcPrimDisco)(CustomDeviceData, 1);
	// 1
	m_Variables[7] = 1.0;
	// 0
	m_Variables[8] = 0.0;
	// 0.5
	m_Variables[9] = 0.5;
	// PBT_RELAYDELAYLOGIC 2	// pArgs->pEquations[10].Value = init_alrelay(pArgs->pEquations[7].Value, pArgs->pEquations[8].Value, pArgs->pEquations[9].Value);
	(CustomDeviceData.pFnProcPrimDisco)(CustomDeviceData, 2);
	// 0
	m_Variables[11] = 0.0;
	// 0.1
	m_Variables[12] = 0.1;
	// PBT_RELAYDELAYLOGIC 3	// pArgs->pEquations[13].Value = init_alrelay(pArgs->pEquations[10].Value, pArgs->pEquations[11].Value, pArgs->pEquations[12].Value);
	(CustomDeviceData.pFnProcPrimDisco)(CustomDeviceData, 3);
	// 0
	m_Variables[14] = 0.0;
	// 1
	m_Variables[15] = 1.0;
	// 0
	m_Variables[16] = 0.0;
	// 1E-7
	m_Variables[17] = 1E-7;
	// 0
	m_Variables[18] = 0.0;
	// 0
	m_Variables[19] = 0.0;

	return eRes;
}

DOUBLEVECTOR& CCustomDevice::GetConstantData()
{
	return m_Consts;
}

void CCustomDevice::SetConstsDefaultValues()
{
	
}

void CCustomDevice::GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps)
{
	DeviceProps.SetType(DEVTYPE_AUTOMATIC);
	DeviceProps.SetClassName(_T("Automatic & scenario"), _T("Automatic"));
	DeviceProps.AddLinkFrom(DEVTYPE_MODEL, DLM_SINGLE, DPD_MASTER);
	
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A3"), CVarIndex(14, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A4"), CVarIndex(15, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A5"), CVarIndex(16, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A6"), CVarIndex(17, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A7"), CVarIndex(18, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A8"), CVarIndex(19, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("L1"), CVarIndex(0, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("L2"), CVarIndex(3, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("L3"), CVarIndex(7, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("L4"), CVarIndex(10, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("LT1"), CVarIndex(3, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("LT2"), CVarIndex(6, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("LT3"), CVarIndex(10, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("LT4"), CVarIndex(13, VARUNIT_NOTSET)));

	DeviceProps.m_VarMap.insert(std::make_pair(_T("L1"), CVarIndex(0, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("0"), CVarIndex(1, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("0.55"), CVarIndex(2, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("0"), CVarIndex(4, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("0.15"), CVarIndex(5, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("L3"), CVarIndex(7, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("0"), CVarIndex(8, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("0.5"), CVarIndex(9, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("0"), CVarIndex(11, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("0.1"), CVarIndex(12, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A3"), CVarIndex(14, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A4"), CVarIndex(15, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A5"), CVarIndex(16, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A6"), CVarIndex(17, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A7"), CVarIndex(18, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("A8"), CVarIndex(19, VARUNIT_NOTSET)));

	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("T1u"),CConstVarIndex(1,eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Ku"), CConstVarIndex(2, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Ku1"), CConstVarIndex(3, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Urv_min"), CConstVarIndex(4, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Urv_max"), CConstVarIndex(5, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Trv"), CConstVarIndex(6, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Tbch"), CConstVarIndex(7, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Kif1"), CConstVarIndex(8, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("T1if"), CConstVarIndex(9, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Kf1"), CConstVarIndex(10, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Kf"), CConstVarIndex(11, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Tf"), CConstVarIndex(12, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("T1f"), CConstVarIndex(13, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Xcomp"), CConstVarIndex(14, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Unom"), CConstVarIndex(15, eDVT_STATE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Eqnom"), CConstVarIndex(16, eDVT_STATE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Eqe"), CConstVarIndex(17, eDVT_STATE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair(_T("Vref"), CConstVarIndex(18, eDVT_INTERNALCONST)));
	DeviceProps.nEquationsCount = DeviceProps.m_VarMap.size();

	DeviceProps.m_ExtVarMap.insert(std::make_pair(_T("S"), CExtVarIndex(0, DEVTYPE_NODE)));
	DeviceProps.m_ExtVarMap.insert(std::make_pair(_T("Eq"), CExtVarIndex(1, DEVTYPE_GEN_1C)));
	DeviceProps.m_ExtVarMap.insert(std::make_pair(_T("P"), CExtVarIndex(2, DEVTYPE_UNKNOWN)));
	DeviceProps.m_ExtVarMap.insert(std::make_pair(_T("V"), CExtVarIndex(3, DEVTYPE_UNKNOWN)));

}

void CCustomDevice::Destroy()
{
    delete this;
}

VARIABLEVECTOR& CCustomDevice::GetVariables()
{
	return m_Variables;
}

PRIMITIVEVECTOR& CCustomDevice::GetPrimitives()
{
	return m_Primitives;
}

EXTVARIABLEVECTOR& CCustomDevice::GetExternalVariables()
{
	return m_ExtVariables;
}

extern "C" __declspec(dllexport) ICustomDevice* __cdecl CustomDeviceFactory()
{
    return new CCustomDevice();
}
