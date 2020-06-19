#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"


using namespace DFW2;

CDynaGenerator1C::CDynaGenerator1C() : CDynaGeneratorMotion()
{

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
			ExtEqe = Eqs - Id * (xd - xd1);
			Eq = Eqs - Id * (xd - xd1); // repeat eq calc after eqs (first eq calc is in processdiscontinuity)
			break;
		case DS_OFF:
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

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::ProcessDiscontinuity(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorMotion::ProcessDiscontinuity(pDynaModel);
	if (CDevice::IsFunctionStatusOK(eRes))
	{
		if (IsStateOn())
		{
			double DeltaGT = Delta - DeltaV;
			double NodeV = V;
			Vd = -NodeV * sin(DeltaGT);
			Vq = NodeV * cos(DeltaGT);
			double det = (Vd * Vd + Vq * Vq);
			Id = (P * Vd - Q * Vq) / det;
			Iq = (Q * Vd + P * Vq) / det;
			Eq = Eqs - Id * (xd - xd1);
			IfromDQ();
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
		double NodeV  =	V;
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

		bRes = bRes && BuildIfromDQEquations(pDynaModel);

	}

	return true;
}


bool CDynaGenerator1C::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (bRes)
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

		double Pairgap = P + (Id*Id + Iq*Iq) * r;
		pDynaModel->SetFunction(Vd, Vd + NodeV * sinDeltaGT);
		pDynaModel->SetFunction(Vq, Vq - NodeV * cosDeltaGT);
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

		bRes = bRes && BuildIfromDQRightHand(pDynaModel);
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
	double *p = CDynaGeneratorMotion::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Eqs.Value, V_EQS)
			MAP_VARIABLE(Vd.Value, V_VD)
			MAP_VARIABLE(Vq.Value, V_VQ)
			MAP_VARIABLE(Id.Value, V_ID)
			MAP_VARIABLE(Iq.Value, V_IQ)
			MAP_VARIABLE(Eq.Value, V_EQ)
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
			MAP_VARIABLE(*ExtEqe.m_pValue, C_EQE)
		}
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator1C::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorMotion::UpdateExternalVariables(pDynaModel);
	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);
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
		cplx Ig = (m_Egen - std::polar((double)V, (double)DeltaV)) * YgInt;
		cplx Idq = Ig * std::polar(1.0, -Delta);
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

const CDeviceContainerProperties CDynaGenerator1C::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaGeneratorMotion::DeviceProperties();
	props.SetType(DEVTYPE_GEN_1C);
	props.AddLinkTo(DEVTYPE_EXCITER, DLM_SINGLE, DPD_SLAVE, CDynaGenerator1C::m_cszExciterId);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGenerator1C, CDeviceContainerProperties::m_cszSysNameGenerator1C);

	props.nEquationsCount = CDynaGenerator1C::VARS::V_LAST;

	props.m_VarMap.insert(std::make_pair(_T("Eqs"), CVarIndex(CDynaGenerator1C::V_EQS, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair(CDynaGenerator1C::m_cszEq, CVarIndex(CDynaGenerator1C::V_EQ, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair(_T("Id"), CVarIndex(CDynaGenerator1C::V_ID, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair(_T("Iq"), CVarIndex(CDynaGenerator1C::V_IQ, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair(_T("Vd"), CVarIndex(CDynaGenerator1C::V_VD, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair(_T("Vq"), CVarIndex(CDynaGenerator1C::V_VQ, VARUNIT_KVOLTS)));

	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszExciterId, CConstVarIndex(CDynaGenerator1C::C_EXCITERID, eDVT_CONSTSOURCE)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszEqnom, CConstVarIndex(CDynaGenerator1C::C_EQNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszSnom, CConstVarIndex(CDynaGenerator1C::C_SNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszInom, CConstVarIndex(CDynaGenerator1C::C_INOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszQnom, CConstVarIndex(CDynaGenerator1C::C_QNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszEqe, CConstVarIndex(CDynaGenerator1C::C_EQE, eDVT_INTERNALCONST)));

	return props;
}

void CDynaGenerator1C::IfromDQ()
{
	double co(cos(Delta)), si(sin(Delta));

	Ire = Iq * co - Id * si;
	Iim = Iq * si + Id * co;
}


// вводит в матрицу блок уравнении для преобразования
// из dq в ri
bool CDynaGenerator1C::BuildIfromDQEquations(CDynaModel *pDynaModel)
{

	double co(cos(Delta)), si(sin(Delta));

	// dIre / dIre
	pDynaModel->SetElement(Ire, Ire, 1.0);
	// dIre / dId
	pDynaModel->SetElement(Ire, Id, si);
	// dIre / dIq
	pDynaModel->SetElement(Ire, Iq, -co);
	// dIre / dDeltaG
	pDynaModel->SetElement(Ire, Delta, Iq * si + Id * co);

	// dIim / dIim
	pDynaModel->SetElement(Iim, Iim, 1.0);
	// dIim / dId
	pDynaModel->SetElement(Iim, Id, -co);
	// dIim / dIq
	pDynaModel->SetElement(Iim, Iq, -si);
	// dIim / dDeltaG
	pDynaModel->SetElement(Iim, Delta, Id * si - Iq * co);

	return true;
}

// вводит в правую часть уравнения для преобразования 
// из dq в ri
bool CDynaGenerator1C::BuildIfromDQRightHand(CDynaModel *pDynaModel)
{
	double co(cos(Delta)), si(sin(Delta));
	pDynaModel->SetFunction(Ire, Ire - Iq * co + Id * si);
	pDynaModel->SetFunction(Iim, Iim - Iq * si - Id * co);

	return true;
}

VariableIndexVec CDynaGenerator1C::GetVariables()
{
	return VariableIndexVec{ P, Q, Ire, Iim, s, Delta, Vd, Vq, Id, Iq, Eqs, Eq };
}

void CDynaGenerator1C::UpdateSerializer(SerializerPtr& Serializer)
{
	CDynaGeneratorMotion::UpdateSerializer(Serializer);
	Serializer->AddState(_T("zsq"), zsq, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Egen"), m_Egen, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(_T("Vd"), Vd, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(_T("Vq"), Vq, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(_T("Ids"), Id, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState(_T("Iqs"), Iq, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState(m_cszEq, Eq, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszEqe, ExtEqe, eVARUNITS::VARUNIT_KVOLTS);

	Serializer->AddProperty(_T("Td0"), Td01, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(_T("xd"), xd, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(_T("r"), r, eVARUNITS::VARUNIT_OHM);

	Serializer->AddProperty(m_cszExciterId, m_ExciterId);

	Serializer->AddState(m_cszEqnom, Eqnom, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszSnom, Snom, eVARUNITS::VARUNIT_MVA);
	Serializer->AddState(m_cszQnom, Qnom, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddState(m_cszInom, Inom, eVARUNITS::VARUNIT_KAMPERES);
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
