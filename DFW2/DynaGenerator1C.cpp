#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"


using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::InitModel(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaGeneratorDQBase::InitModel(pDynaModel) };

	if (CDevice::IsFunctionStatusOK(Status))
	{
		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
			zsq = 1.0 / (r * r + xd1 * xq);
			CDynaGenerator1C::ProcessDiscontinuity(pDynaModel);
			Eqs = Vq + r * Iq - xd1 * Id;
			ExtEqe = Eqs - Id * (xd - xd1);
			Eq = Eqs - Id * (xd - xd1); // repeat eq calc after eqs (first eq calc is in processdiscontinuity)
			break;
		case eDEVICESTATE::DS_OFF:
			zsq = 0.0;
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::PreInit(CDynaModel* pDynaModel)
{
	if (Kgen > 1)
	{
		xd1 /= Kgen;
		Pnom *= Kgen;
		xq /= Kgen;
		Mj *= Kgen;
		xd /= Kgen;
		r /= Kgen;
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes{ CDynaGeneratorDQBase::ProcessDiscontinuity(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(eRes))
	{
		if (IsStateOn())
		{
			Eq = Eqs - Id * (xd - xd1);
		}
		else
		{
			Id = Iq = Eq = Ire = Iim = 0.0;
		}
	}
	return eRes;
}

void CDynaGenerator1C::BuildEquations(CDynaModel *pDynaModel)
{
	double sp1{ ZeroGuardSlip(1.0 + s) }, sp2{ ZeroGuardSlip(1.0 + Sv) };

	if (!IsStateOn())
		sp1 = sp2 = 1.0;

	// S
	// DELTA
	// EQS
	// V_D
	// V_Q
	// I_D
	// I_Q
	// EQ

	// dId/dId
	pDynaModel->SetElement(Id, Id, 1);
	// dId/dVd
	pDynaModel->SetElement(Id, Vd, r * zsq);
	// dId/dVq
	pDynaModel->SetElement(Id, Vq, -xq * zsq);
	// dId/dEqs
	pDynaModel->SetElement(Id, Eqs, xq * zsq);

	// dIq/dIq
	pDynaModel->SetElement(Iq, Iq, 1);
	// dIq/dVd
	pDynaModel->SetElement(Iq, Vd, xd1 * zsq);
	// dIq/dVq
	pDynaModel->SetElement(Iq, Vq, r * zsq);
	// dIq/dEqs
	pDynaModel->SetElement(Iq, Eqs,  -r * zsq);

	// dEqs/dEqs
	pDynaModel->SetElement(Eqs, Eqs, -1.0 / Tdo1);
	// dEqs/dId
	pDynaModel->SetElement(Eqs, Id, -1.0 / Tdo1 * (xd - xd1));
	// dEqs/dEqe
	if (ExtEqe.Indexed())
		pDynaModel->SetElement(Eqs, ExtEqe, -1.0 / Tdo1);
	

	// dS / dS
	pDynaModel->SetElement(s, s, 1.0 / Mj * (-Kdemp - Pt / sp1 / sp1) );
	double Pairgap = P + (Id*Id + Iq*Iq) * r;
	// dS / dNodeS
	pDynaModel->SetElement(s, Sv, -1.0 / Mj * Pairgap / sp2 / sp2);
	// dS / Vd
	pDynaModel->SetElement(s, Vd,  1.0 / Mj * Id / sp2);
	// dS / Vq
	pDynaModel->SetElement(s, Vq, 1.0 / Mj * Iq / sp2);
	// dS / Id
	pDynaModel->SetElement(s, Id, 1.0 / Mj * (Vd + 2 * Id * r) / sp2);
	// dS / Iq
	pDynaModel->SetElement(s, Iq, 1.0 / Mj * (Vq + 2 * Iq * r) / sp2);

	// dEq / dEq
	pDynaModel->SetElement(Eq, Eq, 1.0);
	// dEq / dEqs
	pDynaModel->SetElement(Eq, Eqs, -1.0);
	// dEq / dId
	pDynaModel->SetElement(Eq, Id, xd - xd1);

	BuildAngleEquationBlock(pDynaModel);
	BuildRIfromDQEquations(pDynaModel);
}


void CDynaGenerator1C::BuildRightHand(CDynaModel *pDynaModel)
{
	double sp1{ ZeroGuardSlip(1.0 + s) }, sp2{ ZeroGuardSlip(1.0 + Sv) };

	if (!IsStateOn())
		sp1 = sp2 = 1.0;

	const double Pairgap{ P + (Id * Id + Iq * Iq) * r };

	pDynaModel->SetFunction(Id, Id - zsq * (-r * Vd - xq * (Eqs - Vq)));
	pDynaModel->SetFunction(Iq, Iq - zsq * (r * (Eqs - Vq) - xd1 * Vd));
	pDynaModel->SetFunction(Eq, Eq - Eqs + Id * (xd - xd1));
	const double eEqs{ (ExtEqe - Eqs + Id * (xd - xd1)) / Tdo1 };
	const  double eS{ (Pt / sp1 - Kdemp * s - Pairgap / sp2) / Mj };
	pDynaModel->SetFunctionDiff(s, eS);
	pDynaModel->SetFunctionDiff(Eqs, eEqs);
	pDynaModel->SetFunctionDiff(Delta, pDynaModel->GetOmega0() * s);
	BuildRIfromDQRightHand(pDynaModel);
}


void CDynaGenerator1C::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (IsStateOn())
	{
		double NodeV{ V };
		double DeltaGT{ Delta - DeltaV };
		double cosDeltaGT{ cos(DeltaGT) };
		double sp1{ ZeroGuardSlip(1.0 + s) }, sp2{ ZeroGuardSlip(1.0 + Sv) };

		if (!IsStateOn())
		{
			NodeV = cosDeltaGT = 0.0;
			sp1 = sp2 = 1.0;
		}

		const double Pairgap{ P + (Id * Id + Iq * Iq) * r };
		const double eDelta{ pDynaModel->GetOmega0() * s };
		const double eEqs{ (ExtEqe - Eqs + Id * (xd - xd1)) / Tdo1 };
		const double eS{ (Pt / sp1 - Kdemp * s - Pairgap / sp2) / Mj };
		pDynaModel->SetDerivative(s, eS);
		pDynaModel->SetDerivative(Delta, eDelta);
		pDynaModel->SetDerivative(Eqs, eEqs);
	}
	else
	{
		pDynaModel->SetDerivative(s, 0.0);
		pDynaModel->SetDerivative(Delta, 0.0);
		pDynaModel->SetDerivative(Eqs, 0.0);
	}
}

double* CDynaGenerator1C::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaGeneratorDQBase::GetVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Eqs.Value, V_EQS)
		}
	}
	return p;
}

