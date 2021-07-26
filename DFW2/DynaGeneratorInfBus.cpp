#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaGeneratorInfBus.h"


using namespace DFW2;

bool CDynaGeneratorInfBus::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;

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
	P =  Vre * Ire + Vim * Iim;
	Q = -Vre * Iim + Vim * Ire;
	return true;
}


bool CDynaGeneratorInfBus::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunction(Ire, Ire - (EqsSin - Vim) / xd1);
	pDynaModel->SetFunction(Iim, Iim - (Vre - EqsCos) / xd1);
	return true;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBusBase::InitModel(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaVoltageSource::InitModel(pDynaModel);

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
	m_Zgen = { 0 , xd1 };

	if (Kgen > 1)
		xd1 /= Kgen;

	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBus::InitModel(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorInfBusBase::InitModel(pDynaModel);
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
	// тут еще надо учитывать сопротивление статора
	cplx eQ = v + i * cplx(r, GetXofEqs().imag());
	Delta = arg(eQ);
	Eqs = abs(eQ);
	FromComplex(Ire, Iim, i);
	return bRes;
}

const cplx& CDynaGeneratorInfBusBase::Zgen() const
{
	return m_Zgen;
}

cplx CDynaGeneratorInfBusBase::Igen(ptrdiff_t nIteration)
{
	return GetEMF() / cplx(0.0, xd1);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBusBase::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaVoltageSource::UpdateExternalVariables(pDynaModel);
}

void CDynaGeneratorInfBusBase::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaVoltageSource::UpdateSerializer(Serializer);
	Serializer->AddState("Eqs", Eqs, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(CDynaNodeBase::m_cszDelta, Delta, eVARUNITS::VARUNIT_RADIANS);
	Serializer->AddProperty("xd1", xd1, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty("r", r, eVARUNITS::VARUNIT_OHM);
}

void CDynaGeneratorInfBus::UpdateSerializer(CSerializerBase* Serializer)
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

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGeneratorInfBus>>();
}