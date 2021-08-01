#include "stdafx.h"
#include "DynaDECMustang.h"
#include "DynaExciterMustang.h"
#include "DynaModel.h"

using namespace DFW2;


CDynaDECMustang::CDynaDECMustang() : CDevice(),

	EnforceOn(*this, { EnforceOnOut }, { Vnode }),
	DeforceOn(*this, { DeforceOnOut }, { Vnode }),
	EnforceOff(*this, { EnforceOffOut }, { Vnode }),
	DeforceOff(*this, { DeforceOffOut }, { Vnode }),
	EnfTrigger(*this, { EnforceTrigOut }, { EnforceOffOut, EnforceOnOut }),
	DefTrigger(*this, { DeforceTrigOut }, { DeforceOffOut, DeforceOnOut })
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
	if (!InitConstantVariable(Eqnom, pExciter, CDynaGeneratorDQBase::m_cszEqnom, DEVTYPE_GEN_DQ))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pExciter, CDynaGeneratorDQBase::m_cszEqe, DEVTYPE_GEN_DQ))
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

// сериализация 
void CDynaDECMustang::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем переданный сериализатор
	CDevice::UpdateSerializer(Serializer);
	// добавляем свойства форсировки
	AddStateProperty(Serializer);
	Serializer->AddProperty(CDevice::m_cszName, TypedSerializedValue::eValueType::VT_NAME);
	Serializer->AddProperty(m_cszid, TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty(m_cszUbf, VEnfOn, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszUef, VEnfOff, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszUbrf, VDefOn, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszUerf, VDefOff, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszRf, EnfRatio, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszRrf, DefRatio, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszTexc_f, EnfTexc, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszTexc_rf, DefTexc, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszTz_in, TdelOn, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_cszTz_out, TdelOff, eVARUNITS::VARUNIT_SECONDS);

	// добавляем переменные состояния форсировки
	Serializer->AddState("EnforceOnValue", EnforceOnOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("DeforceOnValue", DeforceOnOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("EnforceOffValue", EnforceOffOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("DeforceOffValue", DeforceOffOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("EnforceTrigValue", EnforceTrigOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("DeforceTrigValue", DeforceTrigOut, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("EnforceValue", m_dEnforceValue, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("DeforceValue", m_dDeforceValue, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Udec", Udec, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Texc", Texc, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Umin", Umin, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Umax", Umax, eVARUNITS::VARUNIT_PU);
}

void CDynaDECMustang::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDevice::UpdateValidator(Validator);
	Validator->AddRule({ m_cszTexc_f, m_cszTexc_rf }, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule({ m_cszTz_in, m_cszTz_out }, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule({ m_cszUef }, &CDynaDECMustang::ValidatorVenfOff);
	Validator->AddRule({ m_cszUbrf }, &CDynaDECMustang::ValidatorVdefOn);
	Validator->AddRule({ m_cszUerf }, &CDynaDECMustang::ValidatorVdefOff);
}

void CDynaDECMustang::DeviceProperties(CDeviceContainerProperties& props)
{
	// базовый тип - дискретный контроллер возбуждения
	// свой тип - форсировка Мустанг
	props.SetType(DEVTYPE_DEC);	
	props.SetType(DEVTYPE_DEC_MUSTANG);	
	props.SetClassName(CDeviceContainerProperties::m_cszNameDECMustang, CDeviceContainerProperties::m_cszSysNameDECMustang);
	// может линковаться с возбудителем
	props.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);

	props.nEquationsCount = CDynaDECMustang::VARS::V_LAST;

	props.m_VarMap.insert(std::make_pair("EnfOnRly", CVarIndex(CDynaDECMustang::V_ENFONRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("DefOnRly", CVarIndex(CDynaDECMustang::V_DEFONRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("EnfOffRly", CVarIndex(CDynaDECMustang::V_ENFOFFRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("DefOffRly", CVarIndex(CDynaDECMustang::V_DEFOFFRELAY, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("EnfTrg", CVarIndex(CDynaDECMustang::V_ENFTRIG, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("DefTrg", CVarIndex(CDynaDECMustang::V_DEFTRIG, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(CDynaExciterBase::m_cszUdec, CVarIndex(CDynaDECMustang::V_DEC, VARUNIT_PU)));

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaDECMustang>>();
}

CValidationRuleVenfOff CDynaDECMustang::ValidatorVenfOff;
CValidationRuleVdefOn CDynaDECMustang::ValidatorVdefOn;
CValidationRuleVdefOff CDynaDECMustang::ValidatorVdefOff;
