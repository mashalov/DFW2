#include "stdafx.h"
#include "DynaDECMustang.h"
#include "DynaExciterBase.h"
#include "DynaModel.h"

using namespace DFW2;


CDynaDECMustang::CDynaDECMustang() : CDevice(),

									 EnforceOn(this,	&EnforceOnValue,	V_ENFONRELAY,	&Vnode),
									 DeforceOn(this,	&DeforceOnValue,	V_DEFONRELAY,	&Vnode),
									 EnforceOff(this,	&EnforceOffValue,	V_ENFOFFRELAY,	&Vnode),
									 DeforceOff(this,	&DeforceOffValue,	V_DEFOFFRELAY,	&Vnode),
									 EnfTrigger(this,	&EnforceTrigValue,	V_ENFTRIG,		&EnforceOffOut,		&EnforceOnOut,		true),
									 DefTrigger(this,	&DeforceTrigValue,	V_DEFTRIG,		&DeforceOffOut,		&DeforceOnOut,		true),
									 EnforceOnOut(V_ENFONRELAY,		EnforceOnValue),
									 DeforceOnOut(V_DEFONRELAY,		DeforceOnValue),
									 EnforceOffOut(V_ENFOFFRELAY,	EnforceOffValue),
									 DeforceOffOut(V_DEFOFFRELAY,	DeforceOffValue)
{
	Prims.Add(&EnforceOn);
	Prims.Add(&DeforceOn);
	Prims.Add(&EnforceOff);
	Prims.Add(&DeforceOff);
	Prims.Add(&EnfTrigger);
	Prims.Add(&DefTrigger);
}

double* CDynaDECMustang::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;

	switch (nVarIndex)
	{
		MAP_VARIABLE(EnforceOnValue, V_ENFONRELAY)
		MAP_VARIABLE(EnforceOffValue, V_ENFOFFRELAY)
		MAP_VARIABLE(DeforceOnValue, V_DEFONRELAY)
		MAP_VARIABLE(DeforceOffValue, V_DEFOFFRELAY)
		MAP_VARIABLE(EnforceTrigValue, V_ENFTRIG)
		MAP_VARIABLE(DeforceTrigValue, V_DEFTRIG)
		MAP_VARIABLE(Udec, V_DEC)
	}
	return p;
}


eDEVICEFUNCTIONSTATUS CDynaDECMustang::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	double Unom, Eqnom, Eqe0;
	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);

	if (!InitConstantVariable(Unom, pExciter, CDynaGeneratorMotion::m_cszUnom, DEVTYPE_GEN_MOTION))
		Status = DFS_FAILED;
	if (!InitConstantVariable(Eqnom, pExciter, CDynaGenerator1C::m_cszEqnom, DEVTYPE_GEN_1C))
		Status = DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pExciter, CDynaGenerator1C::m_cszEqe, DEVTYPE_GEN_1C))
		Status = DFS_FAILED;

	Udec = 0.0;

	if (CDevice::IsFunctionStatusOK(Status) && IsStateOn())
	{
		EnforceOn.SetRefs(pDynaModel, Unom * VEnfOn, Unom * VEnfOn, false, TdelOn);
		EnforceOff.SetRefs(pDynaModel, Unom * VEnfOff, Unom * VEnfOff, true, TdelOff);
		DeforceOn.SetRefs(pDynaModel, Unom * VDefOn, Unom * VDefOn, true, TdelOn);
		DeforceOff.SetRefs(pDynaModel, Unom * VDefOff, Unom * VDefOff, false, TdelOff);

		m_dEnforceValue = EnfRatio * Eqnom - Eqe0;
		m_dDeforceValue = DefRatio * Eqnom - Eqe0;
		EnforceOn.Init(pDynaModel);
		DeforceOn.Init(pDynaModel);
		EnforceOff.Init(pDynaModel);
		DeforceOff.Init(pDynaModel);
		EnfTrigger.Init(pDynaModel);
		DefTrigger.Init(pDynaModel);
		ProcessDiscontinuity(pDynaModel);
	}
	return Status;
}

bool CDynaDECMustang::BuildEquations(CDynaModel* pDynaModel)
{
	if (!pDynaModel->Status())
		return pDynaModel->Status();
	pDynaModel->SetElement(A(V_DEC), A(V_DEC), 1.0);
	bool bRes = Prims.BuildEquations(pDynaModel);
	return pDynaModel->Status() && bRes;
}


bool CDynaDECMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	bool bRes = Prims.BuildRightHand(pDynaModel);
	pDynaModel->SetFunction(A(V_DEC), 0.0);
	return pDynaModel->Status();
}


bool CDynaDECMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = Prims.BuildDerivatives(pDynaModel);
	return bRes;
}

double CDynaDECMustang::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (IsStateOn())
		rH = Prims.CheckZeroCrossing(pDynaModel);
	return rH;
}

eDEVICEFUNCTIONSTATUS CDynaDECMustang::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_NOTREADY;
	CDynaExciterMustang *pExciter = static_cast<CDynaExciterMustang*>(GetSingleLink(DEVTYPE_EXCITER));
	_ASSERTE(CDevice::IsFunctionStatusOK(pExciter->DiscontinuityProcessed()));

	if (IsStateOn())
	{
		// wait for exciter to process disco

		Status = Prims.ProcessDiscontinuity(pDynaModel);
		pExciter->SetLagTimeConstantRatio(1.0);
		double dOldDec = Udec;

		Udec = 0.0;

		//_ASSERTE(!(EnforceTrigValue > 0.0 && DeforceTrigValue  > 0.0));

		// enforce is in priority

		if (EnforceTrigValue > 0.0)
		{
			Udec = m_dEnforceValue;
			pExciter->SetLagTimeConstantRatio(EnfTexc);
		}
		else
			if (DeforceTrigValue > 0.0)
			{
			Udec = m_dDeforceValue;
			pExciter->SetLagTimeConstantRatio(DefTexc);
			}

		if (!Equal(dOldDec, Udec))
			pDynaModel->DiscontinuityRequest();
	}
	else
		Status = DFS_OK;

	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaDECMustang::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = DeviceFunctionResult(InitExternalVariable(Vnode, GetSingleLink(DEVTYPE_EXCITER), CDynaNodeBase::m_cszV, DEVTYPE_NODE));
	return eRes;
}


const CDeviceContainerProperties CDynaDECMustang::DeviceProperties()
{
	CDeviceContainerProperties props;
	// базовый тип - дискретный контроллер возбуждения
	// свой тип - форсировка Мустанг
	props.SetType(DEVTYPE_DEC);	
	props.SetType(DEVTYPE_DEC_MUSTANG);	
	props.m_strClassName = CDeviceContainerProperties::m_cszNameDECMustang;
	// может линковаться с возбудителем
	props.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);

	props.nEquationsCount = CDynaDECMustang::VARS::V_LAST;

	props.m_VarMap.insert(make_pair(_T("EnfOnRly"), CVarIndex(CDynaDECMustang::V_ENFONRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("DefOnRly"), CVarIndex(CDynaDECMustang::V_DEFONRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("EnfOffRly"), CVarIndex(CDynaDECMustang::V_ENFOFFRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("DefOffRly"), CVarIndex(CDynaDECMustang::V_DEFOFFRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("EnfTrg"), CVarIndex(CDynaDECMustang::V_ENFTRIG, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("DefTrg"), CVarIndex(CDynaDECMustang::V_DEFTRIG, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(CDynaExciterBase::m_cszUdec, CVarIndex(CDynaDECMustang::V_DEC, VARUNIT_PU)));
	
	return props;
}