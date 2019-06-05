#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"
#include "DynaGenerator3C.h"

using namespace DFW2;

CDynaGenerator3C::CDynaGenerator3C() : CDynaGenerator1C()
{

}

eDEVICEFUNCTIONSTATUS CDynaGenerator3C::Init(CDynaModel* pDynaModel)
{
	xq1 = xq;

	if (!pDynaModel->ConsiderDampingEquation())
		Kdemp = 0.0;


	if (Kgen > 1)
	{
		xd2 /= Kgen;
		xq2 /= Kgen;
		xq1 /= Kgen;
	}

	eDEVICEFUNCTIONSTATUS Status = CDynaGenerator1C::Init(pDynaModel);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		switch (GetState())
		{
		case DS_ON:
			zsq = 1.0 / (r * r + xd2 * xq2);
			Edss = Vd + r * Id + xq2 * Iq;
			Eqss = Vq - xd2 * Id + r * Iq;
			Eqs = Eqss - Id * (xd1 - xd2);
			Eqe = Eqs - Id * (xd - xd1);
			Eq = Eqss - Id * (xd - xd2);
			break;
		case DS_OFF:
			zsq = Id = Iq = Eqss = Eqs = Edss = Eq = 0.0;
			break;
		}
	}

	return Status;
}

bool CDynaGenerator3C::BuildEquations(CDynaModel *pDynaModel)
{
	if (!pDynaModel->Status())
		return pDynaModel->Status();

	bool bRes = true;

	if (GetId() == 376804 && !pDynaModel->EstimateBuild())
		bRes = bRes;

	if (bRes)
	{
		double NodeV = V.Value();
		double DeltaGT = Delta - DeltaV.Value();

		double cosDeltaGT = cos(DeltaGT);
		double sinDeltaGT = sin(DeltaGT);

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

		// dP/dP
		pDynaModel->SetElement(A(V_P), A(V_P), 1.0);
		// dP/dVd
		pDynaModel->SetElement(A(V_P), A(V_VD), -Id);
		// dP/dVq
		pDynaModel->SetElement(A(V_P), A(V_VQ), -Iq);
		// dP/dId
		pDynaModel->SetElement(A(V_P), A(V_ID), -Vd);
		// dP/dIq
		pDynaModel->SetElement(A(V_P), A(V_IQ), -Vq);

		// dQ/dQ
		pDynaModel->SetElement(A(V_Q), A(V_Q), 1.0);
		// dQ/dVd
		pDynaModel->SetElement(A(V_Q), A(V_VD), -Iq);
		// dQ/dVq
		pDynaModel->SetElement(A(V_Q), A(V_VQ), Id);
		// dQ/dId
		pDynaModel->SetElement(A(V_Q), A(V_ID), Vq);
		// dQ/dIq
		pDynaModel->SetElement(A(V_Q), A(V_IQ), -Vd);

		// dVd/dVd
		pDynaModel->SetElement(A(V_VD), A(V_VD), 1);
		// dVd/dV
		pDynaModel->SetElement(A(V_VD), A(V.Index()), sinDeltaGT);
		// dVd/dDeltaV
		pDynaModel->SetElement(A(V_VD), A(DeltaV.Index()), -NodeV * cosDeltaGT);
		// dVd/dDeltaG
		pDynaModel->SetElement(A(V_VD), A(V_DELTA), NodeV * cosDeltaGT);

		// dVq/dVq
		pDynaModel->SetElement(A(V_VQ), A(V_VQ), 1);
		// dVq/dV
		pDynaModel->SetElement(A(V_VQ), A(V.Index()), -cosDeltaGT);
		// dVq/dDeltaV
		pDynaModel->SetElement(A(V_VQ), A(DeltaV.Index()), -NodeV * sinDeltaGT);
		// dVq/dDeltaG
		pDynaModel->SetElement(A(V_VQ), A(V_DELTA), NodeV * sinDeltaGT);

		// dId/dId
		pDynaModel->SetElement(A(V_ID), A(V_ID), 1);
		// dId/dVd
		pDynaModel->SetElement(A(V_ID), A(V_VD), r * zsq);
		// dId/dVq
		pDynaModel->SetElement(A(V_ID), A(V_VQ), -xq2 * zsq);
		// dId/dEdss
		pDynaModel->SetElement(A(V_ID), A(V_EDSS), -r * zsq);
		// dId/dEqss
		pDynaModel->SetElement(A(V_ID), A(V_EQSS), xq2 * zsq);

		// dIq/dIq
		pDynaModel->SetElement(A(V_IQ), A(V_IQ), 1);
		// dIq/dVd
		pDynaModel->SetElement(A(V_IQ), A(V_VD), xd2 * zsq);
		// dIq/dVq
		pDynaModel->SetElement(A(V_IQ), A(V_VQ), r * zsq);
		// dIq/dEdss
		pDynaModel->SetElement(A(V_IQ), A(V_EDSS), -xd2 * zsq);
		// dIq/dEqss
		pDynaModel->SetElement(A(V_IQ), A(V_EQSS), -r * zsq);

		// dEqs/dEqs
		pDynaModel->SetElement(A(V_EQS), A(V_EQS), 1.0 / Td01);
		// dEqs/dId
		pDynaModel->SetElement(A(V_EQS), A(V_ID), -1.0 / Td01 * (xd - xd1));
		// dEqs/dEqe
		if (ExtEqe.Indexed())
			pDynaModel->SetElement(A(V_EQS), A(ExtEqe.Index()), -1.0 / Td01);


		// dDeltaG / dS
		pDynaModel->SetElement(A(V_DELTA), A(V_S), -pDynaModel->GetOmega0());
		// dDeltaG / dDeltaG
		pDynaModel->SetElement(A(V_DELTA), A(V_DELTA), 1.0);

		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv.Value());
		// dS / dS
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0 / Mj * (-Kdemp - Pt / sp1 / sp1));
		double Pairgap = P + (Id*Id + Iq*Iq) * r;
		// dS / dNodeS
		pDynaModel->SetElement(A(V_S), A(Sv.Index()), -1.0 / Mj * Pairgap / sp2 / sp2);
		// dS / Vd
		pDynaModel->SetElement(A(V_S), A(V_VD), 1.0 / Mj * Id / sp2);
		// dS / Vq
		pDynaModel->SetElement(A(V_S), A(V_VQ), 1.0 / Mj * Iq / sp2);
		// dS / Id
		pDynaModel->SetElement(A(V_S), A(V_ID), 1.0 / Mj * (Vd + 2 * Id * r) / sp2);
		// dS / Iq
		pDynaModel->SetElement(A(V_S), A(V_IQ), 1.0 / Mj * (Vq + 2 * Iq * r) / sp2);
		
		// dEqss / dEqss
		pDynaModel->SetElement(A(V_EQSS), A(V_EQSS), -1.0 / Td0ss);
		// dEqss / dEqs
		pDynaModel->SetElement(A(V_EQSS), A(V_EQS), - 1.0 / Td0ss);
		// dEqss / dId
		pDynaModel->SetElement(A(V_EQSS), A(V_ID), -(xd1 - xd2) / Td0ss);

		// dEdss / dEdss
		pDynaModel->SetElement(A(V_EDSS), A(V_EDSS), -1.0 / Tq0ss);
		// dEdss / dIq
		pDynaModel->SetElement(A(V_EDSS), A(V_IQ), (xq1 - xq2) / Tq0ss);


		// dEq / dEq
		pDynaModel->SetElement(A(V_EQ), A(V_EQ), 1.0);
		// dEq / dEqss
		pDynaModel->SetElement(A(V_EQ), A(V_EQSS), -1.0);
		// dEq / dId
		pDynaModel->SetElement(A(V_EQ), A(V_ID), xd - xd2);
	}

	return pDynaModel->Status() && bRes;
}


