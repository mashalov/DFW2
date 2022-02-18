#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"
#include "DynaGenerator3C.h"

using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGenerator3C::PreInit(CDynaModel* pDynaModel)
{
	xq1 = xq;

	if (Kgen > 1)
	{
		xd1 /= Kgen;
		Pnom *= Kgen;
		xq /= Kgen;
		Mj *= Kgen;
		xd /= Kgen;
		r /= Kgen;
		xq1 /= Kgen;
		xd2 /= Kgen;
		xq2 /= Kgen;
	}

	Zgen_ = { 0, 0.5 * (xd2 + xq2) };

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator3C::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGenerator3C::InitModel(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaGenerator1C::InitModel(pDynaModel) };

	if (CDevice::IsFunctionStatusOK(Status))
	{
		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
			zsq = 1.0 / (r * r + xd2 * xq2);
			Edss = Vd + r * Id + xq2 * Iq;
			Eqss = Vq - xd2 * Id + r * Iq;
			Eqs = Eqss - Id * (xd1 - xd2);
			ExtEqe = Eqs - Id * (xd - xd1);
			Eq = Eqss - Id * (xd - xd2);
			break;
		case eDEVICESTATE::DS_OFF:
			zsq = Id = Iq = Eqss = Eqs = Edss = Eq = 0.0;
			break;
		}
	}

	return Status;
}

void CDynaGenerator3C::BuildEquations(CDynaModel *pDynaModel)
{
	double NodeV{ V };
	double DeltaGT{ Delta - DeltaV };

	const double cosDeltaGT{ cos(DeltaGT) }, sinDeltaGT{ sin(DeltaGT) };

	// P
	// Q
	// S
	// DELTA
	// EQS
	// V_D
	// V_Q
	// I_D
	// I_Q
	// EQSS
	// EDSS

	// dVd/dVd
	pDynaModel->SetElement(Vd, Vd, 1);
	// dVd/dV
	pDynaModel->SetElement(Vd, V, sinDeltaGT);
	// dVd/dDeltaV
	pDynaModel->SetElement(Vd, DeltaV, -NodeV * cosDeltaGT);
	// dVd/dDeltaG
	pDynaModel->SetElement(Vd, Delta, NodeV * cosDeltaGT);

	// dVq/dVq
	pDynaModel->SetElement(Vq, Vq, 1);
	// dVq/dV
	pDynaModel->SetElement(Vq, V, -cosDeltaGT);
	// dVq/dDeltaV
	pDynaModel->SetElement(Vq, DeltaV, -NodeV * sinDeltaGT);
	// dVq/dDeltaG
	pDynaModel->SetElement(Vq, Delta, NodeV * sinDeltaGT);

	// dId/dId
	pDynaModel->SetElement(Id, Id, 1);
	// dId/dVd
	pDynaModel->SetElement(Id, Vd, r * zsq);
	// dId/dVq
	pDynaModel->SetElement(Id, Vq, -xq2 * zsq);
	// dId/dEdss
	pDynaModel->SetElement(Id, Edss, -r * zsq);
	// dId/dEqss
	pDynaModel->SetElement(Id, Eqss, xq2 * zsq);

	// dIq/dIq
	pDynaModel->SetElement(Iq, Iq, 1);
	// dIq/dVd
	pDynaModel->SetElement(Iq, Vd, xd2 * zsq);
	// dIq/dVq
	pDynaModel->SetElement(Iq, Vq, r * zsq);
	// dIq/dEdss
	pDynaModel->SetElement(Iq, Edss, -xd2 * zsq);
	// dIq/dEqss
	pDynaModel->SetElement(Iq, Eqss, -r * zsq);

	// dEqs/dEqs
	pDynaModel->SetElement(Eqs, Eqs, 1.0 / Tdo1);
	// dEqs/dId
	pDynaModel->SetElement(Eqs, Id, -1.0 / Tdo1 * (xd - xd1));
	// dEqs/dEqe
	if (ExtEqe.Indexed())
		pDynaModel->SetElement(Eqs, ExtEqe, -1.0 / Tdo1);


	double sp1{ ZeroGuardSlip(1.0 + s) };
	double sp2{ ZeroGuardSlip(1.0 + Sv) };
	// dS / dS

	pDynaModel->SetElement(s, s, 1.0 / Mj * (-Kdemp - Pt / sp1 / sp1));
	double Pairgap = P + (Id*Id + Iq*Iq) * r;
	// dS / dNodeS
	pDynaModel->SetElement(s, Sv, -1.0 / Mj * Pairgap / sp2 / sp2);
	// dS / Vd
	pDynaModel->SetElement(s, Vd, 1.0 / Mj * Id / sp2);
	// dS / Vq
	pDynaModel->SetElement(s, Vq, 1.0 / Mj * Iq / sp2);
	// dS / Id
	pDynaModel->SetElement(s, Id, 1.0 / Mj * (Vd + 2 * Id * r) / sp2);
	// dS / Iq
	pDynaModel->SetElement(s, Iq, 1.0 / Mj * (Vq + 2 * Iq * r) / sp2);
		
	// dEqss / dEqss
	pDynaModel->SetElement(Eqss, Eqss, -1.0 / Tdo2);
	// dEqss / dEqs
	pDynaModel->SetElement(Eqss, Eqs, - 1.0 / Tdo2);
	// dEqss / dId
	pDynaModel->SetElement(Eqss, Id, -(xd1 - xd2) / Tdo2);

	// dEdss / dEdss
	pDynaModel->SetElement(Edss, Edss, -1.0 / Tqo2);
	// dEdss / dIq
	pDynaModel->SetElement(Edss, Iq, (xq1 - xq2) / Tqo2);


	// dEq / dEq
	pDynaModel->SetElement(Eq, Eq, 1.0);
	// dEq / dEqss
	pDynaModel->SetElement(Eq, Eqss, -1.0);
	// dEq / dId
	pDynaModel->SetElement(Eq, Id, xd - xd2);

	BuildAngleEquationBlock(pDynaModel);
}


