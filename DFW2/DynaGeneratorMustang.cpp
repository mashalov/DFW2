#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"
#include "DynaGeneratorMustang.h"

using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::PreInit(CDynaModel* pDynaModel)
{
	xq1 = xq;

	r = 0.0;		/// в модели генератора Мустанг активного сопротивления статора нет

	if (Kgen > 1)
	{
		xd1 /= Kgen;
		Pnom *= Kgen;
		xq /= Kgen;
		Mj *= Kgen;
		xd /= Kgen;
		xq1 /= Kgen;
		xd2 /= Kgen;
		xq2 /= Kgen;
	}

	// здесь пытаемся вычислить коррекцию до валидации параметров
	// если придется делить на ноль - ничего не делаем.
	// валидация должна далее найти некорректные данные и показать
	if (xd > 0 && xd1 > 0)
	{
		const double Tds{ Tdo1 * xd1 / xd };
		const double Tdss{ Tdo2 * xd2 / xd1 };
		if(Tdo1 > Tdo2)
			xd1 = (xd * (Tds - Tdo2 + Tdss) - xd2 * Tdo2) / (Tdo1 - Tdo2);
	}

	Zgen_ = { 0, 0.5 * (xd2 + xq2) };

	xd2_xq2_ = xd2 - xq2;
	xd_xd1_ = xd - xd1;
	xq1_xq2_ = xq1 - xq2;
	xd1_xd2_ = xd1 - xd2;
	xd_xd2_ = xd - xd2;
	Tm_ = (1.0 / Tdo2 - 1.0 / Tdo1);
	Txm_ = (xd1_xd2_ / Tdo2 + xd_xd1_ / Tdo1);

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::InitModel(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaGenerator3C::InitModel(pDynaModel) };

	if (CDevice::IsFunctionStatusOK(Status))
	{
		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
		break;
		case eDEVICESTATE::DS_OFF:
		break;
		}
	}

	return Status;
}

void CDynaGeneratorMustang::BuildEquations(CDynaModel *pDynaModel)
{
	const double cosg{ cos(Delta) }, sing{ sin(Delta) };
	double sp1{ ZeroGuardSlip(1.0 + s) }, sp2{ ZeroGuardSlip(1.0 + Sv) };

	if (!IsStateOn())
		sp1 = sp2 = 1.0;
	// P
	// Q
	// S
	// DELTA
	// EQS
	// V_D
	// V_Q
	// I_D
	// I_Q
	// EQSS
	// EDSS

	// dId/dId
	pDynaModel->SetElement(Id, Id, 1);
	// dId/dVq
	pDynaModel->SetElement(Id, Vq, -zsq * xq2);
	// dId/dEqss
	pDynaModel->SetElement(Id, Eqss, zsq * sp2 * xq2);
	// dId/dSv
	pDynaModel->SetElement(Id, Sv, zsq * Eqss * xq2);

	// dIq/dIq
	pDynaModel->SetElement(Iq, Iq, 1);
	// dIq/dVd
	pDynaModel->SetElement(Iq, Vd, zsq * xd2);
	// dIq/dEdss
	pDynaModel->SetElement(Iq, Edss, -zsq * sp2 * xd2);
	// dIq/dSv
	pDynaModel->SetElement(Iq, Sv, -zsq * Edss * xd2);


	// dEqs/dEqs
	pDynaModel->SetElement(Eqs, Eqs, -1.0 / Tdo1);
	// dEqs/dId
	pDynaModel->SetElement(Eqs, Id, -xd_xd1_/ Tdo1);
	// dEqs/dEqe
	if (ExtEqe.Indexed())
		pDynaModel->SetElement(Eqs, ExtEqe, -1.0 / Tdo1);

	// m_pExciter->A(CDynaExciterBase::V_EQE)

	// dS / dS
	pDynaModel->SetElement(s, s, (-Kdemp - Pt / sp1 / sp1) / Mj);
	// dS / Eqss
	pDynaModel->SetElement(s, Eqss, Iq / Mj);
	// dS / Edss
	pDynaModel->SetElement(s, Edss, Id / Mj);
	// dS / Id
	pDynaModel->SetElement(s, Id, (Edss + Iq * xd2_xq2_) / Mj);
	// dS / Iq
	pDynaModel->SetElement(s, Iq, (Eqss + Id * xd2_xq2_) / Mj);


	// dEqss / dEqss
	pDynaModel->SetElement(Eqss, Eqss, -1.0 / Tdo2);
	// dEqss / dEqs
	pDynaModel->SetElement(Eqss, Eqs, -Tm_);
	// dEqss / dId
	pDynaModel->SetElement(Eqss, Id, -Txm_);
	// dEqss / dEqe
	if (ExtEqe.Indexed())
		pDynaModel->SetElement(Eqss, ExtEqe, -1.0 / Tdo1);

	// dEdss / dEdss
	pDynaModel->SetElement(Edss, Edss, -1.0 / Tqo2);
	// dEdss / dIq
	pDynaModel->SetElement(Edss, Iq, xq1_xq2_ / Tqo2);

	// dEq / dEq
	pDynaModel->SetElement(Eq, Eq, 1.0);
	// dEq / dEqss
	pDynaModel->SetElement(Eq, Eqss, -1.0);
	// dEq / dId
	pDynaModel->SetElement(Eq, Id, xd_xd2_);

	// строит уравнения для Vd, Vq, Ire, Iim
	BuildRIfromDQEquations(pDynaModel);
	BuildAngleEquationBlock(pDynaModel);
}


