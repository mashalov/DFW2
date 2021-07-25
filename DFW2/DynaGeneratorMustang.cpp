#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"
#include "DynaGeneratorMustang.h"

using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::Init(CDynaModel* pDynaModel)
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

	double Tds = Td01 * xd1 / xd;
	double Tdss = Td02 * xd2 / xd1;

	xd1 = (xd * (Tds - Td02 + Tdss) - xd2 * Td02) / (Td01 - Td02);

	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::InitModel(CDynaModel* pDynaModel)
{

	eDEVICEFUNCTIONSTATUS Status = CDynaGenerator3C::InitModel(pDynaModel);

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

bool CDynaGeneratorMustang::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	
	if (bRes)
	{
		bRes = true;

		double cosg(cos(Delta)), sing(sin(Delta));
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

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
		pDynaModel->SetElement(Eqs, Eqs, -1.0 / Td01);
		// dEqs/dId
		pDynaModel->SetElement(Eqs, Id, -(xd - xd1) / Td01);
		// dEqs/dEqe
		if (ExtEqe.Indexed())
			pDynaModel->SetElement(Eqs, ExtEqe, -1.0 / Td01);

		// m_pExciter->A(CDynaExciterBase::V_EQE)

		// dDeltaG / dS
		pDynaModel->SetElement(Delta, s, -pDynaModel->GetOmega0());
		// dDeltaG / dDeltaG
		pDynaModel->SetElement(Delta, Delta, 0.0);
		
		// dS / dS
		pDynaModel->SetElement(s, s, 1.0 / Mj * (-Kdemp - Pt / sp1 / sp1));
		// dS / Eqss
		pDynaModel->SetElement(s, Eqss, 1.0 / Mj * Iq);
		// dS / Edss
		pDynaModel->SetElement(s, Edss, 1.0 / Mj * Id);
		// dS / Id
		pDynaModel->SetElement(s, Id, 1.0 / Mj * (Edss + Iq * (xd2 - xq2)));
		// dS / Iq
		pDynaModel->SetElement(s, Iq, 1.0 / Mj * (Eqss + Id * (xd2 - xq2)));


		// dEqss / dEqss
		pDynaModel->SetElement(Eqss, Eqss, -1.0 / Td02);
		// dEqss / dEqs
		pDynaModel->SetElement(Eqss, Eqs, -(1.0 / Td02 - 1.0 / Td01));
		// dEqss / dId
		pDynaModel->SetElement(Eqss, Id, -((xd1 - xd2) / Td02 + (xd - xd1) / Td01));
		// dEqss / dEqe
		if (ExtEqe.Indexed())
			pDynaModel->SetElement(Eqss, ExtEqe, -1.0 / Td01);

		// dEdss / dEdss
		pDynaModel->SetElement(Edss, Edss, -1.0 / Tq02);
		// dEdss / dIq
		pDynaModel->SetElement(Edss, Iq, (xq1 - xq2) / Tq02);

		// dEq / dEq
		pDynaModel->SetElement(Eq, Eq, 1.0);
		// dEq / dEqss
		pDynaModel->SetElement(Eq, Eqss, -1.0);
		// dEq / dId
		pDynaModel->SetElement(Eq, Id, xd - xd2);

		// строит уравнения для Vd, Vq, Ire, Iim
		bRes = bRes && BuildRIfromDQEquations(pDynaModel);
	}
	return true;
}


