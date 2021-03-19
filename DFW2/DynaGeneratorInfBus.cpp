#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaGeneratorInfBus.h"


using namespace DFW2;

CDynaGeneratorInfBusBase::CDynaGeneratorInfBusBase() : CDynaVoltageSource()
{

}

bool CDynaGeneratorInfBus::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;

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
	   	
	// dIre / dIre
	pDynaModel->SetElement(Ire, Ire, 1.0);
	// dIre / dVim
	pDynaModel->SetElement(Ire, Vim, 1.0 / xd1);

	// dIim / dIim
	pDynaModel->SetElement(Iim, Iim, 1.0);
	// dIim / dVre
	pDynaModel->SetElement(Iim, Vre, -1.0 / xd1);

	return true;
}

bool CDynaGeneratorInfBusBase::CalculatePower()
{
	Ire = (Eqs * sin(Delta) - Vim) / GetXofEqs().imag();
	Iim = (Vre - Eqs * cos(Delta)) / GetXofEqs().imag();

	P = 2.0 * P;

	P =  Vre * Ire + Vim * Iim;
	Q = -Vre * Iim + Vim * Ire;
	return true;
}


bool CDynaGeneratorInfBus::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunction(P, P - Vre * Ire - Vim * Iim);
	pDynaModel->SetFunction(Q, Q + Vre * Iim - Vim * Ire);
	pDynaModel->SetFunction(Ire, Ire - (EqsSin - Vim) / xd1);
	pDynaModel->SetFunction(Iim, Iim - (Vre - EqsCos) / xd1);
	return true;
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
		case eDEVICESTATE::DS_ON:
			if (!SetUpDelta())
				Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
			break;
		case eDEVICESTATE::DS_OFF:
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
	cplx v = std::polar((double)V, (double)DeltaV);
	_ASSERTE(abs(v) > 0.0);
	cplx i = conj(S / v);
	cplx eQ = v + i * GetXofEqs();
	Delta = arg(eQ);
	Eqs = abs(eQ);
	Ire = (Eqs * sin(Delta) - Vim) / GetXofEqs().imag();
	Iim = (Vre - Eqs * cos(Delta)) / GetXofEqs().imag();
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

void CDynaGeneratorInfBusBase::UpdateSerializer(SerializerPtr& Serializer)
{
	CDynaVoltageSource::UpdateSerializer(Serializer);
	Serializer->AddState("Eqs", Eqs, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(CDynaNodeBase::m_cszDelta, Delta, eVARUNITS::VARUNIT_RADIANS);
	Serializer->AddProperty("xd1", xd1, eVARUNITS::VARUNIT_PU);
}

void CDynaGeneratorInfBus::UpdateSerializer(SerializerPtr& Serializer)
{
	CDynaGeneratorInfBusBase::UpdateSerializer(Serializer);
	Serializer->AddState("EqsCos", EqsCos, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("EqsSin", EqsSin, eVARUNITS::VARUNIT_PU);
}

VariableIndexRefVec& CDynaGeneratorInfBusBase::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaVoltageSource::GetVariables(ChildVec);
}

void CDynaGeneratorInfBusBase::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaVoltageSource::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_INFPOWER);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorInfPower, CDeviceContainerProperties::m_cszSysNameGeneratorInfPower);
	props.nEquationsCount = CDynaGeneratorInfBusBase::VARS::V_LAST;
}