void CDynaGeneratorMustang::BuildRightHand(CDynaModel *pDynaModel)
{
	double sp1{ ZeroGuardSlip(1.0 + s) }, sp2{ ZeroGuardSlip(1.0 + Sv) };

	if (!IsStateOn())
		sp1 = sp2 = 1.0;

	pDynaModel->SetFunction(Id, Id + zsq * (sp2 * Eqss - Vq) * xq2);
	pDynaModel->SetFunction(Iq, Iq + zsq * (Vd - sp2 * Edss) * xd2);
	pDynaModel->SetFunction(Eq, Eq - Eqss + Id * xd_xd2_);
	SetFunctionsDiff(pDynaModel);
	// строит уравнения для Vd, Vq, Ire, Iim
	BuildRIfromDQRightHand(pDynaModel);
}

void CDynaGeneratorMustang::CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn)
{
	if (IsStateOn())
	{
		(pDynaModel->*fn)(Delta, pDynaModel->GetOmega0() * s);
		(pDynaModel->*fn)(s, (Pt / ( 1.0 + s ) - Kdemp * s - (Eqss * Iq + Edss * Id + Id * Iq * xd2_xq2_)) / Mj);
		(pDynaModel->*fn)(Eqs, (ExtEqe - Eqs + Id * xd_xd1_) / Tdo1);
		(pDynaModel->*fn)(Edss, (-Edss - Iq * xq1_xq2_) / Tqo2);
		(pDynaModel->*fn)(Eqss, Eqs * Tm_ + Id * Txm_ - Eqss / Tdo2 + ExtEqe / Tdo1);
	}
	else
	{
		(pDynaModel->*fn)(Delta, 0);
		(pDynaModel->*fn)(s, 0);
		(pDynaModel->*fn)(Eqs, 0);
		(pDynaModel->*fn)(Edss, 0);
		(pDynaModel->*fn)(Eqss, 0);
	}
}

void CDynaGeneratorMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	SetDerivatives(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::ProcessDiscontinuity(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes{ CDynaGenerator3C::ProcessDiscontinuity(pDynaModel) };
	if (IsStateOn())
	{
		const double sp1{ ZeroGuardSlip(1.0 + s) }, sp2{ ZeroGuardSlip(1.0 + Sv) };
		GetVdVq();
		Id = -zsq * (sp2 * Eqss - Vq) * xq2;
		Iq = -zsq * (Vd - sp2 * Edss) * xd2;
		P =  sp2 * (Eqss * Iq + Edss * Id + Id * Iq * xd2_xq2_);
		Q =  Vd * Iq - Vq * Id;
		Eq  = Eqss - Id * xd_xd2_;
		IfromDQ();
	}
	else
	{
		Id = Iq = Eq = P = Q = Ire = Iim = 0.0;
	}
	
	return eRes;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaGenerator3C::UpdateExternalVariables(pDynaModel);
}

const cplx& CDynaGeneratorMustang::CalculateEgen()
{
	const double xgen{ Zgen().imag() };
	const double sp2{ ZeroGuardSlip(1.0 + Sv) };
	return Egen_ = cplx(sp2 * Eqss - Id * (xgen - xd2), sp2 * Edss + Iq * (xgen - xq2)) * std::polar(1.0, (double)Delta);
}

bool CDynaGeneratorMustang::CalculatePower()
{
	const double sp1{ ZeroGuardSlip(1.0 + s) };
	const double sp2{ ZeroGuardSlip(1.0 + Sv) };

	GetVdVq();

	Id = -zsq * (sp2 * Eqss - Vq) * xq2;
	Iq = -zsq * (Vd - sp2 * Edss) * xd2;
	P = sp2 * (Eqss * Iq + Edss * Id + Id * Iq * xd2_xq2_);
	Q = Vd * Iq - Vq * Id;
	IfromDQ();
	return true;
}


void CDynaGeneratorMustang::DeviceProperties(CDeviceContainerProperties& props)
{
	// пример "наследования" атрибутов контейнера
	
	// берем атрибуты родителя
	CDynaGenerator3C::DeviceProperties(props);
	// добавляем свой тип. В списке типов уже есть все типы родительской цепочки 
	props.SetType(DEVTYPE_GEN_MUSTANG);
	// задаем имя типа устройства
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorMustang, CDeviceContainerProperties::m_cszSysNameGeneratorMustang);
	// задаем количество уравнений устройства
	props.EquationsCount = CDynaGeneratorMustang::VARS::V_LAST;

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGeneratorMustang>>();
}