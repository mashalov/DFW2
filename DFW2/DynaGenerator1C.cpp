#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"


using namespace DFW2;

CDynaGenerator1C::CDynaGenerator1C() : CDynaGeneratorMotion()
{
	ExtEqe.Value(&Eqe);
}

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::Init(CDynaModel* pDynaModel)
{

	if (fabs(xq) < 1E-7) xq = xd1; // place to validation !!!
	if (xd <= 0) xd = xd1;
	if (xq <= 0) xq = xd1;

	r = 0.0;		/// debug !!!!!

	if (Kgen > 1)
	{
		xd /= Kgen;
		r /= Kgen;
	}

	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorMotion::Init(pDynaModel);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		Snom = Equal(cosPhinom, 0.0) ? Pnom : Pnom / cosPhinom;
		Qnom = Snom * sqrt(1.0 - cosPhinom * cosPhinom);
		Inom = Snom / Unom / M_SQRT3;
		Eqnom = (Unom * Unom * (Unom * Unom + Qnom * (xd + xq)) + Snom * Snom * xd * xq) / (Unom * sqrt(Unom * Unom * (Unom * Unom + 2.0 * Qnom * xq) + Snom * Snom * xq * xq));

		switch (GetState())
		{
		case DS_ON:
			zsq = 1.0 / (r*r + xd1 * xq);
			CDynaGenerator1C::ProcessDiscontinuity(pDynaModel);
			Eqs = Vq + r * Iq - xd1 * Id;
			Eqe = Eqs - Id * (xd - xd1);
			Eq = Eqs - Id * (xd - xd1); // repeat eq calc after eqs (first eq calc is in processdiscontinuity)
			break;
		case DS_OFF:
			zsq = Id = Iq = Eq = 0.0;
			Vd = -V.Value();
			Vq = V.Value();
			Eq = 0.0;
			Eqe = 0.0;
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::ProcessDiscontinuity(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorMotion::ProcessDiscontinuity(pDynaModel);
	if (CDevice::IsFunctionStatusOK(eRes))
	{
		if (IsStateOn())
		{
			double DeltaGT = Delta - DeltaV.Value();
			double NodeV = V.Value();
			Vd = -NodeV * sin(DeltaGT);
			Vq = NodeV * cos(DeltaGT);
			double det = (Vd * Vd + Vq * Vq);
			Id = (P * Vd - Q * Vq) / det;
			Iq = (Q * Vd + P * Vq) / det;
			Eq = Eqs - Id * (xd - xd1);
		}
		else
		{
			Id = Iq = Eq = 0.0;
		}
	}
	return eRes;
}

bool CDynaGenerator1C::BuildEquations(CDynaModel *pDynaModel)
{
	if (!pDynaModel->Status())
		return pDynaModel->Status();

	bool bRes = true;

	if (bRes)
	{
		double NodeV  =	V.Value();
		double DeltaGT = Delta - DeltaV.Value();

		double cosDeltaGT = cos(DeltaGT);
		double sinDeltaGT = sin(DeltaGT);

		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv.Value());

		if (!IsStateOn())
		{
			NodeV = cosDeltaGT = sinDeltaGT = 0.0;
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
		pDynaModel->SetElement(A(V_ID), A(V_VQ), -xq * zsq);
		// dId/dEqs
		pDynaModel->SetElement(A(V_ID), A(V_EQS), xq * zsq);

		// dIq/dIq
		pDynaModel->SetElement(A(V_IQ), A(V_IQ), 1);
		// dIq/dVd
		pDynaModel->SetElement(A(V_IQ), A(V_VD), xd1 * zsq);
		// dIq/dVq
		pDynaModel->SetElement(A(V_IQ), A(V_VQ), r * zsq);
		// dIq/dEqs
		pDynaModel->SetElement(A(V_IQ), A(V_EQS),  -r * zsq);

		// dEqs/dEqs
		pDynaModel->SetElement(A(V_EQS), A(V_EQS), -1.0 / Td01);
		// dEqs/dId
		pDynaModel->SetElement(A(V_EQS), A(V_ID), -1.0 / Td01 * (xd - xd1));
		// dEqs/dEqe
		if (ExtEqe.Indexed())
			pDynaModel->SetElement(A(V_EQS), A(ExtEqe.Index()), -1.0 / Td01);

		
		// dDeltaG / dS
		pDynaModel->SetElement(A(V_DELTA), A(V_S), -pDynaModel->GetOmega0());
		// dDeltaG / dDeltaG
		pDynaModel->SetElement(A(V_DELTA), A(V_DELTA), 1.0);
		

		// dS / dS
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0 / Mj * (-Kdemp - Pt / sp1 / sp1) );
		double Pairgap = P + (Id*Id + Iq*Iq) * r;
		// dS / dNodeS
		pDynaModel->SetElement(A(V_S), A(Sv.Index()), -1.0 / Mj * Pairgap / sp2 / sp2);
		// dS / Vd
		pDynaModel->SetElement(A(V_S), A(V_VD),  1.0 / Mj * Id / sp2);
		// dS / Vq
		pDynaModel->SetElement(A(V_S), A(V_VQ), 1.0 / Mj * Iq / sp2);
		// dS / Id
		pDynaModel->SetElement(A(V_S), A(V_ID), 1.0 / Mj * (Vd + 2 * Id * r) / sp2);
		// dS / Iq
		pDynaModel->SetElement(A(V_S), A(V_IQ), 1.0 / Mj * (Vq + 2 * Iq * r) / sp2);

		// dEq / dEq
		pDynaModel->SetElement(A(V_EQ), A(V_EQ), 1.0);
		// dEq / dEqs
		pDynaModel->SetElement(A(V_EQ), A(V_EQS), -1.0);
		// dEq / dId
		pDynaModel->SetElement(A(V_EQ), A(V_ID), xd - xd1);
	}

	return pDynaModel->Status() && bRes;
}


bool CDynaGenerator1C::BuildRightHand(CDynaModel *pDynaModel)
{
	if (!pDynaModel->Status())
		return pDynaModel->Status();

	bool bRes = true;

	if (bRes)
	{
		double NodeV = V.Value();
		double DeltaGT = Delta - DeltaV.Value();
		double cosDeltaGT = cos(DeltaGT);
		double sinDeltaGT = sin(DeltaGT);
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv.Value());

		if (!IsStateOn())
		{
			NodeV = cosDeltaGT = sinDeltaGT = 0.0;
			sp1 = sp2 = 1.0;
		}

		double Pairgap = P + (Id*Id + Iq*Iq) * r;
		pDynaModel->SetFunction(A(V_VD), Vd + NodeV * sinDeltaGT);
		pDynaModel->SetFunction(A(V_VQ), Vq - NodeV * cosDeltaGT);
		pDynaModel->SetFunction(A(V_P), P - Vd * Id - Vq * Iq);
		pDynaModel->SetFunction(A(V_Q), Q - Vd * Iq + Vq * Id);
		pDynaModel->SetFunction(A(V_ID), Id - zsq * (-r * Vd - xq * (Eqs - Vq)));
		pDynaModel->SetFunction(A(V_IQ), Iq - zsq * (r * (Eqs - Vq) - xd1 * Vd));
		pDynaModel->SetFunction(A(V_EQ), Eq - Eqs + Id * (xd - xd1));
		double eDelta = pDynaModel->GetOmega0() * s;
		double eEqs = (ExtEqe.Value() - Eqs + Id * (xd - xd1)) / Td01;
		double eS = (Pt / sp1 - Kdemp  * s - Pairgap / sp2) / Mj;
		pDynaModel->SetFunctionDiff(A(V_S), eS);
		pDynaModel->SetFunctionDiff(A(V_DELTA), eDelta);
		pDynaModel->SetFunctionDiff(A(V_EQS), eEqs);
	}

	return pDynaModel->Status() && bRes;
}