void CDynaGenerator3C::BuildRightHand(CDynaModel *pDynaModel)
{
	double NodeV{ V };
	double DeltaGT{ Delta - DeltaV };
	const double cosDeltaGT{ cos(DeltaGT) }, sinDeltaGT{ sin(DeltaGT) };
	const double sp1{ ZeroGuardSlip(1.0 + s) }, sp2{ ZeroGuardSlip(1.0 + Sv) };
		
	const double Pairgap{ P + (Id * Id + Iq * Iq) * r };
		
	pDynaModel->SetFunction(Vd, Vd + NodeV * sinDeltaGT);
	pDynaModel->SetFunction(Vq, Vq - NodeV * cosDeltaGT);
	pDynaModel->SetFunction(Id, Id + zsq * (r * (Vd - Edss) + xq2 * (Eqss - Vq)));
	pDynaModel->SetFunction(Iq, Iq + zsq * (r * (Vq - Eqss) + xd2 * (Vd - Edss)));
	pDynaModel->SetFunction(Eq, Eq - Eqss + Id * (xd - xd2));


	const double eEqs{ (ExtEqe - Eqs + Id * (xd - xd1)) / Tdo1 };
	const double eS{ (Pt / sp1 - Kdemp * s - Pairgap / sp2) / Mj };

	const double eEqss{ (Eqs - Eqss + Id * (xd1 - xd2)) / Tdo2 };
	const double eEdss{ (-Edss - Iq * (xq1 - xq2)) / Tqo2 };

	pDynaModel->SetFunctionDiff(s, eS);
	pDynaModel->SetFunctionDiff(Eqs, eEqs);
	pDynaModel->SetFunctionDiff(Eqss, eEqss);
	pDynaModel->SetFunctionDiff(Edss, eEdss);

	pDynaModel->SetFunctionDiff(Delta, pDynaModel->GetOmega0() * s);
}


