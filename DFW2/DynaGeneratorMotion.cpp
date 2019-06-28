#include "stdafx.h"
#include "DynaGeneratorMotion.h"
#include "DynaModel.h"


using namespace DFW2;

CDynaGeneratorMotion::CDynaGeneratorMotion() : CDynaGeneratorInfBus()
{

}


double* CDynaGeneratorMotion::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorInfBus::GetVariablePtr(nVarIndex);
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

	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorInfBus::Init(pDynaModel);
	
	if (IsPresent() && CDevice::IsFunctionStatusOK(Status))
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
	if (!pDynaModel->Status())
		return pDynaModel->Status();

	// для угла относительная точность не имеет смысла
	pDynaModel->GetRightVector(A(V_DELTA))->Rtol = 0.0;

	bool bRes = true;
	if (bRes)
	{

		if (GetId() == 510384)
			bRes = true;

		double NodeV = V.Value();
		double NodeSv = Sv.Value();

		double DeltaGT = Delta - DeltaV.Value();
		double EcosDeltaGT = Eqs * cos(DeltaGT);
		double EsinDeltaGT = Eqs * sin(DeltaGT);
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + NodeSv);

		if (!IsStateOn())
		{
			NodeV = EcosDeltaGT = EsinDeltaGT = 0.0;
			sp1 = sp2 = 1.0;
		}


		

		// P
		// Q
		// S
		// DELTA

		//dP/dP
		pDynaModel->SetElement(A(V_P), A(V_P), 1.0);
		// dP/dV
		pDynaModel->SetElement(A(V_P), A(V.Index()), -EsinDeltaGT / xd1);
		// dP/dDelta
		double dPdDelta = NodeV * EcosDeltaGT / xd1;
		pDynaModel->SetElement(A(V_P), A(V_DELTA), -dPdDelta);
		// dP/dDeltaV
		pDynaModel->SetElement(A(V_P), A(DeltaV.Index()), dPdDelta);


		// dQ/dQ
		pDynaModel->SetElement(A(V_Q), A(V_Q), 1.0);
		// dQ/dV
		pDynaModel->SetElement(A(V_Q), A(V.Index()), (2 * NodeV - EcosDeltaGT) / xd1);
		// dQ/dDelta
		double dQDelta = NodeV * EsinDeltaGT / xd1;
		pDynaModel->SetElement(A(V_Q), A(V_DELTA), dQDelta);
		// dQ/dDeltaV
		pDynaModel->SetElement(A(V_Q), A(DeltaV.Index()), -dQDelta);

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
	}
	return pDynaModel->Status() && bRes;
}


bool CDynaGeneratorMotion::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{
		double NodeV = V.Value();
		double NodeSv = Sv.Value();
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + NodeSv);
		double DeltaGT = Delta - DeltaV.Value();
		double EcosDeltaGT = Eqs * cos(DeltaGT);
		double EsinDeltaGT = Eqs * sin(DeltaGT);

		if (!IsStateOn())
		{
			NodeV = EcosDeltaGT = EsinDeltaGT = 0.0;
			sp1 = sp2 = 1.0;
		}
		

		pDynaModel->SetFunction(A(V_P), P - NodeV * EsinDeltaGT / xd1);
		pDynaModel->SetFunction(A(V_Q), Q - (NodeV * EcosDeltaGT - NodeV * NodeV) / xd1);
		double eDelta = pDynaModel->GetOmega0() * s;
		double eS = (Pt / sp1 - Kdemp  * s - P / sp2) / Mj;
		pDynaModel->SetFunctionDiff(A(V_S), eS);
		pDynaModel->SetFunctionDiff(A(V_DELTA), eDelta);
	}

	return pDynaModel->Status() && bRes;
}


bool CDynaGeneratorMotion::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{
		double DeltaGT = Delta - DeltaV.Value();
		double sp1 = 1.0 + s;
		double sp2 = 1.0 + Sv.Value();

		if (!IsStateOn())
			sp1 = sp2 = 1.0;

		double eDelta = pDynaModel->GetOmega0() * s;
		double eS = (Pt / sp1 - Kdemp  * s - P / sp2) / Mj;

		pDynaModel->SetDerivative(A(V_S), eS);
		pDynaModel->SetDerivative(A(V_DELTA), eDelta);
	}
	return pDynaModel->Status();
}

double* CDynaGeneratorMotion::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorInfBus::GetConstVariablePtr(nVarIndex);
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
	return CDynaGeneratorInfBus::UpdateExternalVariables(pDynaModel);
}


const CDeviceContainerProperties CDynaGeneratorMotion::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaGeneratorInfBus::DeviceProperties();
	props.SetType(DEVTYPE_GEN_MOTION);
	props.m_strClassName = CDeviceContainerProperties::m_cszNameGeneratorMotion;
	props.nEquationsCount = CDynaGeneratorMotion::VARS::V_LAST;

	props.m_VarMap.insert(make_pair(_T("S"), CVarIndex(CDynaGeneratorMotion::V_S, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("Delta"), CVarIndex(CDynaGeneratorMotion::V_DELTA, VARUNIT_RADIANS)));
	props.m_ConstVarMap.insert(make_pair(CDynaGeneratorMotion::m_cszUnom, CConstVarIndex(CDynaGeneratorMotion::C_UNOM, eDVT_CONSTSOURCE)));
	return props;
}

const _TCHAR *CDynaGeneratorMotion::m_cszUnom = _T("Unom");