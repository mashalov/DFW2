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
		MAP_VARIABLE(SyncDelta_, C_SYNCDELTA)
	}
	return p;
}

VariableIndexRefVec& CDynaPowerInjector::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Ire, Iim, P, Q },ChildVec));
}

double* CDynaPowerInjector::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ nullptr };

	switch (nVarIndex)
	{
		MAP_VARIABLE(Ire.Value, V_IRE)
		MAP_VARIABLE(Iim.Value, V_IIM)
		MAP_VARIABLE(P.Value, V_P)
		MAP_VARIABLE(Q.Value, V_Q)
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::InitModel(CDynaModel* pDynaModel)
{
	if (!IsStateOn())
		P = Q = Ire = Iim = 0.0;

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (IsStateOn())
	{
		const double g{ Ynorton_.real() }, b{ Ynorton_.imag() };
		const double vre{ Vre }, vim{ Vim };
		const double ire{ Ire }, iim{ Iim };
		P = vre * ire + vim * iim - (vre * vre - vim * vim) * g;
		Q = vim * ire - vre * iim + (vre * vre + vim * vim) * b;
	}
	else
		P = Q = Ire = Iim = 0.0;

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaPowerInjector::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes{ DeviceFunctionResult(InitExternalVariable(V, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::cszV_, DEVTYPE_NODE)) };
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(DeltaV, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::cszDelta_, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Vre, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::cszVre_, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Vim, GetSingleLink(DEVTYPE_NODE), CDynaNodeBase::cszVim_, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Sv, GetSingleLink(DEVTYPE_NODE), pDynaModel->GetDampingName(), DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Scoi, GetSingleLink(DEVTYPE_NODE), CDynaNode::cszSz_, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(Dcoi, GetSingleLink(DEVTYPE_NODE), CDynaNode::cszSyncDelta_, DEVTYPE_NODE));
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

void CDynaPowerInjector::BuildRightHand(CDynaModel* pDynaModel)
{
	// если в модели инжектора учитывается шунт Нортона,
	// рассчитываем токи в шунте и добавляем к току инжектора
	//if (std::abs(Ynorton_) > Consts::epsilon)
	//	FromComplex(dIre, dIim, cplx(dIre, dIim) - cplx(dVre, dVim) * Ynorton_);

	const double g{ Ynorton_.real() }, b{ Ynorton_.imag() };
	const double vre{ Vre }, vim{ Vim };
	const double ire{ Ire }, iim{ Iim };
	const double vre2{ vre * vre }, vim2{ vim * vim };
	const double dp{ P - vre * ire - vim * iim + (vre2 - vim2) * g };
	const double dq{ Q - vim * ire + vre * iim - (vre2 + vim2) * b };

	pDynaModel->SetFunction(P, dp);
	pDynaModel->SetFunction(Q, dq);
}

void CDynaPowerInjector::BuildEquations(CDynaModel* pDynaModel)
{
	const double g{ Ynorton_.real() }, b{ Ynorton_.imag() };
	const double vre{ Vre }, vim{ Vim };
	const double ire{ Ire }, iim{ Iim };

	pDynaModel->SetElement(P, P, 1.0);
	pDynaModel->SetElement(P, Vre, 2.0 * g * vre - ire);
	pDynaModel->SetElement(P, Vim, -(2.0 * g * vim + iim));
	pDynaModel->SetElement(P, Ire, -vre);
	pDynaModel->SetElement(P, Iim, -vim);

	pDynaModel->SetElement(Q, Q, 1.0);
	pDynaModel->SetElement(Q, Vre, iim  - 2.0 * b * vre);
	pDynaModel->SetElement(Q, Vim, -(ire + 2.0 * b * vim));
	pDynaModel->SetElement(Q, Ire, -vim);
	pDynaModel->SetElement(Q, Iim, vre);

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
	Serializer->AddProperty(CDevice::cszName_, TypedSerializedValue::eValueType::VT_NAME, eVARUNITS::VARUNIT_UNITLESS);
	AddStateProperty(Serializer);
	Serializer->AddProperty(cszid_, TypedSerializedValue::eValueType::VT_ID, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszNodeId, NodeId, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszP, P, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty(m_cszQ, Q, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddState(m_cszIre, Ire, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState(m_cszIim, Iim, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddProperty(m_cszQmin, LFQmin, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty(m_cszQmax, LFQmax, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty(m_cszKgen, Kgen, eVARUNITS::VARUNIT_PIECES);
}

void  CDynaPowerInjector::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_POWER_INJECTOR);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, CDynaPowerInjector::m_cszNodeId);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorPowerInjector, CDeviceContainerProperties::m_cszSysNameGeneratorPowerInjector);

	props.VarMap_.insert({ m_cszIre, CVarIndex(CDynaPowerInjector::V_IRE, VARUNIT_KAMPERES) });
	props.VarMap_.insert({ m_cszIim, CVarIndex(CDynaPowerInjector::V_IIM, VARUNIT_KAMPERES) });
	props.VarMap_.insert({ m_cszP, CVarIndex(CDynaPowerInjector::V_P, VARUNIT_MW) });
	props.VarMap_.insert({ m_cszQ, CVarIndex(CDynaPowerInjector::V_Q, VARUNIT_MVAR) });

	props.ConstVarMap_.insert({ CDynaPowerInjector::m_cszNodeId, CConstVarIndex(CDynaPowerInjector::C_NODEID, VARUNIT_UNITLESS, eDVT_CONSTSOURCE) });

	props.EquationsCount = CDynaPowerInjector::VARS::V_LAST;
	props.Aliases_.push_back(CDeviceContainerProperties::m_cszAliasGenerator);
	props.bUseCOI = true;
	props.SyncDeltaId = CDynaPowerInjector::C_SYNCDELTA;
	props.bStoreStates = false;
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaPowerInjector>>();
}

CValidationRuleGeneratorKgen CDynaPowerInjector::ValidatorKgen;
CValidationRuleGeneratorUnom CDynaPowerInjector::ValidatorUnom;