void CDynaGenerator3C::BuildDerivatives(CDynaModel *pDynaModel)
{
	CDynaGenerator1C::BuildDerivatives(pDynaModel);
	if (IsStateOn())
	{
		const double eEqss{ (Eqs - Eqss + Id * (xd1 - xd2)) / Tdo2 };
		pDynaModel->SetDerivative(Eqss, eEqss);
		const double eEdss{ (-Edss - Iq * (xq1 - xq2)) / Tqo2 };
		pDynaModel->SetDerivative(Edss, eEdss);
	}
	else
	{
		pDynaModel->SetDerivative(Eqss, 0.0);
		pDynaModel->SetDerivative(Edss, 0.0);
	}
}



double* CDynaGenerator3C::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaGenerator1C::GetVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Eqss.Value, V_EQSS)
			MAP_VARIABLE(Edss.Value, V_EDSS)
		}
	}
	return p;
}

VariableIndexRefVec& CDynaGenerator3C::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGenerator1C::GetVariables(JoinVariables({ Eqss, Edss }, ChildVec));
}


double* CDynaGenerator3C::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaGenerator1C::GetConstVariablePtr(nVarIndex) };
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator3C::ProcessDiscontinuity(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes{ CDynaGenerator1C::ProcessDiscontinuity(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(eRes) && IsStateOn())
	{
		Id = -zsq * (r * (Vd - Edss) + xq2 * (Eqss - Vq));
		Eq = Eqss - Id * (xd - xd2);
	}
	return eRes;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator3C::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaGenerator1C::UpdateExternalVariables(pDynaModel);
}

bool CDynaGenerator3C::CalculatePower()
{
	
	double NodeV{ V };
	double DeltaGT{ Delta - DeltaV };
	const double cosDeltaGT{ cos(DeltaGT) };
	const double sinDeltaGT{ sin(DeltaGT) };

	double sp1 = ZeroGuardSlip(1.0 + s);
	double sp2 = ZeroGuardSlip(1.0 + Sv);

	double Pairgap = P + (Id*Id + Iq * Iq) * r;

	Vd = -NodeV * sinDeltaGT;
	Vq = NodeV * cosDeltaGT;
	Id = -zsq * (r * (Vd - Edss) + xq2 * (Eqss - Vq));
	Iq = -zsq * (r * (Vq - Eqss) + xd2 * (Vd - Edss));
	P = Vd * Id + Vq * Iq;
	Q = Vd * Iq - Vq * Id;

	return true;
}

cplx CDynaGenerator3C::GetEMF()
{
	return cplx(Eqss, Edss) * std::polar(1.0, (double)Delta);
}

const cplx& CDynaGenerator3C::CalculateEgen()
{
	const double xgen{ Zgen().imag() };
	return Egen_ = cplx(Eqss - Id * (xgen - xd2), Edss + Iq*(xgen - xq2)) * std::polar(1.0, (double)Delta);
}

void CDynaGenerator3C::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaGenerator1C::UpdateValidator(Validator);
	Validator->AddRule({ m_csztdo2, m_csztqo2, m_cszxd2, m_cszxq2, m_cszxq1 }, &CSerializerValidatorRules::BiggerThanZero);
}

void CDynaGenerator3C::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGenerator1C::UpdateSerializer(Serializer);
	// добавляем перменные состояния трехконтурной модели в ЭДС
	Serializer->AddState("Eqss", Eqss, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Edss", Edss, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty(m_csztdo2, Tdo2, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztqo2, Tqo2, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_cszxd2, xd2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszxq2, xq2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszxq1, xq1, eVARUNITS::VARUNIT_OHM);
}

void CDynaGenerator3C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGenerator1C::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_3C);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGenerator3C, CDeviceContainerProperties::m_cszSysNameGenerator3C);
	props.nEquationsCount = CDynaGenerator3C::VARS::V_LAST;
	props.VarMap_.insert(std::make_pair("Eqss", CVarIndex(CDynaGenerator3C::V_EQSS, VARUNIT_KVOLTS)));
	props.VarMap_.insert(std::make_pair("Edss", CVarIndex(CDynaGenerator3C::V_EDSS, VARUNIT_KVOLTS)));

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGenerator3C>>();
}