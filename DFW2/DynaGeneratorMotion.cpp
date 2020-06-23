#include "stdafx.h"
#include "DynaGeneratorMotion.h"
#include "DynaModel.h"


using namespace DFW2;

CDynaGeneratorMotion::CDynaGeneratorMotion() : CDynaGeneratorInfBusBase()
{

}


VariableIndexRefVec& CDynaGeneratorMotion::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorInfBusBase::GetVariables(JoinVariables({ s, Delta },ChildVec));
}

double* CDynaGeneratorMotion::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorInfBusBase::GetVariablePtr(nVarIndex);
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
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
	}

	return Status;
}


bool CDynaGeneratorMotion::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{

		double NodeSv = Sv;
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + NodeSv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

		// dP / dP
		pDynaModel->SetElement(P, P, 1.0);
		// dP / dVre
		pDynaModel->SetElement(P, Vre, -Ire);
		// dP / dVim
		pDynaModel->SetElement(P, Vim, -Iim);
		// dP / dIre
		pDynaModel->SetElement(P, Ire, -Vre);
		// dP / dIim
		pDynaModel->SetElement(P, Iim, -Vim);

		// dQ / dP
		pDynaModel->SetElement(Q, Q, 1.0);
		// dQ / dVre
		pDynaModel->SetElement(Q, Vre, Iim);
		// dQ / dVim
		pDynaModel->SetElement(Q, Vim, -Ire);
		// dQ / dIre
		pDynaModel->SetElement(Q, Ire, -Vim);
		// dQ / dIim
		pDynaModel->SetElement(Q, Iim, Vre);

		// dDeltaG / dS
		pDynaModel->SetElement(Delta, s, -pDynaModel->GetOmega0());
		// dDeltaG / dDeltaG
		pDynaModel->SetElement(Delta, Delta, 1.0);

		// dS / dS
		pDynaModel->SetElement(s, s, - 1.0 / Mj * (-Kdemp - Pt / sp1 / sp1));
		// dS / dP
		pDynaModel->SetElement(s, P, 1.0 / Mj / sp2);
		// dS / dNodeS
		pDynaModel->SetElement(s, Sv, -1.0 / Mj * P / sp2 / sp2);

		// dIre / dIre
		pDynaModel->SetElement(Ire, Ire, 1.0);
		// dIre / dVim
		pDynaModel->SetElement(Ire, Vim, 1.0 / xd1);
		// dIre / dDeltaG
		pDynaModel->SetElement(Ire, Delta, -Eqs * cos(Delta) / xd1);

		// dIim / dIim
		pDynaModel->SetElement(Iim, Iim, 1.0);
		// dIim / dVre
		pDynaModel->SetElement(Iim, Vre, -1.0 / xd1);
		// dIim / dDeltaG
		pDynaModel->SetElement(Iim, Delta, -Eqs * sin(Delta) / xd1);

	}
	return true;
}


bool CDynaGeneratorMotion::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{
		double NodeSv = Sv;
		double dVre(Vre), dVim(Vim);
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + NodeSv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

		pDynaModel->SetFunction(P, P - dVre * Ire - dVim * Iim);
		pDynaModel->SetFunction(Q, Q + dVre * Iim - dVim * Ire);
		pDynaModel->SetFunction(Ire, Ire - (Eqs * sin(Delta) - dVim) / xd1);
		pDynaModel->SetFunction(Iim, Iim - (dVre - Eqs * cos(Delta)) / xd1);

		double eDelta = pDynaModel->GetOmega0() * s;
		double eS = (Pt / sp1 - Kdemp  * s - P / sp2) / Mj;
		pDynaModel->SetFunctionDiff(s, eS);
		pDynaModel->SetFunctionDiff(Delta, eDelta);
	}

	return true;
}


bool CDynaGeneratorMotion::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{
		double sp1 = 1.0 + s;
		double sp2 = 1.0 + Sv;

		if (!IsStateOn())
			sp1 = sp2 = 1.0;

		double eDelta = pDynaModel->GetOmega0() * s;
		double eS = (Pt / sp1 - Kdemp  * s - P / sp2) / Mj;

		pDynaModel->SetDerivative(s, eS);
		pDynaModel->SetDerivative(Delta , eDelta);
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

	props.m_VarMap.insert(std::make_pair(_T("S"), CVarIndex(CDynaGeneratorMotion::V_S, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(CDynaNodeBase::m_cszDelta, CVarIndex(CDynaGeneratorMotion::V_DELTA, VARUNIT_RADIANS)));

	props.m_ConstVarMap.insert(std::make_pair(CDynaGeneratorMotion::m_cszUnom, CConstVarIndex(CDynaGeneratorMotion::C_UNOM, eDVT_CONSTSOURCE)));
	return props;
}

const _TCHAR *CDynaGeneratorMotion::m_cszUnom = _T("Unom");