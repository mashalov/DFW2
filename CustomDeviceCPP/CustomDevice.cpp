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
	m_Primitives = { {PBT_DERLAG, _T("dSu")}, {PBT_DERLAG, _T("dV")}, {PBT_DERLAG, _T("dEq")} };
}

void CCustomDevice::BuildRightHand(CCustomDeviceData& CustomDeviceData)
{
	for(const auto& var : m_Variables)
		(*CustomDeviceData.pFnSetFunction)(CustomDeviceData, var, 0.0);
}

void CCustomDevice::BuildEquations(CCustomDeviceData& CustomDeviceData)
{
	for (const auto& var : m_Variables)
		(*CustomDeviceData.pFnSetElement)(CustomDeviceData, var, var, 1.0);
}

void CCustomDevice::BuildDerivatives(CCustomDeviceData& CustomDeviceData)
{

}

eDEVICEFUNCTIONSTATUS CCustomDevice::Init(CCustomDeviceData& CustomDeviceData)
{
	eDEVICEFUNCTIONSTATUS eRes = (CustomDeviceData.pFnInitPrimitive)(CustomDeviceData, 0);
	return eRes;
}

eDEVICEFUNCTIONSTATUS CCustomDevice::ProcessDiscontinuity(CCustomDeviceData& CustomDeviceData)
{
	eDEVICEFUNCTIONSTATUS eRes = (CustomDeviceData.pFnProcPrimDisco)(CustomDeviceData, 0);
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
	DeviceProps.SetType(DEVTYPE_EXCCON);
	DeviceProps.SetType(DEVTYPE_EXCCON_MUSTANG);
	DeviceProps.SetClassName(_T("ÀÐÂ Ìóñòàíã"), _T("ExciterMustang"));
	DeviceProps.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);
	
	DeviceProps.m_VarMap.insert(std::make_pair(_T("Uf"), CVarIndex(0, VARUNIT_PU)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("Usum"), CVarIndex(1, VARUNIT_PU)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("Svt"), CVarIndex(2, VARUNIT_PU)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("dVdt"), CVarIndex(3, VARUNIT_PU)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("dEqdt"), CVarIndex(4, VARUNIT_PU)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("dSdt"), CVarIndex(5, VARUNIT_PU)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("dVdtLag"), CVarIndex(6, VARUNIT_PU)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("dEqdtLag"), CVarIndex(7, VARUNIT_PU)));
	DeviceProps.m_VarMap.insert(std::make_pair(_T("dSdtLag"), CVarIndex(8, VARUNIT_PU)));

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
