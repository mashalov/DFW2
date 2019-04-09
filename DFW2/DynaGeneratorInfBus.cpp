#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"


using namespace DFW2;

CDynaGeneratorInfBus::CDynaGeneratorInfBus() : CDynaVoltageSource()
{

}

bool CDynaGeneratorInfBus::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;
	double DeltaGT = Delta - DeltaV.Value();
	double NodeV = V.Value();
	double EsinDeltaGT = Eqs * sin(DeltaGT);
	double EcosDeltaGT = Eqs * cos(DeltaGT);

	if (!IsStateOn())
	{
		NodeV = EcosDeltaGT = EsinDeltaGT = 0.0;
	}

	// dP/dP
	pDynaModel->SetElement(A(V_P), A(V_P), 1.0);
	// dP/dV
	pDynaModel->SetElement(A(V_P), A(V.Index()), -EsinDeltaGT / xd1);
	// dP/dDeltaV
	pDynaModel->SetElement(A(V_P), A(DeltaV.Index()), NodeV * EcosDeltaGT / xd1);

	// dQ/dQ
	pDynaModel->SetElement(A(V_Q), A(V_Q), 1.0);
	// dQ/dV
	pDynaModel->SetElement(A(V_Q), A(V.Index()), (2.0 * NodeV - EcosDeltaGT) / xd1);
	// dQ/dDeltaV
	pDynaModel->SetElement(A(V_Q), A(DeltaV.Index()), -NodeV * EsinDeltaGT / xd1);

	return pDynaModel->Status() && bRes;
}

bool CDynaGeneratorInfBus::CalculatePower()
{
	double DeltaGT = Delta - DeltaV.Value();
	double NodeV = V.Value();
	double EsinDeltaGT = Eqs * sin(DeltaGT);
	double EcosDeltaGT = Eqs * cos(DeltaGT);
	P = NodeV * EsinDeltaGT / xd1;
	Q = (NodeV * EcosDeltaGT - NodeV * NodeV) / xd1;
	return true;
}


bool CDynaGeneratorInfBus::BuildRightHand(CDynaModel* pDynaModel)
{
	double DeltaGT = Delta - DeltaV.Value();
	double NodeV = V.Value();
	double EsinDeltaGT = Eqs * sin(DeltaGT);
	double EcosDeltaGT = Eqs * cos(DeltaGT);

	if (!IsStateOn())
	{
		NodeV = EcosDeltaGT = EsinDeltaGT = 0.0;
	}

	pDynaModel->SetFunction(A(V_P), P - NodeV * EsinDeltaGT / xd1);
	pDynaModel->SetFunction(A(V_Q), Q - (NodeV * EcosDeltaGT - NodeV * NodeV) / xd1);
	return pDynaModel->Status();
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBus::Init(CDynaModel* pDynaModel)
{
	if (Kgen > 1)
		xd1 /= Kgen;

	eDEVICEFUNCTIONSTATUS Status = CDynaVoltageSource::Init(pDynaModel);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		switch (GetState())
		{
		case DS_ON:
			if (!SetUpDelta())
				Status = DFS_FAILED;
			break;
		case DS_OFF:
			P = Q = Delta = Eqs = 0.0;
			break;
		}
	}

	return Status;
}


bool CDynaGeneratorInfBus::SetUpDelta()
{
	bool bRes = true;
	cplx S(P, Q);
	cplx v = polar(V.Value(), DeltaV.Value());
	_ASSERTE(abs(v) > 0.0);
	cplx i = conj(S / v);
	cplx eQ = v + i * GetXofEqs();
	Delta = arg(eQ);
	Eqs = abs(eQ);
	return bRes;
}

bool CDynaGeneratorInfBus::InitExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaVoltageSource::InitExternalVariables(pDynaModel);
}

const CDeviceContainerProperties CDynaGeneratorInfBus::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaVoltageSource::DeviceProperties();
	props.SetType(DEVTYPE_GEN_INFPOWER);
	props.m_strClassName = CDeviceContainerProperties::m_cszNameGeneratorInfPower;
	props.nEquationsCount = CDynaGeneratorInfBus::VARS::V_LAST;
	props.m_VarMap.insert(make_pair(_T("P"), CVarIndex(CDynaGeneratorInfBus::V_P, VARUNIT_MW)));
	props.m_VarMap.insert(make_pair(_T("Q"), CVarIndex(CDynaGeneratorInfBus::V_Q, VARUNIT_MVAR)));
	return props;
}