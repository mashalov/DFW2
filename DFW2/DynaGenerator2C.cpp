#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaGenerator2C.h"

using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGenerator2C::InitModel(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaGeneratorDQBase::InitModel(pDynaModel) };

	if (CDevice::IsFunctionStatusOK(Status))
	{
		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
			zsq = 1.0 / (r * r + xd1 * xq1);
			CDynaGenerator2C::ProcessDiscontinuity(pDynaModel);
			Eqs = Vq + r * Iq - xd1 * Id;
			Eds = Vd + r * Id + xq1 * Iq;
			ExtEqe = Eqs - Id * (xd - xd1);
			Eq = Eqs - Id * (xd - xd1); // repeat eq calc after eqs (first eq calc is in processdiscontinuity)
			break;
		case eDEVICESTATE::DS_OFF:
			zsq = 0.0;
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator2C::PreInit(CDynaModel* pDynaModel)
{
	if (Kgen > 1)
	{
		xd /= Kgen;
		xd1 /= Kgen;
		Pnom *= Kgen;
		xq /= Kgen;
		xq1 /= Kgen;
		Mj *= Kgen;
		r /= Kgen;
	}

	Zgen_ = { 0, 0.5 * (xd1 + xq1) };

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaGenerator2C::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGenerator2C::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes{ CDynaGeneratorDQBase::ProcessDiscontinuity(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(eRes))
	{
		if (IsStateOn())
		{
			Eq = Eqs - Id * (xd - xd1);
		}
		else
		{
			Id = Iq = Eq = Ire = Iim = 0.0;
		}
	}
	return eRes;
}

void CDynaGenerator2C::BuildEquations(CDynaModel* pDynaModel)
{
	if (pDynaModel->FillConstantElements())
	{
		// dId/dId
		pDynaModel->SetElement(Id, Id, 1);
		// dId/dVd
		pDynaModel->SetElement(Id, Vd, r * zsq);
		// dId/dVq
		pDynaModel->SetElement(Id, Vq, -xq1 * zsq);
		// dId/dEqs
		pDynaModel->SetElement(Id, Eqs, xq1 * zsq);
		// dId/dEds
		pDynaModel->SetElement(Id, Eds, -r * zsq);

		// dIq/dIq
		pDynaModel->SetElement(Iq, Iq, 1);
		// dIq/dVd
		pDynaModel->SetElement(Iq, Vd, xd1 * zsq);
		// dIq/dVq
		pDynaModel->SetElement(Iq, Vq, r * zsq);
		// dIq/dEqs
		pDynaModel->SetElement(Iq, Eqs, -r * zsq);
		// dIq/dEds
		pDynaModel->SetElement(Iq, Eds, -xd1 * zsq);

		// dEq / dEq
		pDynaModel->SetElement(Eq, Eq, 1.0);
		// dEq / dEqs
		pDynaModel->SetElement(Eq, Eqs, -1.0);
		// dEq / dId
		pDynaModel->SetElement(Eq, Id, xd - xd1);

		pDynaModel->CountConstElementsToSkip(Id, Iq, Eqs);
	}
	else
		pDynaModel->SkipConstElements(Id, Iq, Eqs);


	// dEqs/dEqs
	pDynaModel->SetElement(Eqs, Eqs, -1.0 / Tdo1);
	// dEqs/dId
	pDynaModel->SetElement(Eqs, Id, (xd1 - xd) / Tdo1);
	// dEqs/dEqe
	if (ExtEqe.Indexed())
		pDynaModel->SetElement(Eqs, ExtEqe, -1.0 / Tdo1);

	// dEds/dEds
	pDynaModel->SetElement(Eds, Eds, -1.0 / Tqo1);
	// dEds/dIq
	pDynaModel->SetElement(Eds, Iq, (xq - xq1) / Tqo1);


	BuildMotionEquationBlock(pDynaModel);
	BuildRIfromDQEquations(pDynaModel);
}


void CDynaGenerator2C::BuildRightHand(CDynaModel* pDynaModel)
{
	const cplx I{ GetIdIq() };
	pDynaModel->SetFunction(Id, Id - I.real());
	pDynaModel->SetFunction(Iq, Iq - I.imag());
	pDynaModel->SetFunction(Eq, Eq - Eqs + Id * (xd - xd1));
	SetFunctionsDiff(pDynaModel);
	BuildRIfromDQRightHand(pDynaModel);
}


void CDynaGenerator2C::BuildDerivatives(CDynaModel* pDynaModel)
{
	SetDerivatives(pDynaModel);
}

double* CDynaGenerator2C::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaGenerator1C::GetVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Eqs.Value, V_EQS)
			MAP_VARIABLE(Eds.Value, V_EDS)
		}
	}
	return p;
}

cplx CDynaGenerator2C::GetEMF()
{
	return ToRI({ Eqs, Eds });
}

const cplx& CDynaGenerator2C::CalculateEgen()
{
	const double xgen{ Zgen().imag() };
	return Egen_ = ToRI({ Eqs - Id * (xgen - xd1), Eds + Iq * (xgen - xq1) });
}


cplx CDynaGenerator2C::GetIdIq() const
{
	const double id{ zsq * (r * (Eds - Vd) - xq1 * (Eqs - Vq)) };
	const double iq{ zsq * (r * (Eqs - Vq) + xd1 * (Eds - Vd)) };
	return { id, iq };
}

bool CDynaGenerator2C::CalculatePower()
{
	GetVdVq();
	FromComplex(Id, Iq, GetIdIq());
	P = Vd * Id + Vq * Iq;
	Q = Vd * Iq - Vq * Id;
	return true;
}

void CDynaGenerator2C::CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn)
{
	if (IsStateOn())
	{
		(pDynaModel->*fn)(Eqs, (ExtEqe - Eqs + Id * (xd - xd1)) / Tdo1);
		(pDynaModel->*fn)(Eds, (-Eds - Iq * (xq - xq1)) / Tqo1);
	}
	else
	{
		(pDynaModel->*fn)(Eqs, 0.0);
		(pDynaModel->*fn)(Eds, 0.0);
	}
	CDynaGeneratorDQBase::CalculateDerivatives(pDynaModel, fn);
}


void CDynaGenerator2C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGenerator1C::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_2C);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGenerator2C, CDeviceContainerProperties::m_cszSysNameGenerator2C);
	props.EquationsCount = CDynaGenerator2C::VARS::V_LAST;
	props.VarMap_.insert(std::make_pair(CDynaGeneratorInfBus::m_cszEqs, CVarIndex(CDynaGenerator1C::V_EQS, VARUNIT_KVOLTS)));
	props.VarMap_.insert(std::make_pair(CDynaGenerator2C::m_cszEds, CVarIndex(CDynaGenerator2C::V_EDS, VARUNIT_KVOLTS)));
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGenerator2C>>();
}

VariableIndexRefVec& CDynaGenerator2C::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGenerator1C::GetVariables(JoinVariables({ Eds }, ChildVec));
}

void CDynaGenerator2C::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGenerator1C::UpdateSerializer(Serializer);
	// добавляем свойства модели двухконтурной  модели генератора в ЭДС
	Serializer->AddProperty(m_csztqo1, Tqo1, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_cszxq1, xq1, eVARUNITS::VARUNIT_OHM);
	// добавляем переменные состояния модели двухконтурной модели генератора в ЭДС
	Serializer->AddState(CDynaGenerator2C::m_cszEds, Eds, eVARUNITS::VARUNIT_PU);
}