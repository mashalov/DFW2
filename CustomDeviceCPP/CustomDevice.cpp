#include "pch.h"
#include "CustomDevice.h"

using namespace DFW2;

CCustomDevice::CCustomDevice()
{
	CDeviceContainerPropertiesBase props;
	GetDeviceProperties(props);
	m_Consts.resize(props.m_ConstVarMap.size());
	m_Variables.resize(props.nEquationsCount);
	m_StateVariablesRefs = VariableIndexRefVec(m_Variables.begin(), m_Variables.end());
	m_ExtVariables.resize(props.m_ExtVarMap.size());
	m_ExternalVariablesRefs = VariableIndexExternalRefVec(m_ExtVariables.begin(), m_ExtVariables.end());
	m_Primitives =
	{
		{PBT_RELAYDELAYLOGIC, "L1", { {0, m_Variables[16] } }, { {0, m_Variables[0]  } } },
		{PBT_RELAYDELAYLOGIC, "L2", { {0, m_Variables[17] } }, { {0, m_Variables[16] } } },
		{PBT_RELAYDELAYLOGIC, "L3", { {0, m_Variables[18] } }, { {0, m_Variables[5]  } } },
		{PBT_RELAYDELAYLOGIC, "L4", { {0, m_Variables[19] } }, { {0, m_Variables[18] } } }
	};
}

void CCustomDevice::BuildRightHand(CCustomDeviceData& CustomDeviceData)
{
	// 1
	CustomDeviceData.SetFunction(m_Variables[0], m_Variables[0] - 1.0);
	// 0
	CustomDeviceData.SetFunction(m_Variables[1], m_Variables[1] - 0.0);
	// 0.55
	CustomDeviceData.SetFunction(m_Variables[2], m_Variables[2] - 0.55);
	// 0
	CustomDeviceData.SetFunction(m_Variables[3], m_Variables[3] - 0.0);
	// 0.15
	CustomDeviceData.SetFunction(m_Variables[4], m_Variables[4] - 0.15);
	// 1
	CustomDeviceData.SetFunction(m_Variables[5], m_Variables[5] - 1.0);
	// 0
	CustomDeviceData.SetFunction(m_Variables[6], m_Variables[6] - 0.0);
	// 0.5
	CustomDeviceData.SetFunction(m_Variables[7], m_Variables[7] - 0.5);
	// 0
	CustomDeviceData.SetFunction(m_Variables[8], m_Variables[8] - 0.0);
	// 0.1
	CustomDeviceData.SetFunction(m_Variables[9], m_Variables[9] - 0.1);
	// 0
	CustomDeviceData.SetFunction(m_Variables[10], m_Variables[10] - 0.0);
	// 1
	CustomDeviceData.SetFunction(m_Variables[11], m_Variables[11] - 1.0);
	// 0
	CustomDeviceData.SetFunction(m_Variables[12], m_Variables[12] - 0.0);
	// 1E-7
	CustomDeviceData.SetFunction(m_Variables[13], m_Variables[13] - 1E-7);
	// 0
	CustomDeviceData.SetFunction(m_Variables[14], m_Variables[14] - 0.0);
	// 0
	CustomDeviceData.SetFunction(m_Variables[15], m_Variables[15] - 0.0);
}

