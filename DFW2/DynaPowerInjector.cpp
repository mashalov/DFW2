#include "stdafx.h"
#include "DynaNode.h"
#include "DynaPowerInjector.h"
#include "DynaModel.h"
using namespace DFW2;

double* CDynaPowerInjector::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ nullptr };
	switch (nVarIndex)
	{
		MAP_VARIABLE(NodeId, C_NODEID)
		MAP_VARIABLE(P, C_P)
		MAP_VARIABLE(Q, C_Q)
	}
	return p;
}

VariableIndexRefVec& CDynaPowerInjector::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Ire, Iim },ChildVec));
}

double* CDynaPowerInjector::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ nullptr };

	switch (nVarIndex)
	{
		MAP_VARIABLE(Ire.Value, V_IRE)
		MAP_VARIABLE(Iim.Value, V_IIM)
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::InitModel(CDynaModel* pDynaModel)
{
	if (!IsStateOn())
		P = Q = Ire = Iim = 0.0;

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes{ DeviceFunctionResult(InitExternalVariable(V, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::m_cszV, DEVTYPE_NODE)) };
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(DeltaV, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::m_cszDelta, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Vre, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::m_cszVre, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Vim, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::m_cszVim, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Sv, GetSingleLink(DEVTYPE_NODE), pDynaModel->GetDampingName(), DEVTYPE_NODE));
	return eRes;
}

bool CDynaPowerInjector::CalculatePower()
{
	return true;
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice)
{
	// если устройство отключается - обнуляем выходные мощности
	// они не являются переменными состояния и не обнуляются
	// в базовом классе
	const auto SetStateRet{ CDevice::SetState(eState, eStateCause, pCauseDevice) };
	if (CDevice::IsFunctionStatusOK(SetStateRet) && !IsStateOn())
		P = Q = 0;
	return SetStateRet;
}

void CDynaPowerInjector::FinishStep(const CDynaModel& DynaModel)
{
	const double dVre{ Vre }, dVim{ Vim };
	double dIre{ Ire }, dIim{ Iim };
	// если в модели инжектора учитывается шунт Нортона,
	// рассчитываем токи в шунте и добавляем к току инжектора
	if (std::abs(Ynorton_) > DFW2_EPSILON)
		FromComplex(dIre, dIim, cplx(dIre, dIim) - cplx(dVre,dVim) * Ynorton_);
	P = dVre * dIre + dVim * dIim;
	Q = dVim * dIre - dVre * dIim;
}

void CDynaPowerInjector::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDevice::UpdateValidator(Validator);
	Validator->AddRule(m_cszNodeId, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszKgen, &CDynaPowerInjector::ValidatorKgen);
}

void CDynaPowerInjector::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty(CDevice::m_cszName, TypedSerializedValue::eValueType::VT_NAME, eVARUNITS::VARUNIT_UNITLESS);
	AddStateProperty(Serializer);
	Serializer->AddProperty(m_cszid, TypedSerializedValue::eValueType::VT_ID, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszNodeId, NodeId, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszP, P, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty(m_cszQ, Q, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddState(m_cszIre, Ire, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState(m_cszIim, Iim, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddProperty("Qmin", LFQmin, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty("Qmax", LFQmax, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty(m_cszKgen, Kgen, eVARUNITS::VARUNIT_PIECES);
}

void  CDynaPowerInjector::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_POWER_INJECTOR);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, CDynaPowerInjector::m_cszNodeId);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorPowerInjector, CDeviceContainerProperties::m_cszSysNameGeneratorPowerInjector);

	props.VarMap_.insert({ m_cszIre, CVarIndex(CDynaPowerInjector::V_IRE, VARUNIT_KAMPERES) });
	props.VarMap_.insert({ m_cszIim, CVarIndex(CDynaPowerInjector::V_IIM, VARUNIT_KAMPERES) });

	props.ConstVarMap_.insert({ CDynaPowerInjector::m_cszNodeId, CConstVarIndex(CDynaPowerInjector::C_NODEID, VARUNIT_UNITLESS, eDVT_CONSTSOURCE) });
	props.ConstVarMap_.insert({ m_cszP, CConstVarIndex(CDynaPowerInjector::C_P, VARUNIT_MW, true, eDVT_CONSTSOURCE) });
	props.ConstVarMap_.insert({ m_cszQ, CConstVarIndex(CDynaPowerInjector::C_Q, VARUNIT_MVAR, true, eDVT_CONSTSOURCE) });

	props.EquationsCount = CDynaPowerInjector::VARS::V_LAST;
	props.Aliases_.push_back(CDeviceContainerProperties::m_cszAliasGenerator);
	props.bFinishStep = true;	// нужно рассчитывать мощности после выполнения шага
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaPowerInjector>>();
}

CValidationRuleGeneratorKgen CDynaPowerInjector::ValidatorKgen;