bool CDynaGenerator3C::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (bRes)
	{
		double NodeV = V.Value();
		double DeltaGT = Delta - DeltaV.Value();

		double cosDeltaGT = cos(DeltaGT);
		double sinDeltaGT = sin(DeltaGT);

		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv.Value());
		
		double Pairgap = P + (Id*Id + Iq*Iq) * r;
		
		pDynaModel->SetFunction(A(V_VD), Vd + NodeV * sinDeltaGT);
		pDynaModel->SetFunction(A(V_VQ), Vq - NodeV * cosDeltaGT);
		pDynaModel->SetFunction(A(V_P), P - Vd * Id - Vq * Iq);
		pDynaModel->SetFunction(A(V_Q), Q - Vd * Iq + Vq * Id);
		pDynaModel->SetFunction(A(V_ID), Id + zsq * (r * (Vd - Edss) + xq2 * (Eqss - Vq)));
		pDynaModel->SetFunction(A(V_IQ), Iq + zsq * (r * (Vq - Eqss) + xd2 * (Vd - Edss)));
		pDynaModel->SetFunction(A(V_EQ), Eq - Eqss + Id * (xd - xd2));


		double eDelta = pDynaModel->GetOmega0() * s;
		double eEqs = (ExtEqe.Value() - Eqs + Id * (xd - xd1)) / Td01;
		double eS = (Pt / sp1 - Kdemp  * s - Pairgap / sp2) / Mj;

		double eEqss = (Eqs - Eqss + Id * (xd1 - xd2)) / Td0ss;
		double eEdss = (-Edss - Iq * (xq1 - xq2)) / Tq0ss;

		pDynaModel->SetFunctionDiff(A(V_S), eS);
		pDynaModel->SetFunctionDiff(A(V_DELTA), eDelta);
		pDynaModel->SetFunctionDiff(A(V_EQS), eEqs);
		pDynaModel->SetFunctionDiff(A(V_EQSS), eEqss);
		pDynaModel->SetFunctionDiff(A(V_EDSS), eEdss);
	}

	return pDynaModel->Status() && bRes;
}


