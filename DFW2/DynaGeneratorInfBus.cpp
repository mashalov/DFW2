#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"


using namespace DFW2;

CDynaGeneratorInfBusBase::CDynaGeneratorInfBusBase() : CDynaVoltageSource()
{

}

bool CDynaGeneratorInfBus::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;

	double dVre(Vre.Value()), dVim(Vim.Value());
	ptrdiff_t iVre(Vre.Index()), iVim(Vim.Index());

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
	   	
	// dIre / dIre
	pDynaModel->SetElement(A(V_IRE), A(V_IRE), 1.0);
	// dIre / dVim
	pDynaModel->SetElement(A(V_IRE), A(iVim), 1.0 / xd1);

	// dIim / dIim
	pDynaModel->SetElement(A(V_IIM), A(V_IIM), 1.0);
	// dIim / dVre
	pDynaModel->SetElement(A(V_IIM), A(iVre), -1.0 / xd1);

	return pDynaModel->Status() && bRes;
}

bool CDynaGeneratorInfBusBase::CalculatePower()
{
	Ire = (Eqs * sin(Delta) - Vim.Value()) / GetXofEqs().imag();
	Iim = (Vre.Value() - Eqs * cos(Delta)) / GetXofEqs().imag();
	P =  Vre.Value() * Ire + Vim.Value() * Iim;
	Q = -Vre.Value() * Iim + Vim.Value() * Ire;
	return true;
}


bool CDynaGeneratorInfBus::BuildRightHand(CDynaModel* pDynaModel)
{
	double dVre(Vre.Value()), dVim(Vim.Value());

	pDynaModel->SetFunction(A(V_P), P - dVre * Ire - dVim * Iim);
	pDynaModel->SetFunction(A(V_Q), Q + dVre * Iim - dVim * Ire);
	pDynaModel->SetFunction(A(V_IRE), Ire - (EqsSin - dVim) / xd1);
	pDynaModel->SetFunction(A(V_IIM), Iim - (dVre - EqsCos) / xd1);
	return pDynaModel->Status();
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBusBase::Init(CDynaModel* pDynaModel)
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
			P = Q = Delta = Eqs = Ire = Iim = 0.0;
			break;
		}
	}

	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBus::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorInfBusBase::Init(pDynaModel);
	if (CDevice::IsFunctionStatusOK(Status))
	{
		EqsCos = Eqs * cos(Delta);
		EqsSin = Eqs * sin(Delta);
	}
	return Status;
}


bool CDynaGeneratorInfBusBase::SetUpDelta()
{
	bool bRes = true;
	cplx S(P, Q);
	cplx v = polar(V.Value(), DeltaV.Value());
	_ASSERTE(abs(v) > 0.0);
	cplx i = conj(S / v);
	cplx eQ = v + i * GetXofEqs();
	Delta = arg(eQ);
	Eqs = abs(eQ);
	Ire = (Eqs * sin(Delta) - Vim.Value()) / GetXofEqs().imag();
	Iim = (Vre.Value() - Eqs * cos(Delta)) / GetXofEqs().imag();
	return bRes;
}

double CDynaGeneratorInfBusBase::Xgen()
{
	return xd1;
}

cplx CDynaGeneratorInfBusBase::Igen(ptrdiff_t nIteration)
{
	return GetEMF() / cplx(0.0, xd1);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBusBase::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaVoltageSource::UpdateExternalVariables(pDynaModel);
}

const CDeviceContainerProperties CDynaGeneratorInfBusBase::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaVoltageSource::DeviceProperties();
	props.SetType(DEVTYPE_GEN_INFPOWER);
	props.m_strClassName = CDeviceContainerProperties::m_cszNameGeneratorInfPower;
	props.nEquationsCount = CDynaGeneratorInfBusBase::VARS::V_LAST;
	props.m_VarMap.insert(make_pair(_T("P"), CVarIndex(CDynaGeneratorInfBusBase::V_P, VARUNIT_MW)));
	props.m_VarMap.insert(make_pair(_T("Q"), CVarIndex(CDynaGeneratorInfBusBase::V_Q, VARUNIT_MVAR)));
	return props;
}