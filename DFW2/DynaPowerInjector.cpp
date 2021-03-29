#include "stdafx.h"
#include "DynaNode.h"
#include "DynaPowerInjector.h"
#include "DynaModel.h"
using namespace DFW2;

CDynaPowerInjector::CDynaPowerInjector() : CDevice()
{

}

double* CDynaPowerInjector::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(NodeId, C_NODEID)
	}
	return p;
}

VariableIndexRefVec& CDynaPowerInjector::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ P, Q, Ire, Iim },ChildVec));
}

double* CDynaPowerInjector::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);

	switch (nVarIndex)
	{
		MAP_VARIABLE(P.Value, V_P)
		MAP_VARIABLE(Q.Value, V_Q)
		MAP_VARIABLE(Ire.Value, V_IRE)
		MAP_VARIABLE(Iim.Value, V_IIM)
	}
	return p;
}


eDEVICEFUNCTIONSTATUS CDynaPowerInjector::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

	if (!IsStateOn())
	{
		P = Q = Ire = Iim = 0.0;
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
	}

	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = DeviceFunctionResult(InitExternalVariable(V, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::m_cszV, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(DeltaV, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::m_cszDelta, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Vre, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::m_cszVre, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Vim, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::m_cszVim, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Sv, GetSingleLink(DEVTYPE_NODE), pDynaModel->GetDampingName(), DEVTYPE_NODE));
	return eRes;
}


void CDynaPowerInjector::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty("Name", TypedSerializedValue::eValueType::VT_NAME);
	AddStateProperty(Serializer);
	Serializer->AddProperty("Id", TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty("NodeId", NodeId, eVARUNITS::VARUNIT_NOTSET);
	Serializer->AddProperty("P", P, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty("Q", Q, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddState("Ire", Ire, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState("Iim", Iim, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddProperty("Qmin", LFQmin, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty("Qmax", LFQmax, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty("Kgen", Kgen, eVARUNITS::VARUNIT_PIECES);
}

void  CDynaPowerInjector::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_POWER_INJECTOR);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, CDynaPowerInjector::m_cszNodeId);

	props.m_VarMap.insert(std::make_pair("Ire", CVarIndex(CDynaPowerInjector::V_IRE, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Iim", CVarIndex(CDynaPowerInjector::V_IIM, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("P", CVarIndex(CDynaPowerInjector::V_P, VARUNIT_MW)));
	props.m_VarMap.insert(std::make_pair("Q", CVarIndex(CDynaPowerInjector::V_Q, VARUNIT_MVAR)));

	props.m_ConstVarMap.insert(std::make_pair(CDynaPowerInjector::m_cszNodeId, CConstVarIndex(CDynaPowerInjector::C_NODEID, eDVT_CONSTSOURCE)));

	props.nEquationsCount = CDynaPowerInjector::VARS::V_LAST;
	props.m_lstAliases.push_back(CDeviceContainerProperties::m_cszAliasGenerator);

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaPowerInjector>>();

}

const char* CDynaPowerInjector::m_cszNodeId = "NodeId";