void CCustomDevice::BuildEquations(CCustomDeviceData& CustomDeviceData)
{
	//for (const auto& var : m_Variables)
	//	(*CustomDeviceData.pFnSetElement)(CustomDeviceData, var, var, 1.0);
	// 1 / 1;
	CustomDeviceData.SetElement(m_Variables[0], m_Variables[0], 1.0);
	// 0 / 0;
	CustomDeviceData.SetElement(m_Variables[1], m_Variables[1], 1.0);
	// 0.55 / 0.55;
	CustomDeviceData.SetElement(m_Variables[2], m_Variables[2], 1.0);
	// 0 / 0;
	CustomDeviceData.SetElement(m_Variables[3], m_Variables[3], 1.0);
	// 0.15 / 0.15;
	CustomDeviceData.SetElement(m_Variables[4], m_Variables[4], 1.0);
	// 1 / 1;
	CustomDeviceData.SetElement(m_Variables[5], m_Variables[5], 1.0);
	// 0 / 0;
	CustomDeviceData.SetElement(m_Variables[6], m_Variables[6], 1.0);
	// 0.5 / 0.5;
	CustomDeviceData.SetElement(m_Variables[7], m_Variables[7], 1.0);
	// 0 / 0;
	CustomDeviceData.SetElement(m_Variables[8], m_Variables[8], 1.0);
	// 0.1 / 0.1;
	CustomDeviceData.SetElement(m_Variables[9], m_Variables[9], 1.0);
	// 0 / 0;
	CustomDeviceData.SetElement(m_Variables[10], m_Variables[10], 1.0);
	// 1 / 1;
	CustomDeviceData.SetElement(m_Variables[11], m_Variables[11], 1.0);
	// 0 / 0;
	CustomDeviceData.SetElement(m_Variables[12], m_Variables[12], 1.0);
	// 1E-7 / 1E-7;
	CustomDeviceData.SetElement(m_Variables[13], m_Variables[13], 1.0);
	// 0 / 0;
	CustomDeviceData.SetElement(m_Variables[14], m_Variables[14], 1.0);
	// 0 / 0;
	CustomDeviceData.SetElement(m_Variables[15], m_Variables[15], 1.0);
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
	CustomDeviceData.InitPrimitive(0);
	// 0
	m_Variables[3] = 0.0;
	// 0.15
	m_Variables[4] = 0.15;
	// PBT_RELAYDELAYLOGIC 1	// pArgs->pEquations[6].Value = init_alrelay(pArgs->pEquations[3].Value, pArgs->pEquations[4].Value, pArgs->pEquations[5].Value);
	CustomDeviceData.InitPrimitive(1);
	// 1
	m_Variables[5] = 1.0;
	// 0
	m_Variables[6] = 0.0;
	// 0.5
	m_Variables[7] = 0.5;
	// PBT_RELAYDELAYLOGIC 2	// pArgs->pEquations[10].Value = init_alrelay(pArgs->pEquations[7].Value, pArgs->pEquations[8].Value, pArgs->pEquations[9].Value);
	CustomDeviceData.InitPrimitive(2);
	// 0
	m_Variables[8] = 0.0;
	// 0.1
	m_Variables[9] = 0.1;
	// PBT_RELAYDELAYLOGIC 3	// pArgs->pEquations[13].Value = init_alrelay(pArgs->pEquations[10].Value, pArgs->pEquations[11].Value, pArgs->pEquations[12].Value);
	CustomDeviceData.InitPrimitive(3);
	// 0
	m_Variables[10] = 0.0;
	// 1
	m_Variables[11] = 1.0;
	// 0
	m_Variables[12] = 0.0;
	// 1E-7
	m_Variables[13] = 1E-7;
	// 0
	m_Variables[14] = 0.0;
	// 0
	m_Variables[15] = 0.0;

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
	CustomDeviceData.ProcessPrimitiveDisco(0);
	// 0
	m_Variables[3] = 0.0;
	// 0.15
	m_Variables[4] = 0.15;
	// PBT_RELAYDELAYLOGIC 1	// pArgs->pEquations[6].Value = init_alrelay(pArgs->pEquations[3].Value, pArgs->pEquations[4].Value, pArgs->pEquations[5].Value);
	CustomDeviceData.ProcessPrimitiveDisco(1);
	// 1
	m_Variables[5] = 1.0;
	// 0
	m_Variables[6] = 0.0;
	// 0.5
	m_Variables[7] = 0.5;
	// PBT_RELAYDELAYLOGIC 2	// pArgs->pEquations[10].Value = init_alrelay(pArgs->pEquations[7].Value, pArgs->pEquations[8].Value, pArgs->pEquations[9].Value);
	CustomDeviceData.ProcessPrimitiveDisco(2);
	// 0
	m_Variables[8] = 0.0;
	// 0.1
	m_Variables[9] = 0.1;
	// PBT_RELAYDELAYLOGIC 3	// pArgs->pEquations[13].Value = init_alrelay(pArgs->pEquations[10].Value, pArgs->pEquations[11].Value, pArgs->pEquations[12].Value);
	CustomDeviceData.ProcessPrimitiveDisco(3);
	// 0
	m_Variables[10] = 0.0;
	// 1
	m_Variables[11] = 1.0;
	// 0
	m_Variables[12] = 0.0;
	// 1E-7
	m_Variables[13] = 1E-7;
	// 0
	m_Variables[14] = 0.0;
	// 0
	m_Variables[15] = 0.0;

	return eRes;
}

