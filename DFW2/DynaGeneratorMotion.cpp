#include "stdafx.h"
#include "DynaGeneratorMotion.h"
#include "DynaModel.h"


using namespace DFW2;

VariableIndexRefVec& CDynaGeneratorMotion::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorInfBusBase::GetVariables(JoinVariables({ s, Delta },ChildVec));
}

double* CDynaGeneratorMotion::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaGeneratorInfBusBase::GetVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Delta.Value, V_DELTA)
			MAP_VARIABLE(s.Value, V_S)
		}
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::InitModel(CDynaModel* pDynaModel)
{
	auto Status{ CDynaGeneratorInfBusBase::InitModel(pDynaModel) };

	if (CDevice::IsFunctionStatusOK(Status))
	{ 
		if (Mj < 1E-7)
			Mj = 3 * Pnom;

		Mj /= Snom_; 

		s = 0;

		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
	}

	return Status;
}


eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::GetConnection(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS ret{ CDynaPowerInjector::GetConnection(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(ret))
	{
		Snom_ = Equal(cosPhinom, 0.0) ? Pnom : Pnom / cosPhinom;
		Qnom = Snom_ * std::sqrt(1.0 - cosPhinom * cosPhinom);
		Inom = Snom_ / Unom_ / CDynaModel::Sqrt3();
		puV_ = NodeUnom_ / Unom_;
		puI_ = Snom_ / pDynaModel->Sbase() * puV_;
	}
	return ret;

}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::PreInit(CDynaModel* pDynaModel)
{
		xq = 1.0;

		if (Kgen > 1)
		{
			xd1 /= Kgen;
			Pnom *= Kgen;
			xq /= Kgen; 
			Mj *= Kgen;
		}

		eDEVICEFUNCTIONSTATUS ret{ GetConnection(pDynaModel) };
		if (CDevice::IsFunctionStatusOK(ret))
		{
			P *= pDynaModel->Sbase() / Snom_;
			Q *= pDynaModel->Sbase() / Snom_;
			Zgen_ = { 0 , xd1 };
			// приводим сопротивление генератора к о.е. сети
			Zgen_ /= NodeUnom_ * NodeUnom_ / pDynaModel->Sbase();
			// шунт Нортона для УД
			Ynorton_ = 1.0 / Zgen_;
			xd1 /= Unom_ * Unom_ / Snom_;
		}
	
	return ret;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::Init(CDynaModel* pDynaModel)
{
	const auto ret{ InitModel(pDynaModel) };

	if (CDevice::IsFunctionStatusOK(ret))
	{
		Pt = P;
		Eqsxd1 = Eqs / xd1;
	}
	return ret;
}

bool CDynaGeneratorMotion::CalculatePower()
{
	Ire = Eqsxd1 * sin(Delta) - puV_ *  Vim / xd1;
	Iim = puV_ * Vre / xd1 - Eqsxd1 * cos(Delta);

	P = Vre * Ire + Vim * Iim;
	Q = -Vre * Iim + Vim * Ire;

	P *= puV_;
	Q *= puV_;
		
	if (std::abs(Ynorton_) > DFW2_EPSILON)
		FromComplex(Ire, Iim, GetEMF() / cplx(0, GetXofEqs()));
		
	return true;
}


void CDynaGeneratorMotion::BuildEquations(CDynaModel *pDynaModel)
{
	const double NodeSv{ Sv };
	double sp1{ ZeroGuardSlip(1.0 + s) };
	double sp2{ ZeroGuardSlip(1.0 + NodeSv) };

	if (!IsStateOn())
	{
		sp1 = sp2 = 1.0;
	}

	const double MjBySp2{ puV_ / sp2 / Mj };
	
	pDynaModel->SetElement(s, s, -(Kdemp + Pt / sp1 / sp1)/ Mj );
	pDynaModel->SetElement(s, Vre, Ire * MjBySp2);
	pDynaModel->SetElement(s, Vim, Iim * MjBySp2);
	pDynaModel->SetElement(s, Ire, Vre * MjBySp2);
	pDynaModel->SetElement(s, Iim, Vim * MjBySp2);
	pDynaModel->SetElement(s, Sv, -(Iim * Vim + Ire * Vre) * MjBySp2 / sp2);

	// dIre / dIre
	pDynaModel->SetElement(Ire, Ire, 1.0);
	// dIre / dDeltaG
	pDynaModel->SetElement(Ire, Delta, -Eqsxd1 * cos(Delta));

	// dIim / dIim
	pDynaModel->SetElement(Iim, Iim, 1.0);
	// dIim / dDeltaG
	pDynaModel->SetElement(Iim, Delta, -Eqsxd1 * sin(Delta));

	BuildAngleEquationBlock(pDynaModel);
}


void CDynaGeneratorMotion::BuildRightHand(CDynaModel *pDynaModel)
{
	const double NodeSv{ Sv };
	// в уравнение входит только составляющая тока генератора
	// от ЭДС
	pDynaModel->SetFunction(Ire, Ire - Eqsxd1 * sin(Delta));
	pDynaModel->SetFunction(Iim, Iim + Eqsxd1 * cos(Delta));
	SetFunctionsDiff(pDynaModel);
}

void CDynaGeneratorMotion::CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn)
{
	if (IsStateOn())
	{
		(pDynaModel->*fn)(Delta, pDynaModel->GetOmega0() * s);
		// интересно, что _активная_ мощность от токов ЭДС равна мощности от полных токов c Ynorton ?! 
		// I = (Ere + jEim - Vre - jVim) / jxd = (Eim - Vim + j(Vre - Ere)) / xd
		// Ire = (Eim - Vim) / xd 
		// Iim = (Vre - Ere) / xd
		// S = (Vre + jVim) * (Eim - Vim + j(Ere - Vre)) / xd 
		//   = (VreEim - VreVim + jVreEre - jVreVre + jVimEim - jVimVim - VimEim + VimVre) / xd
		//   = (VreEim - VreVim + VimVre - VimEim) / xd + j(VreEre - VreVre - VimVim - VimEim) / xd
		//   = (VreEim - VimEre) / xd + j(VreEre - VimEim - Vre^2 - Vim^2) / xd
		// напряжения в активной мощности вычитаются
		const auto Pel{ (Vre * Ire + Vim * Iim) * puV_ };
		(pDynaModel->*fn)(s, (ZeroDivGuard(Pt, 1.0 + s) - Kdemp * s - ZeroDivGuard(Pel, 1 + Sv)) / Mj);
	}
	else
	{
		(pDynaModel->*fn)(Delta, 0);
		(pDynaModel->*fn)(s, 0);
	}
}

void CDynaGeneratorMotion::BuildDerivatives(CDynaModel *pDynaModel)
{
	SetDerivatives(pDynaModel);
}

double* CDynaGeneratorMotion::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaGeneratorInfBusBase::GetConstVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Qnom, C_QNOM)
			MAP_VARIABLE(Inom, C_INOM)
		}
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaGeneratorInfBusBase::UpdateExternalVariables(pDynaModel);
}