bool CDynaGenerator1C::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (IsStateOn())
	{
		double NodeV = V.Value();
		double DeltaGT = Delta - DeltaV.Value();
		double cosDeltaGT = cos(DeltaGT);
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv.Value());

		if (!IsStateOn())
		{
			NodeV = cosDeltaGT = 0.0;
			sp1 = sp2 = 1.0;
		}

		double Pairgap = P + (Id*Id + Iq*Iq) * r;
		double eDelta = pDynaModel->GetOmega0() * s;
		double eEqs = (ExtEqe.Value() - Eqs + Id * (xd - xd1)) / Td01;
		double eS = (Pt / sp1 - Kdemp  * s - Pairgap / sp2) / Mj;
		pDynaModel->SetDerivative(A(V_S), eS);
		pDynaModel->SetDerivative(A(V_DELTA), eDelta);
		pDynaModel->SetDerivative(A(V_EQS), eEqs);
	}
	else
	{
		pDynaModel->SetDerivative(A(V_S), 0.0);
		pDynaModel->SetDerivative(A(V_DELTA), 0.0);
		pDynaModel->SetDerivative(A(V_EQS), 0.0);
	}
	return pDynaModel->Status();
}

double* CDynaGenerator1C::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorMotion::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Eqs, V_EQS)
			MAP_VARIABLE(Vd, V_VD)
			MAP_VARIABLE(Vq, V_VQ)
			MAP_VARIABLE(Id, V_ID)
			MAP_VARIABLE(Iq, V_IQ)
			MAP_VARIABLE(Eq, V_EQ)
		}
	}
	return p;
}