const DOUBLEVECTOR& CCustomDevice::GetBlockParameters(ptrdiff_t nBlockIndex)
{
	m_BlockParameters.clear();
	switch (nBlockIndex)
	{
	case 0:
		m_BlockParameters = DOUBLEVECTOR{ 0.0, 0.55 };
		break;
	case 1:
		m_BlockParameters = DOUBLEVECTOR{ 0.0, 0.15 };
		break;
	case 2:
		m_BlockParameters = DOUBLEVECTOR{ 0.0, 0.5 };
		break;
	case 3:
		m_BlockParameters = DOUBLEVECTOR{ 0.0, 0.1 };
		break;
	}
	return m_BlockParameters;
}

bool CCustomDevice::SetSourceConstant(size_t nIndex, double Value)
{
	if (nIndex < m_Consts.size())
	{
		m_Consts[nIndex] = Value;
		return true;
	}
	return false;
}

void CCustomDevice::SetConstsDefaultValues()
{

}

void CCustomDevice::GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps)
{
	DeviceProps.SetType(DEVTYPE_AUTOMATIC);
	DeviceProps.SetClassName("Automatic & scenario", "Automatic");
	DeviceProps.AddLinkFrom(DEVTYPE_MODEL, DLM_SINGLE, DPD_MASTER);
	DeviceProps.m_VarMap.insert(std::make_pair("V0", CVarIndex(0, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V1", CVarIndex(1, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V2", CVarIndex(2, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V3", CVarIndex(3, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V4", CVarIndex(4, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V5", CVarIndex(5, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V6", CVarIndex(6, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V7", CVarIndex(7, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V8", CVarIndex(8, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("V9", CVarIndex(9, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("A3", CVarIndex(10, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("A4", CVarIndex(11, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("A5", CVarIndex(12, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("A6", CVarIndex(13, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("A7", CVarIndex(14, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("A8", CVarIndex(15, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("LT1", CVarIndex(16, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("LT2", CVarIndex(17, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("LT3", CVarIndex(18, VARUNIT_NOTSET)));
	DeviceProps.m_VarMap.insert(std::make_pair("LT4", CVarIndex(19, VARUNIT_NOTSET)));
	DeviceProps.nEquationsCount = DeviceProps.m_VarMap.size();



	/*
	DeviceProps.m_ConstVarMap.insert(std::make_pair("T1u",CConstVarIndex(1,eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Ku", CConstVarIndex(2, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Ku1", CConstVarIndex(3, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Urv_min", CConstVarIndex(4, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Urv_max", CConstVarIndex(5, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Trv", CConstVarIndex(6, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Tbch", CConstVarIndex(7, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Kif1", CConstVarIndex(8, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("T1if", CConstVarIndex(9, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Kf1", CConstVarIndex(10, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Kf", CConstVarIndex(11, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Tf", CConstVarIndex(12, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("T1f", CConstVarIndex(13, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Xcomp", CConstVarIndex(14, eDVT_CONSTSOURCE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Unom", CConstVarIndex(15, eDVT_STATE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Eqnom", CConstVarIndex(16, eDVT_STATE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Eqe", CConstVarIndex(17, eDVT_STATE)));
	DeviceProps.m_ConstVarMap.insert(std::make_pair("Vref", CConstVarIndex(18, eDVT_INTERNALCONST)));
	DeviceProps.nEquationsCount = DeviceProps.m_VarMap.size();
	*/

	/*
	DeviceProps.m_ExtVarMap.insert(std::make_pair("S", CExtVarIndex(0, DEVTYPE_NODE)));
	DeviceProps.m_ExtVarMap.insert(std::make_pair("Eq", CExtVarIndex(1, DEVTYPE_GEN_1C)));
	DeviceProps.m_ExtVarMap.insert(std::make_pair("P", CExtVarIndex(2, DEVTYPE_UNKNOWN)));
	DeviceProps.m_ExtVarMap.insert(std::make_pair("V", CExtVarIndex(3, DEVTYPE_UNKNOWN)));
	*/
}

void CCustomDevice::Destroy()
{
	delete this;
}

const VariableIndexRefVec& CCustomDevice::GetVariables()
{
	return m_StateVariablesRefs;
}

const PRIMITIVEVECTOR& CCustomDevice::GetPrimitives()
{
	return m_Primitives;
}

const VariableIndexExternalRefVec& CCustomDevice::GetExternalVariables()
{
	return m_ExternalVariablesRefs;
}

extern "C" __declspec(dllexport) ICustomDevice * __cdecl CustomDeviceFactory()
{
	return new CCustomDevice();
}