void CDynaGeneratorMotion::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGeneratorInfBusBase::UpdateSerializer(Serializer);
	// добавляем свойства модели генератора в уравнении движения
	Serializer->AddProperty(m_cszKdemp, Kdemp, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddProperty(m_cszxq, xq, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszMj, Mj, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszPnom, Pnom, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty(m_cszcosPhinom, cosPhinom, eVARUNITS::VARUNIT_UNITLESS);
	// добавляем переменные состояния
	Serializer->AddState("Pt", Pt, eVARUNITS::VARUNIT_MW);
	Serializer->AddState("s", s, eVARUNITS::VARUNIT_PU);
}

void CDynaGeneratorMotion::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaGeneratorInfBusBase::UpdateValidator(Validator);
	Validator->AddRule(m_cszKdemp, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule({ m_cszxq, m_cszPnom }, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszcosPhinom, &CDynaGeneratorMotion::ValidatorCos);
	Validator->AddRule(CDynaNode::m_cszUnom, &CDynaGeneratorMotion::ValidatorUnom);
	Validator->AddRule(m_cszMj, &CDynaGeneratorMotion::ValidatorMj);
	Validator->AddRule(m_cszPnom, &CDynaGeneratorMotion::ValidatorPnom);
}

void CDynaGeneratorMotion::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorInfBusBase::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_MOTION);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorMotion, CDeviceContainerProperties::m_cszSysNameGeneratorMotion);
	props.EquationsCount = CDynaGeneratorMotion::VARS::V_LAST;

	props.VarMap_.insert(std::make_pair("S", CVarIndex(CDynaGeneratorMotion::V_S, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair(CDynaNodeBase::m_cszDelta, CVarIndex(CDynaGeneratorMotion::V_DELTA, VARUNIT_RADIANS)));

	props.ConstVarMap_.insert({ CDynaGeneratorMotion::m_cszInom, CConstVarIndex(CDynaGeneratorMotion::C_INOM, VARUNIT_KAMPERES, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaGeneratorMotion::m_cszQnom, CConstVarIndex(CDynaGeneratorMotion::C_QNOM, VARUNIT_MVAR, eDVT_INTERNALCONST) });

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGeneratorMotion>>();

}

void CDynaGeneratorMotion::BuildAngleEquationBlock(CDynaModel* pDynaModel)
{
	pDynaModel->SetElement(Delta, s, -pDynaModel->GetOmega0());
	pDynaModel->SetElement(Delta, Delta, 0.0);
}


CValidationRuleGeneratorUnom CDynaGeneratorMotion::ValidatorUnom;
CValidationRuleGeneratorMj CDynaGeneratorMotion::ValidatorMj;
CValidationRuleGeneratorPnom CDynaGeneratorMotion::ValidatorPnom;