const cplx& CDynaGenerator1C::CalculateEgen()
{
	const double xgen{ Zgen().imag() };
	return Egen_ = cplx(Eqs - Id * (xgen - xd1), Iq * (xgen - xq)) * std::polar(1.0, (double)Delta);
}

void CDynaGenerator1C::CalculatePower()
{

	double NodeV{ V };
	double DeltaGT{ Delta - DeltaV };
	double cosDeltaGT{ cos(DeltaGT) };
	double sinDeltaGT{ sin(DeltaGT) };
	double sp1{ ZeroGuardSlip(1.0 + s) };
	double sp2{ ZeroGuardSlip(1.0 + Sv) };

	if (!IsStateOn())
	{
		NodeV = cosDeltaGT = sinDeltaGT = 0.0;
		sp1 = sp2 = 1.0;
	}

	Vd = -NodeV * sinDeltaGT;
	Vq = NodeV * cosDeltaGT;
	Id = zsq * (-r * Vd - xq * (Eqs - Vq));
	Iq = zsq * (r * (Eqs - Vq) - xd1 * Vd);
	P = Vd * Id + Vq * Iq;
	Q = Vd * Iq - Vq * Id;
}

void CDynaGenerator1C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorDQBase::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_1C);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGenerator1C, CDeviceContainerProperties::m_cszSysNameGenerator1C);
	props.EquationsCount = CDynaGenerator1C::VARS::V_LAST;
	props.VarMap_.insert(std::make_pair("Eqs", CVarIndex(CDynaGenerator1C::V_EQS, VARUNIT_KVOLTS)));
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGenerator1C>>();
}

VariableIndexRefVec& CDynaGenerator1C::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorDQBase::GetVariables(JoinVariables({ Eqs }, ChildVec));
}

void CDynaGenerator1C::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGeneratorDQBase::UpdateSerializer(Serializer);
	// добавляем свойства модели одноконтурной модели генератора в ЭДС
	Serializer->AddProperty(m_csztdo1, Tdo1, eVARUNITS::VARUNIT_SECONDS);
	// добавляем переменные состояния модели одноконтурной модели генератора в ЭДС
	Serializer->AddState("zsq", zsq, eVARUNITS::VARUNIT_PU);
}