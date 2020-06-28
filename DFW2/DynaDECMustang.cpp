#include "stdafx.h"
#include "DynaDECMustang.h"
#include "DynaExciterMustang.h"
#include "DynaModel.h"

using namespace DFW2;


CDynaDECMustang::CDynaDECMustang() : CDevice(),

	EnforceOn(*this,  EnforceOnOut,		{ Vnode }),
	DeforceOn(*this,  DeforceOnOut,		{ Vnode }),
	EnforceOff(*this, EnforceOffOut,	{ Vnode }),
	DeforceOff(*this, DeforceOffOut,	{ Vnode }),
	EnfTrigger(*this, EnforceTrigOut, { EnforceOffOut, EnforceOnOut }),
	DefTrigger(*this, DeforceTrigOut, { DeforceOffOut, DeforceOnOut })
{

}


VariableIndexRefVec& CDynaDECMustang::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ EnforceOnOut,
												 EnforceOffOut,
												 DeforceOnOut,
												 DeforceOffOut,
												 EnforceTrigOut,
												 DeforceTrigOut,
												 Udec
											   }, ChildVec));
}

double* CDynaDECMustang::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);

	switch (nVarIndex)
	{
		MAP_VARIABLE(EnforceOnOut.Value, V_ENFONRELAY)
		MAP_VARIABLE(EnforceOffOut.Value, V_ENFOFFRELAY)
		MAP_VARIABLE(DeforceOnOut.Value, V_DEFONRELAY)
		MAP_VARIABLE(DeforceOffOut.Value, V_DEFOFFRELAY)
		MAP_VARIABLE(EnforceTrigOut.Value, V_ENFTRIG)
		MAP_VARIABLE(DeforceTrigOut.Value, V_DEFTRIG)
		MAP_VARIABLE(Udec.Value, V_DEC)
	}
	return p;
}


eDEVICEFUNCTIONSTATUS CDynaDECMustang::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

	double Unom, Eqnom, Eqe0;
	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);

	if (!InitConstantVariable(Unom, pExciter, CDynaGeneratorMotion::m_cszUnom, DEVTYPE_GEN_MOTION))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqnom, pExciter, CDynaGenerator1C::m_cszEqnom, DEVTYPE_GEN_1C))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pExciter, CDynaGenerator1C::m_cszEqe, DEVTYPE_GEN_1C))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

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
	pDynaModel->SetElement(Udec, Udec, 1.0);
	// строим уравнения для примитивов
	bool bRes = CDevice::BuildEquations(pDynaModel);
	return true;
}


bool CDynaDECMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	bool bRes = CDevice::BuildRightHand(pDynaModel);
	pDynaModel->SetFunction(Udec, 0.0);
	return true;
}


bool CDynaDECMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = CDevice::BuildDerivatives(pDynaModel);
	return bRes;
}

double CDynaDECMustang::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (IsStateOn())
		rH = CDevice::CheckZeroCrossing(pDynaModel);
	return rH;
}

eDEVICEFUNCTIONSTATUS CDynaDECMustang::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
	CDynaExciterMustang *pExciter = static_cast<CDynaExciterMustang*>(GetSingleLink(DEVTYPE_EXCITER));
	_ASSERTE(CDevice::IsFunctionStatusOK(pExciter->DiscontinuityProcessed()));

	if (IsStateOn())
	{
		// wait for exciter to process disco

		Status = CDevice::ProcessDiscontinuity(pDynaModel);
		pExciter->SetLagTimeConstantRatio(1.0);
		double dOldDec = Udec;

		Udec = 0.0;

		//_ASSERTE(!(EnforceTrigValue > 0.0 && DeforceTrigValue  > 0.0));

		// enforce is in priority

		if (EnforceTrigOut > 0.0)
		{
			Udec = m_dEnforceValue;
			pExciter->SetLagTimeConstantRatio(EnfTexc);
		}
		else
			if (DeforceTrigOut > 0.0)
			{
			Udec = m_dDeforceValue;
			pExciter->SetLagTimeConstantRatio(DefTexc);
			}

		if (!Equal(dOldDec, Udec))
			pDynaModel->DiscontinuityRequest();
	}
	else
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaDECMustang::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = DeviceFunctionResult(InitExternalVariable(Vnode, GetSingleLink(DEVTYPE_EXCITER), CDynaNodeBase::m_cszV, DEVTYPE_NODE));
	return eRes;
}


void CDynaDECMustang::UpdateSerializer(SerializerPtr& Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty(_T("sta"), TypedSerializedValue::eValueType::VT_STATE);
	Serializer->AddProperty(_T("Name"), TypedSerializedValue::eValueType::VT_NAME);
	Serializer->AddProperty(_T("Id"), TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty(_T("Ubf"), VEnfOn, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Uef"), VEnfOff, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Ubrf"), VDefOn, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Uerf"), VDefOff, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Rf"), EnfRatio);
	Serializer->AddProperty(_T("Rrf"), DefRatio);
	Serializer->AddProperty(_T("Texc_f"), EnfTexc, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(_T("Texc_rf"), DefTexc, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(_T("Tz_in"), TdelOn, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(_T("Tz_out"), TdelOff, eVARUNITS::VARUNIT_SECONDS);

	Serializer->AddState(_T("EnforceOnValue"), EnforceOnOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("DeforceOnValue"), DeforceOnOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("EnforceOffValue"), EnforceOffOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("DeforceOffValue"), DeforceOffOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("EnforceTrigValue"), EnforceTrigOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("DeforceTrigValue"), DeforceTrigOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("EnforceValue"), m_dEnforceValue, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("DeforceValue"), m_dDeforceValue, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Udec"), Udec, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Texc"), Texc, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Umin"), Umin, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Umax"), Umax, eVARUNITS::VARUNIT_PU);
}

const CDeviceContainerProperties CDynaDECMustang::DeviceProperties()
{
	CDeviceContainerProperties props;
	// базовый тип - дискретный контроллер возбуждения
	// свой тип - форсировка Мустанг
	props.SetType(DEVTYPE_DEC);	
	props.SetType(DEVTYPE_DEC_MUSTANG);	
	props.SetClassName(CDeviceContainerProperties::m_cszNameDECMustang, CDeviceContainerProperties::m_cszSysNameDECMustang);
	// может линковаться с возбудителем
	props.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);

	props.nEquationsCount = CDynaDECMustang::VARS::V_LAST;

	props.m_VarMap.insert(std::make_pair(_T("EnfOnRly"), CVarIndex(CDynaDECMustang::V_ENFONRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("DefOnRly"), CVarIndex(CDynaDECMustang::V_DEFONRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("EnfOffRly"), CVarIndex(CDynaDECMustang::V_ENFOFFRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("DefOffRly"), CVarIndex(CDynaDECMustang::V_DEFOFFRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("EnfTrg"), CVarIndex(CDynaDECMustang::V_ENFTRIG, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("DefTrg"), CVarIndex(CDynaDECMustang::V_DEFTRIG, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(CDynaExciterBase::m_cszUdec, CVarIndex(CDynaDECMustang::V_DEC, VARUNIT_PU)));
	
	return props;
}