bool CDynaGeneratorMustang::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (bRes)
	{
		bRes = true;
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

		pDynaModel->SetFunction(Id, Id + zsq * (sp2 * Eqss - Vq) * xq2);
		pDynaModel->SetFunction(Iq, Iq + zsq * (Vd - sp2 * Edss) * xd2);
		pDynaModel->SetFunction(Eq, Eq - Eqss + Id * (xd - xd2));
		double eS = (Pt / sp1 - Kdemp  * s - (Eqss * Iq + Edss * Id + Id * Iq * (xd2 - xq2))) / Mj;
		double eEqs = (ExtEqe - Eqs + Id * (xd - xd1)) / Td01;
		double eEdss = (-Edss - Iq * (xq1 - xq2)) / Tq02;
		double eEqss = Eqs * (1.0 / Td02 - 1.0 / Td01) + Id * ((xd1 - xd2) / Td02 + (xd - xd1) / Td01) - Eqss / Td02 + ExtEqe / Td01;
		pDynaModel->SetFunctionDiff(s, eS);
		pDynaModel->SetFunctionDiff(Delta, pDynaModel->GetOmega0() * s);
		pDynaModel->SetFunctionDiff(Eqs, eEqs);
		pDynaModel->SetFunctionDiff(Eqss, eEqss);
		pDynaModel->SetFunctionDiff(Edss, eEdss);

		// строит уравнения для Vd, Vq, Ire, Iim
		bRes = bRes && BuildRIfromDQRightHand(pDynaModel);

		//DumpIntegrationStep(97, 2028);
	}
	return true;
}


bool CDynaGeneratorMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = CDynaGenerator3C::BuildDerivatives(pDynaModel);
	if (bRes)
	{
		if (IsStateOn())
		{
			double sp1 = ZeroGuardSlip(1.0 + s);
			double eS = (Pt / sp1 - Kdemp  * s - (Eqss * Iq + Edss * Id + Id * Iq * (xd2 - xq2))) / Mj;
			double eEqss = Eqs * (1.0 / Td02 - 1.0 / Td01) + Id * ((xd1 - xd2) / Td02 + (xd - xd1) / Td01) - Eqss / Td02 + ExtEqe / Td01;
			double eEdss = (-Edss - Iq * (xq1 - xq2)) / Tq02;
			pDynaModel->SetDerivative(s, eS);
			pDynaModel->SetDerivative(Edss, eEdss);
			pDynaModel->SetDerivative(Eqss, eEqss);
		}
		else
		{
			pDynaModel->SetDerivative(s, 0.0);
			pDynaModel->SetDerivative(Eqss, 0.0);
			pDynaModel->SetDerivative(Edss, 0.0);
		}
	}
	return true;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::ProcessDiscontinuity(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGenerator3C::ProcessDiscontinuity(pDynaModel);
	if (IsStateOn())
	{
		double dVre(Vre), dVim(Vim);
		double cosg(cos(Delta)), sing(sin(Delta));
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv);

		Vd = -dVre * sing + dVim * cosg;
		Vq =  dVre * cosg + dVim * sing;
		Id = -zsq * (sp2 * Eqss - Vq) * xq2;
		Iq = -zsq * (Vd - sp2 * Edss) * xd2;
		P =  sp2 * (Eqss * Iq + Edss * Id + Id * Iq * (xd2 - xq2));
		Q =  Vd * Iq - Vq * Id;
		Eq  = Eqss - Id * (xd - xd2);
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
	double xgen = Xgen();
	double sp2 = ZeroGuardSlip(1.0 + Sv);
	return m_Egen = cplx(sp2 * Eqss - Id * (xgen - xd2), sp2 * Edss + Iq * (xgen - xq2)) * std::polar(1.0, (double)Delta);
}

bool CDynaGeneratorMustang::CalculatePower()
{
	double dVre(Vre), dVim(Vim);
	double cosg(cos(Delta)), sing(sin(Delta));
	double sp1 = ZeroGuardSlip(1.0 + s);
	double sp2 = ZeroGuardSlip(1.0 + Sv);

	Vd = -dVre * sing + dVim * cosg;
	Vq =  dVre * cosg + dVim * sing;
	Id = -zsq * (sp2 * Eqss - Vq) * xq2;
	Iq = -zsq * (Vd - sp2 * Edss) * xd2;
	P = sp2 * (Eqss * Iq + Edss * Id + Id * Iq * (xd2 - xq2));
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
	props.nEquationsCount = CDynaGeneratorMustang::VARS::V_LAST;

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGeneratorMustang>>();
}