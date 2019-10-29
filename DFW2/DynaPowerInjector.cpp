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

double* CDynaPowerInjector::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);

	switch (nVarIndex)
	{
		MAP_VARIABLE(P, V_P)
		MAP_VARIABLE(Q, V_Q)
		MAP_VARIABLE(Ire, V_IRE)
		MAP_VARIABLE(Iim, V_IIM)
	}
	return p;
}


eDEVICEFUNCTIONSTATUS CDynaPowerInjector::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	if (!IsStateOn())
	{
		P = Q = Ire = Iim = 0.0;
		Status = DFS_OK;
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

const CDeviceContainerProperties CDynaPowerInjector::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.SetType(DEVTYPE_POWER_INJECTOR);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, CDynaPowerInjector::m_cszNodeId);

	props.m_VarMap.insert(make_pair(_T("Ire"), CVarIndex(CDynaPowerInjector::V_IRE, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("Iim"), CVarIndex(CDynaPowerInjector::V_IIM, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("P"), CVarIndex(CDynaPowerInjector::V_P, VARUNIT_MW)));
	props.m_VarMap.insert(make_pair(_T("Q"), CVarIndex(CDynaPowerInjector::V_Q, VARUNIT_MVAR)));

	props.m_ConstVarMap.insert(make_pair(CDynaPowerInjector::m_cszNodeId, CConstVarIndex(CDynaPowerInjector::C_NODEID, eDVT_CONSTSOURCE)));

	props.nEquationsCount = CDynaPowerInjector::VARS::V_LAST;
	props.m_lstAliases.push_back(CDeviceContainerProperties::m_cszAliasGenerator);
	return props;
}

const _TCHAR* CDynaPowerInjector::m_cszNodeId = _T("NodeId");