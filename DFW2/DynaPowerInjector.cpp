#include "stdafx.h"
#include "DynaNode.h"
#include "DynaPowerInjector.h"
#include "DynaModel.h"
using namespace DFW2;

CDynaPowerInjector::CDynaPowerInjector() : CDevice(),
										   Kgen(1)
{

}

double* CDynaPowerInjector::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;
	switch (nVarIndex)
	{
		MAP_VARIABLE(NodeId, C_NODEID)
	}
	return p;
}

double* CDynaPowerInjector::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;

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
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Sv, GetSingleLink(DEVTYPE_NODE), pDynaModel->GetDampingName(), DEVTYPE_NODE));
	return eRes;
}

const CDeviceContainerProperties CDynaPowerInjector::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.SetType(DEVTYPE_POWER_INJECTOR);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, CDynaPowerInjector::m_cszNodeId);
	props.m_ConstVarMap.insert(make_pair(CDynaPowerInjector::m_cszNodeId, CConstVarIndex(CDynaPowerInjector::C_NODEID, eDVT_CONSTSOURCE)));
	props.nEquationsCount = CDynaPowerInjector::VARS::V_LAST;
	props.m_lstAliases.push_back(CDeviceContainerProperties::m_cszAliasGenerator);
	return props;
}

const _TCHAR* CDynaPowerInjector::m_cszNodeId = _T("NodeId");