double* CDynaGenerator1C::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorMotion::GetConstVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(m_ExciterId, C_EXCITERID)
			MAP_VARIABLE(Eqnom, C_EQNOM)
			MAP_VARIABLE(Snom, C_SNOM)
			MAP_VARIABLE(Qnom, C_QNOM)
			MAP_VARIABLE(Inom, C_INOM)
			MAP_VARIABLE(Eqe, C_EQE)
		}
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorMotion::UpdateExternalVariables(pDynaModel);
	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);
	if (pExciter)
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtEqe, pExciter, m_cszEqe));
	return eRes;
}

double CDynaGenerator1C::Xgen()
{
	return 0.5 * (xq + xd1);
}

cplx CDynaGenerator1C::Igen(ptrdiff_t nIteration)
{
	cplx YgInt = 1.0 / cplx(0.0, Xgen());

	if (!nIteration)
		m_Egen = GetEMF();
	else
	{
		cplx Ig = (m_Egen - polar(V.Value(), DeltaV.Value())) * YgInt;
		cplx Idq = Ig * polar(1.0, -Delta);
		Id = Idq.imag();
		Iq = Idq.real();
	}
	
	//double Id0 = Id, Iq0 = Iq;
	cplx Ig = CalculateEgen() * YgInt;
	//Id = Id0; Iq = Iq0;
	return Ig;
}

const cplx& CDynaGenerator1C::CalculateEgen()
{
	double xgen = Xgen();
	return m_Egen = cplx(Eqs - Id * (xgen - xd1), Iq * (xgen - xq)) * polar(1.0, Delta);
}

bool CDynaGenerator1C::CalculatePower()
{

	double NodeV = V.Value();
	double DeltaGT = Delta - DeltaV.Value();
	double cosDeltaGT = cos(DeltaGT);
	double sinDeltaGT = sin(DeltaGT);
	double sp1 = ZeroGuardSlip(1.0 + s);
	double sp2 = ZeroGuardSlip(1.0 + Sv.Value());

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

const CDeviceContainerProperties CDynaGenerator1C::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaGeneratorMotion::DeviceProperties();
	props.SetType(DEVTYPE_GEN_1C);
	props.AddLinkTo(DEVTYPE_EXCITER, DLM_SINGLE, DPD_SLAVE, CDynaGenerator1C::m_cszExciterId);
	props.m_strClassName = CDeviceContainerProperties::m_cszNameGenerator1C;

	props.nEquationsCount = CDynaGenerator1C::VARS::V_LAST;

	props.m_VarMap.insert(make_pair(CDynaGenerator1C::m_cszId, CVarIndex(CDynaGenerator1C::V_ID, false, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(CDynaGenerator1C::m_cszIq, CVarIndex(CDynaGenerator1C::V_IQ, false, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("Eqs"), CVarIndex(CDynaGenerator1C::V_EQS, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(make_pair(CDynaGenerator1C::m_cszEq, CVarIndex(CDynaGenerator1C::V_EQ, VARUNIT_KVOLTS)));

	props.m_ConstVarMap.insert(make_pair(CDynaGenerator1C::m_cszExciterId, CConstVarIndex(CDynaGenerator1C::C_EXCITERID, eDVT_CONSTSOURCE)));
	props.m_ConstVarMap.insert(make_pair(CDynaGenerator1C::m_cszEqnom, CConstVarIndex(CDynaGenerator1C::C_EQNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(make_pair(CDynaGenerator1C::m_cszSnom, CConstVarIndex(CDynaGenerator1C::C_SNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(make_pair(CDynaGenerator1C::m_cszInom, CConstVarIndex(CDynaGenerator1C::C_INOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(make_pair(CDynaGenerator1C::m_cszQnom, CConstVarIndex(CDynaGenerator1C::C_QNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(make_pair(CDynaGenerator1C::m_cszEqe, CConstVarIndex(CDynaGenerator1C::C_EQE, eDVT_INTERNALCONST)));

	return props;
}


const _TCHAR *CDynaGenerator1C::m_cszEqe = _T("Eqe");
const _TCHAR *CDynaGenerator1C::m_cszEq  = _T("Eq");
const _TCHAR *CDynaGenerator1C::m_cszId = _T("Id");
const _TCHAR *CDynaGenerator1C::m_cszIq = _T("Iq");
const _TCHAR *CDynaGenerator1C::m_cszExciterId = _T("ExciterId");
const _TCHAR *CDynaGenerator1C::m_cszEqnom = _T("Eqnom");
const _TCHAR *CDynaGenerator1C::m_cszSnom = _T("Snom");
const _TCHAR *CDynaGenerator1C::m_cszInom = _T("Inom");
const _TCHAR *CDynaGenerator1C::m_cszQnom = _T("Qnom");