bool CDynaGenerator3C::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = CDynaGenerator1C::BuildDerivatives(pDynaModel);
	if (bRes)
	{
		if (IsStateOn())
		{
			double eEqss = (Eqs - Eqss + Id * (xd1 - xd2)) / Td0ss;
			pDynaModel->SetDerivative(A(V_EQSS), eEqss);
			double eEdss = (-Edss - Iq * (xq1 - xq2)) / Tq0ss;
			pDynaModel->SetDerivative(A(V_EDSS), eEdss);
		}
		else
		{
			pDynaModel->SetDerivative(A(V_EQSS), 0.0);
			pDynaModel->SetDerivative(A(V_EDSS), 0.0);
		}
	}
	return pDynaModel->Status();
}



double* CDynaGenerator3C::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGenerator1C::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Eqss, V_EQSS)
			MAP_VARIABLE(Edss, V_EDSS)
		}
	}
	return p;
}


double* CDynaGenerator3C::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGenerator1C::GetConstVariablePtr(nVarIndex);
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator3C::ProcessDiscontinuity(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGenerator1C::ProcessDiscontinuity(pDynaModel);
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
	
	double NodeV = V.Value();
	double DeltaGT = Delta - DeltaV.Value();

	double cosDeltaGT = cos(DeltaGT);
	double sinDeltaGT = sin(DeltaGT);

	double sp1 = ZeroGuardSlip(1.0 + s);
	double sp2 = ZeroGuardSlip(1.0 + Sv.Value());

	double Pairgap = P + (Id*Id + Iq * Iq) * r;

	Vd = -NodeV * sinDeltaGT;
	Vq = NodeV * cosDeltaGT;
	Id = -zsq * (r * (Vd - Edss) + xq2 * (Eqss - Vq));
	Iq = -zsq * (r * (Vq - Eqss) + xd2 * (Vd - Edss));
	P = Vd * Id + Vq * Iq;
	Q = Vd * Iq - Vq * Id;

	return true;
}

const cplx& CDynaGenerator3C::CalculateEgen()
{
	double xgen = Xgen();
	return m_Egen = cplx(Eqss - Id * (xgen - xd2), Edss + Iq*(xgen - xq2)) * polar(1.0, Delta);
}

double CDynaGenerator3C::Xgen()
{
	return 0.5 * (xd2 + xq2);
}

const CDeviceContainerProperties CDynaGenerator3C::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaGenerator1C::DeviceProperties();
	props.SetType(DEVTYPE_GEN_3C);
	props.m_strClassName = CDeviceContainerProperties::m_cszNameGenerator3C;
	props.nEquationsCount = CDynaGenerator3C::VARS::V_LAST;
	props.m_VarMap.insert(make_pair(_T("Eqss"), CVarIndex(CDynaGenerator3C::V_EQSS, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(make_pair(_T("Edss"), CVarIndex(CDynaGenerator3C::V_EDSS, VARUNIT_KVOLTS)));

	return props;
}