#include "stdafx.h"
#include "DynaGeneratorMotion.h"
#include "DynaModel.h"


using namespace DFW2;

CDynaGeneratorMotion::CDynaGeneratorMotion() : CDynaGeneratorInfBusBase()
{

}


double* CDynaGeneratorMotion::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorInfBusBase::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Delta, V_DELTA)
			MAP_VARIABLE(s, V_S)
		}
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::Init(CDynaModel* pDynaModel)
{
	if (Kgen > 1)
	{
		Pnom *= Kgen;
		xq /= Kgen;
		Mj *= Kgen;
	}

	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorInfBusBase::Init(pDynaModel);
	
	if (CDevice::IsFunctionStatusOK(Status))
	{
		if (Mj < 1E-7)
			Mj = 3 * Pnom;

		Kdemp *= Pnom;
		Pt = P;
		s = 0;
		Status = DFS_OK;
	}

	return Status;
}


bool CDynaGeneratorMotion::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{

		double NodeSv = Sv.Value();
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + NodeSv);

		double dVre(Vre.Value()), dVim(Vim.Value());
		ptrdiff_t iVre(Vre.Index()), iVim(Vim.Index());

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

		// dP / dP
		pDynaModel->SetElement(A(V_P), A(V_P), 1.0);
		// dP / dVre
		pDynaModel->SetElement(A(V_P), A(iVre), -Ire);
		// dP / dVim
		pDynaModel->SetElement(A(V_P), A(iVim), -Iim);
		// dP / dIre
		pDynaModel->SetElement(A(V_P), A(V_IRE), -dVre);
		// dP / dIim
		pDynaModel->SetElement(A(V_P), A(V_IIM), -dVim);

		// dQ / dP
		pDynaModel->SetElement(A(V_Q), A(V_Q), 1.0);
		// dQ / dVre
		pDynaModel->SetElement(A(V_Q), A(iVre), Iim);
		// dQ / dVim
		pDynaModel->SetElement(A(V_Q), A(iVim), -Ire);
		// dQ / dIre
		pDynaModel->SetElement(A(V_Q), A(V_IRE), -dVim);
		// dQ / dIim
		pDynaModel->SetElement(A(V_Q), A(V_IIM), dVre);

		// dDeltaG / dS
		pDynaModel->SetElement(A(V_DELTA), A(V_S), -pDynaModel->GetOmega0());
		// dDeltaG / dDeltaG
		pDynaModel->SetElement(A(V_DELTA), A(V_DELTA), 1.0);

		// dS / dS
		pDynaModel->SetElement(A(V_S), A(V_S), - 1.0 / Mj * (-Kdemp - Pt / sp1 / sp1));
		// dS / dP
		pDynaModel->SetElement(A(V_S), A(V_P), 1.0 / Mj / sp2);
		// dS / dNodeS
		pDynaModel->SetElement(A(V_S), A(Sv.Index()), -1.0 / Mj * P / sp2 / sp2);

		// dIre / dIre
		pDynaModel->SetElement(A(V_IRE), A(V_IRE), 1.0);
		// dIre / dVim
		pDynaModel->SetElement(A(V_IRE), A(Vim.Index()), 1.0 / xd1);
		// dIre / dDeltaG
		pDynaModel->SetElement(A(V_IRE), A(V_DELTA), -Eqs * cos(Delta) / xd1);

		// dIim / dIim
		pDynaModel->SetElement(A(V_IIM), A(V_IIM), 1.0);
		// dIim / dVre
		pDynaModel->SetElement(A(V_IIM), A(Vre.Index()), -1.0 / xd1);
		// dIim / dDeltaG
		pDynaModel->SetElement(A(V_IIM), A(V_DELTA), -Eqs * sin(Delta) / xd1);

	}
	return true;
}


bool CDynaGeneratorMotion::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{
		double NodeSv = Sv.Value();
		double dVre(Vre.Value()), dVim(Vim.Value());
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + NodeSv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

		pDynaModel->SetFunction(A(V_P), P - dVre * Ire - dVim * Iim);
		pDynaModel->SetFunction(A(V_Q), Q + dVre * Iim - dVim * Ire);
		pDynaModel->SetFunction(A(V_IRE), Ire - (Eqs * sin(Delta) - dVim) / xd1);
		pDynaModel->SetFunction(A(V_IIM), Iim - (dVre - Eqs * cos(Delta)) / xd1);

		double eDelta = pDynaModel->GetOmega0() * s;
		double eS = (Pt / sp1 - Kdemp  * s - P / sp2) / Mj;
		pDynaModel->SetFunctionDiff(A(V_S), eS);
		pDynaModel->SetFunctionDiff(A(V_DELTA), eDelta);
	}

	return true;
}


bool CDynaGeneratorMotion::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{
		double sp1 = 1.0 + s;
		double sp2 = 1.0 + Sv.Value();

		if (!IsStateOn())
			sp1 = sp2 = 1.0;

		double eDelta = pDynaModel->GetOmega0() * s;
		double eS = (Pt / sp1 - Kdemp  * s - P / sp2) / Mj;

		pDynaModel->SetDerivative(A(V_S), eS);
		pDynaModel->SetDerivative(A(V_DELTA), eDelta);
	}
	return true;
}

double* CDynaGeneratorMotion::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorInfBusBase::GetConstVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Unom, C_UNOM)
		}
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaGeneratorInfBusBase::UpdateExternalVariables(pDynaModel);
}


void CDynaGeneratorMotion::UpdateSerializer(SerializerPtr& Serializer)
{
	CDynaGeneratorInfBusBase::UpdateSerializer(Serializer);
	Serializer->AddProperty(_T("Kdemp"), Kdemp, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty(_T("xq"), xq, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(_T("Mj"), Mj, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Pnom"), Pnom, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty(_T("Unom"), Unom, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty(_T("cosPhinom"), cosPhinom, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Pt"), Pt, eVARUNITS::VARUNIT_MW);
	Serializer->AddState(_T("s"), s, eVARUNITS::VARUNIT_PU);
}

const CDeviceContainerProperties CDynaGeneratorMotion::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaGeneratorInfBusBase::DeviceProperties();
	props.SetType(DEVTYPE_GEN_MOTION);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorMotion, CDeviceContainerProperties::m_cszSysNameGeneratorMotion);
	props.nEquationsCount = CDynaGeneratorMotion::VARS::V_LAST;

	props.m_VarMap.insert(make_pair(_T("S"), CVarIndex(CDynaGeneratorMotion::V_S, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszDelta, CVarIndex(CDynaGeneratorMotion::V_DELTA, VARUNIT_RADIANS)));

	props.m_ConstVarMap.insert(make_pair(CDynaGeneratorMotion::m_cszUnom, CConstVarIndex(CDynaGeneratorMotion::C_UNOM, eDVT_CONSTSOURCE)));
	return props;
}

const _TCHAR *CDynaGeneratorMotion::m_cszUnom = _T("Unom");