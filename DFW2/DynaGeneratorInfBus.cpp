#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaGeneratorInfBus.h"


using namespace DFW2;

void CDynaGeneratorInfBus::BuildEquations(CDynaModel* pDynaModel)
{
	// ток от ЭДС ШБМ является постоянным
	// изменяется только составляющая тока на шунте Нортона
	// dIre / dIre
	pDynaModel->SetElement(Ire, Ire, 1.0);
	// dIim / dIim
	pDynaModel->SetElement(Iim, Iim, 1.0);
}

bool CDynaGeneratorInfBusBase::CalculatePower()
{
	Ire = (Eqs * sin(Delta) - Vim) / GetXofEqs();
	Iim = (Vre - Eqs * cos(Delta)) / GetXofEqs();
	P = Vre * Ire + Vim * Iim;
	Q = -Vre * Iim + Vim * Ire;

	if (std::abs(Ynorton_) > DFW2_EPSILON)
	{
		FromComplex(Ire, Iim, GetEMF() * Ynorton_);
	}
	
	return true;
}


void CDynaGeneratorInfBus::BuildRightHand(CDynaModel* pDynaModel)
{
	// ток от ЭДС ШБМ является постоянным
	// изменяется только составляющая тока на шунте Нортона
	pDynaModel->SetFunction(Ire, 0.0);
	pDynaModel->SetFunction(Iim, 0.0);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBusBase::InitModel(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaVoltageSource::InitModel(pDynaModel) };

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

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBus::PreInit(CDynaModel* pDynaModel)
{
	if (Kgen > 1)
		xd1 /= Kgen;

	Zgen_ = { 0 , xd1 };
	// шунт Нортона для ШБМ
	Ynorton_ = 1.0 / Zgen_;

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBus::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBus::InitModel(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaGeneratorInfBusBase::InitModel(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(Status))
	{
		EqsCos = Eqs * cos(Delta);
		EqsSin = Eqs * sin(Delta);
	}
	return Status;
}


bool CDynaGeneratorInfBusBase::SetUpDelta()
{
	bool bRes{ true };
	cplx S(P, Q);
	cplx v = std::polar((double)V, (double)DeltaV);
	_ASSERTE(abs(v) > 0.0);
	cplx i = conj(S / v);
	// тут еще надо учитывать сопротивление статора
	const cplx eQ{ v + i * cplx(r, GetXofEqs()) };
	Delta = arg(eQ);
	Eqs = abs(eQ);

	// если у генератора есть ненулевой шунт Нортона,
	// его ток инициализируется как ток только от ЭДС
	if (std::abs(Ynorton_) > DFW2_EPSILON)
		i = eQ * Ynorton_;

	FromComplex(Ire, Iim, i);
	return bRes;
}

const cplx& CDynaGeneratorInfBusBase::Zgen() const
{
	return Zgen_;
}

cplx CDynaGeneratorInfBusBase::Igen(ptrdiff_t nIteration)
{
	// функция общая для всех генераторов с шунтом Нортона
	return GetEMF() * Ynorton_;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorInfBusBase::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaVoltageSource::UpdateExternalVariables(pDynaModel);
}

void CDynaGeneratorInfBusBase::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaVoltageSource::UpdateSerializer(Serializer);
	Serializer->AddState(m_cszEqs, Eqs, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(CDynaNodeBase::m_cszDelta, Delta, eVARUNITS::VARUNIT_RADIANS);
	Serializer->AddProperty(m_cszxd1, xd1, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszr, r, eVARUNITS::VARUNIT_OHM);
}

void CDynaGeneratorInfBusBase::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaVoltageSource::UpdateValidator(Validator);
	Validator->AddRule(m_cszxd1, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszr, &CSerializerValidatorRules::NonNegative);
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