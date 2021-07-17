#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"


using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::Init(CDynaModel* pDynaModel)
{

	if (std::abs(xq) < 1E-7) xq = xd1; // place to validation !!!
	if (xd <= 0) xd = xd1;
	if (xq <= 0) xq = xd1;

	if (Kgen > 1)
	{
		xd /= Kgen;
		r /= Kgen;
	}

	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorDQBase::Init(pDynaModel);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		Snom = Equal(cosPhinom, 0.0) ? Pnom : Pnom / cosPhinom;
		Qnom = Snom * sqrt(1.0 - cosPhinom * cosPhinom);
		Inom = Snom / Unom / M_SQRT3;
		Eqnom = (Unom * Unom * (Unom * Unom + Qnom * (xd + xq)) + Snom * Snom * xd * xq) / (Unom * sqrt(Unom * Unom * (Unom * Unom + 2.0 * Qnom * xq) + Snom * Snom * xq * xq));

		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
			zsq = 1.0 / (r*r + xd1 * xq);
			CDynaGenerator1C::ProcessDiscontinuity(pDynaModel);
			Eqs = Vq + r * Iq - xd1 * Id;
			ExtEqe = Eqs - Id * (xd - xd1);
			Eq = Eqs - Id * (xd - xd1); // repeat eq calc after eqs (first eq calc is in processdiscontinuity)
			break;
		case eDEVICESTATE::DS_OFF:
			zsq = Id = Iq = Eq = 0.0;
			Vd = -V;
			Vq = V;
			Eq = 0.0;
			ExtEqe = 0.0;
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorDQBase::ProcessDiscontinuity(pDynaModel);
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

bool CDynaGenerator1C::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (bRes)
	{
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

		// P
		// Q
		// S
		// DELTA
		// EQS
		// V_D
		// V_Q
		// I_D
		// I_Q
		// EQ



		// dP/dP
		pDynaModel->SetElement(P, P, 1.0);
		// dP/dVd
		pDynaModel->SetElement(P, Vd, -Id);
		// dP/dVq
		pDynaModel->SetElement(P, Vq, -Iq);
		// dP/dId
		pDynaModel->SetElement(P, Id, -Vd);
		// dP/dIq
		pDynaModel->SetElement(P, Iq, -Vq);

		// dQ/dQ
		pDynaModel->SetElement(Q, Q, 1.0);
		// dQ/dVd
		pDynaModel->SetElement(Q, Vd, -Iq);
		// dQ/dVq
		pDynaModel->SetElement(Q, Vq, Id);
		// dQ/dId
		pDynaModel->SetElement(Q, Id, Vq);
		// dQ/dIq
		pDynaModel->SetElement(Q, Iq, -Vd);

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
		pDynaModel->SetElement(Eqs, Eqs, -1.0 / Td01);
		// dEqs/dId
		pDynaModel->SetElement(Eqs, Id, -1.0 / Td01 * (xd - xd1));
		// dEqs/dEqe
		if (ExtEqe.Indexed())
			pDynaModel->SetElement(Eqs, ExtEqe, -1.0 / Td01);

		
		// dDeltaG / dS
		pDynaModel->SetElement(Delta, s, -pDynaModel->GetOmega0());
		// dDeltaG / dDeltaG
		pDynaModel->SetElement(Delta , Delta, 1.0);
		

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

		bRes = bRes && BuildRIfromDQEquations(pDynaModel);

	}

	return true;
}


bool CDynaGenerator1C::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (bRes)
	{
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

		double Pairgap = P + (Id*Id + Iq*Iq) * r;

		pDynaModel->SetFunction(P, P - Vd * Id - Vq * Iq);
		pDynaModel->SetFunction(Q, Q - Vd * Iq + Vq * Id);
		pDynaModel->SetFunction(Id, Id - zsq * (-r * Vd - xq * (Eqs - Vq)));
		pDynaModel->SetFunction(Iq, Iq - zsq * (r * (Eqs - Vq) - xd1 * Vd));
		pDynaModel->SetFunction(Eq, Eq - Eqs + Id * (xd - xd1));
		double eDelta = pDynaModel->GetOmega0() * s;
		double eEqs = (ExtEqe - Eqs + Id * (xd - xd1)) / Td01;
		double eS = (Pt / sp1 - Kdemp  * s - Pairgap / sp2) / Mj;
		pDynaModel->SetFunctionDiff(s, eS);
		pDynaModel->SetFunctionDiff(Delta, eDelta);
		pDynaModel->SetFunctionDiff(Eqs, eEqs);

		bRes = bRes && BuildRIfromDQRightHand(pDynaModel);
	}

	return true;
}


bool CDynaGenerator1C::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (IsStateOn())
	{
		double NodeV = V;
		double DeltaGT = Delta - DeltaV;
		double cosDeltaGT = cos(DeltaGT);
		double sp1 = ZeroGuardSlip(1.0 + s) ;
		double sp2 = ZeroGuardSlip(1.0 + Sv);

		if (!IsStateOn())
		{
			NodeV = cosDeltaGT = 0.0;
			sp1 = sp2 = 1.0;
		}

		double Pairgap = P + (Id*Id + Iq*Iq) * r;
		double eDelta = pDynaModel->GetOmega0() * s;
		double eEqs = (ExtEqe - Eqs + Id * (xd - xd1)) / Td01;
		double eS = (Pt / sp1 - Kdemp  * s - Pairgap / sp2) / Mj;
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
	return true;
}

double* CDynaGenerator1C::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorDQBase::GetVariablePtr(nVarIndex);
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
	double xgen = Xgen();
	return m_Egen = cplx(Eqs - Id * (xgen - xd1), Iq * (xgen - xq)) * std::polar(1.0, (double)Delta);
}

bool CDynaGenerator1C::CalculatePower()
{

	double NodeV = V;
	double DeltaGT = Delta - DeltaV;
	double cosDeltaGT = cos(DeltaGT);
	double sinDeltaGT = sin(DeltaGT);
	double sp1 = ZeroGuardSlip(1.0 + s);
	double sp2 = ZeroGuardSlip(1.0 + Sv);

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

	return true;
}

void CDynaGenerator1C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorDQBase::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_1C);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGenerator1C, CDeviceContainerProperties::m_cszSysNameGenerator1C);
	props.nEquationsCount = CDynaGenerator1C::VARS::V_LAST;
	props.m_VarMap.insert(std::make_pair("Eqs", CVarIndex(CDynaGenerator1C::V_EQS, VARUNIT_KVOLTS)));
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
	Serializer->AddProperty("td01", Td01, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty("xd", xd, eVARUNITS::VARUNIT_OHM);
	// добавляем переменные состояния модели одноконтурной модели генератора в ЭДС
	Serializer->AddState("zsq", zsq, eVARUNITS::VARUNIT_PU);
}

