#include "stdafx.h"
#include "DynaNode.h"
#include "DynaModel.h"
#include "DynaExciterBase.h"
#include "DynaGeneratorMustang.h"

using namespace DFW2;

CDynaGeneratorMustang::CDynaGeneratorMustang() : CDynaGenerator3C()
{

}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::Init(CDynaModel* pDynaModel)
{
	xq1 = xq;

	if (!pDynaModel->ConsiderDampingEquation())
		Kdemp = 0.0;


	if (Kgen > 1)
	{
		xd2 /= Kgen;
		xq2 /= Kgen;
		xq1 /= Kgen;
	}

	double Tds = Td01 * xd1 / xd;
	double Tdss = Td0ss * xd2 / xd1;
	xd1 = (xd * (Tds - Td0ss + Tdss) - xd2 * Td0ss) / (Td01 - Td0ss);


	eDEVICEFUNCTIONSTATUS Status = CDynaGenerator3C::Init(pDynaModel);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		switch (GetState())
		{
			case DS_ON:
		break;
			case DS_OFF:
		break;
		}
	}

	return Status;
}

bool CDynaGeneratorMustang::BuildEquations(CDynaModel *pDynaModel)
{
	if (!pDynaModel->Status())
		return pDynaModel->Status();
	
	bool bRes = true;
	
	if (bRes)
	{
		bRes = true;

		double DeltaGT = Delta - DeltaV.Value();
		double NodeV = V.Value();
		double cosDeltaGT = cos(DeltaGT);
		double sinDeltaGT = sin(DeltaGT);
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv.Value());
		double cosDelta = cos(Delta);
		double sinDelta = sin(Delta);

		if (!IsStateOn())
		{
			NodeV = cosDeltaGT = sinDeltaGT = 0.0;
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

		// dP/dP
		pDynaModel->SetElement(A(V_P), A(V_P), 1.0);
		// dP/dEqss
		pDynaModel->SetElement(A(V_P), A(V_EQSS), -sp2 * Iq);
		// dP/dEdss
		pDynaModel->SetElement(A(V_P), A(V_EDSS), -sp2 * Id);
		// dP/dId
		pDynaModel->SetElement(A(V_P), A(V_ID), -sp2 * (Edss + Iq * (xd2 - xq2)));
		// dP/dIq
		pDynaModel->SetElement(A(V_P), A(V_IQ), -sp2 * (Eqss + Id * (xd2 - xq2)));
		// dP/dSv
		pDynaModel->SetElement(A(V_P), A(Sv.Index()), -(Eqss * Iq + Edss * Id + Id * Iq * (xd2 - xq2)));

		// dQ/dQ
		pDynaModel->SetElement(A(V_Q), A(V_Q), 1.0);
		// dQ/dVd
		pDynaModel->SetElement(A(V_Q), A(V_VD), -Iq);
		// dQ/dVq
		pDynaModel->SetElement(A(V_Q), A(V_VQ), Id);
		// dQ/dId
		pDynaModel->SetElement(A(V_Q), A(V_ID), Vq);
		// dQ/dIq
		pDynaModel->SetElement(A(V_Q), A(V_IQ), -Vd);

		// dVd/dVd
		pDynaModel->SetElement(A(V_VD), A(V_VD), 1);
		// dVd/dV
		pDynaModel->SetElement(A(V_VD), A(V.Index()), sinDeltaGT);
		// dVd/dDeltaV
		pDynaModel->SetElement(A(V_VD), A(DeltaV.Index()), -NodeV * cosDeltaGT);
		// dVd/dDeltaG
		pDynaModel->SetElement(A(V_VD), A(V_DELTA), NodeV * cosDeltaGT);

		// dVq/dVq
		pDynaModel->SetElement(A(V_VQ), A(V_VQ), 1);
		// dVq/dV
		pDynaModel->SetElement(A(V_VQ), A(V.Index()), -cosDeltaGT);
		// dVq/dDeltaV
		pDynaModel->SetElement(A(V_VQ), A(DeltaV.Index()), -NodeV * sinDeltaGT);
		// dVq/dDeltaG
		pDynaModel->SetElement(A(V_VQ), A(V_DELTA), NodeV * sinDeltaGT);

		
		// dId/dId
		pDynaModel->SetElement(A(V_ID), A(V_ID), 1);
		// dId/dVq
		pDynaModel->SetElement(A(V_ID), A(V_VQ), -zsq * xq2);
		// dId/dEqss
		pDynaModel->SetElement(A(V_ID), A(V_EQSS), zsq * sp2 * xq2);
		// dId/dSv
		pDynaModel->SetElement(A(V_ID), A(Sv.Index()), zsq * Eqss * xq2);

		// dIq/dIq
		pDynaModel->SetElement(A(V_IQ), A(V_IQ), 1);
		// dIq/dVd
		pDynaModel->SetElement(A(V_IQ), A(V_VD), zsq * xd2);
		// dIq/dEdss
		pDynaModel->SetElement(A(V_IQ), A(V_EDSS), -zsq * sp2 * xd2);
		// dIq/dSv
		pDynaModel->SetElement(A(V_IQ), A(Sv.Index()), -zsq * Edss * xd2);


		// dEqs/dEqs
		pDynaModel->SetElement(A(V_EQS), A(V_EQS), -1.0 / Td01);
		// dEqs/dId
		pDynaModel->SetElement(A(V_EQS), A(V_ID), -(xd - xd1) / Td01);
		// dEqs/dEqe
		if (ExtEqe.Indexed())
			pDynaModel->SetElement(A(V_EQS), A(ExtEqe.Index()), -1.0 / Td01);

		// m_pExciter->A(CDynaExciterBase::V_EQE)

		// dDeltaG / dS
		pDynaModel->SetElement(A(V_DELTA), A(V_S), -pDynaModel->GetOmega0());
		// dDeltaG / dDeltaG
		pDynaModel->SetElement(A(V_DELTA), A(V_DELTA), 1.0);


		// dS / dS
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0 / Mj * (-Kdemp - Pt / sp1 / sp1));
		// dS / Eqss
		pDynaModel->SetElement(A(V_S), A(V_EQSS), 1.0 / Mj * Iq);
		// dS / Edss
		pDynaModel->SetElement(A(V_S), A(V_EDSS), 1.0 / Mj * Id);
		// dS / Id
		pDynaModel->SetElement(A(V_S), A(V_ID), 1.0 / Mj * (Edss + Iq * (xd2 - xq2)));
		// dS / Iq
		pDynaModel->SetElement(A(V_S), A(V_IQ), 1.0 / Mj * (Eqss + Id * (xd2 - xq2)));


		// dEqss / dEqss
		pDynaModel->SetElement(A(V_EQSS), A(V_EQSS), -1.0 / Td0ss);
		// dEqss / dEqs
		pDynaModel->SetElement(A(V_EQSS), A(V_EQS), -(1.0 / Td0ss - 1.0 / Td01));
		// dEqss / dId
		pDynaModel->SetElement(A(V_EQSS), A(V_ID), -((xd1 - xd2) / Td0ss + (xd - xd1) / Td01));
		// dEqss / dEqe
		if (ExtEqe.Indexed())
			pDynaModel->SetElement(A(V_EQSS), A(ExtEqe.Index()), -1.0 / Td01);

		// dEdss / dEdss
		pDynaModel->SetElement(A(V_EDSS), A(V_EDSS), -1.0 / Tq0ss);
		// dEdss / dIq
		pDynaModel->SetElement(A(V_EDSS), A(V_IQ), (xq1 - xq2) / Tq0ss);

		// dEq / dEq
		pDynaModel->SetElement(A(V_EQ), A(V_EQ), 1.0);
		// dEq / dEqss
		pDynaModel->SetElement(A(V_EQ), A(V_EQSS), -1.0);
		// dEq / dId
		pDynaModel->SetElement(A(V_EQ), A(V_ID), xd - xd2);


		//pDynaModel->SetFunction(A(V_IRE), Ire - Iq * co + Id * si);

		

	    // dIre / dIre
		pDynaModel->SetElement(A(V_IRE), A(V_IRE), 1.0);
		// dIre / dId
		pDynaModel->SetElement(A(V_IRE), A(V_ID), sinDelta);
		// dIre / dIq
		pDynaModel->SetElement(A(V_IRE), A(V_IQ),  -cosDelta);
		// dIre / dDeltaG
		pDynaModel->SetElement(A(V_IRE), A(V_DELTA), Iq * sinDelta + Id * cosDeltaGT);

		// dIim / dIim
		pDynaModel->SetElement(A(V_IIM), A(V_IIM), 1.0);
		// dIim / dId
		pDynaModel->SetElement(A(V_IIM), A(V_ID), -cosDelta);
		// dIim / dIq
		pDynaModel->SetElement(A(V_IIM), A(V_IQ), -sinDelta);
		// dIim / dDeltaG
		pDynaModel->SetElement(A(V_IIM), A(V_DELTA), Id * sinDelta - Iq * cosDelta);
		
	}
	return pDynaModel->Status() && bRes;
}


bool CDynaGeneratorMustang::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (bRes)
	{
		bRes = true;

		double DeltaGT = Delta - DeltaV.Value();
		double NodeV = V.Value();
		double cosDeltaGT = cos(DeltaGT);
		double sinDeltaGT = sin(DeltaGT);
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + Sv.Value());

		if (!IsStateOn())
		{
			NodeV = cosDeltaGT = sinDeltaGT = 0.0;
			sp1 = sp2 = 1.0;
		}

		pDynaModel->SetFunction(A(V_VD), Vd + NodeV * sinDeltaGT);
		pDynaModel->SetFunction(A(V_VQ), Vq - NodeV * cosDeltaGT);
		pDynaModel->SetFunction(A(V_P), P - sp2 * (Eqss * Iq + Edss * Id + Id * Iq * (xd2 - xq2)));
		pDynaModel->SetFunction(A(V_Q), Q - Vd * Iq + Vq * Id);
		pDynaModel->SetFunction(A(V_ID), Id + zsq * (sp2 * Eqss - Vq) * xq2);
		pDynaModel->SetFunction(A(V_IQ), Iq + zsq * (Vd - sp2 * Edss) * xd2);
		pDynaModel->SetFunction(A(V_EQ), Eq - Eqss + Id * (xd - xd2));
		double eDelta = pDynaModel->GetOmega0() * s;
		double eS = (Pt / sp1 - Kdemp  * s - (Eqss * Iq + Edss * Id + Id * Iq * (xd2 - xq2))) / Mj;
		double eEqs = (ExtEqe.Value() - Eqs + Id * (xd - xd1)) / Td01;
		double eEdss = (-Edss - Iq * (xq1 - xq2)) / Tq0ss;
		double eEqss = Eqs * (1.0 / Td0ss - 1.0 / Td01) + Id * ((xd1 - xd2) / Td0ss + (xd - xd1) / Td01) - Eqss / Td0ss + ExtEqe.Value() / Td01;
		pDynaModel->SetFunctionDiff(A(V_S), eS);
		pDynaModel->SetFunctionDiff(A(V_DELTA), pDynaModel->GetOmega0() * s);
		pDynaModel->SetFunctionDiff(A(V_EQS), eEqs);
		pDynaModel->SetFunctionDiff(A(V_EQSS), eEqss);
		pDynaModel->SetFunctionDiff(A(V_EDSS), eEdss);

		double co = cos(Delta);
		double si = sin(Delta);
		pDynaModel->SetFunction(A(V_IRE), Ire - Iq * co + Id * si);
		pDynaModel->SetFunction(A(V_IIM), Iim - Iq * si - Id * co);
	}
	return pDynaModel->Status() && bRes;
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
			double eEqss = Eqs * (1.0 / Td0ss - 1.0 / Td01) + Id * ((xd1 - xd2) / Td0ss + (xd - xd1) / Td01) - Eqss / Td0ss + ExtEqe.Value() / Td01;
			double eEdss = (-Edss - Iq * (xq1 - xq2)) / Tq0ss;
			pDynaModel->SetDerivative(A(V_S), eS);
			pDynaModel->SetDerivative(A(V_EDSS), eEdss);
			pDynaModel->SetDerivative(A(V_EQSS), eEqss);
		}
		else
		{
			pDynaModel->SetDerivative(A(V_S), 0.0);
			pDynaModel->SetDerivative(A(V_EQSS), 0.0);
			pDynaModel->SetDerivative(A(V_EDSS), 0.0);
		}
	}
	return pDynaModel->Status();
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMustang::ProcessDiscontinuity(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGenerator3C::ProcessDiscontinuity(pDynaModel);
	if (IsStateOn())
	{
		double DeltaGT = Delta - DeltaV.Value();
		double NodeV = V.Value();
		double sinDeltaGT = sin(DeltaGT);
		double cosDeltaGT = cos(DeltaGT);
		double sp2 = ZeroGuardSlip(1.0 + Sv.Value());
		Vd = -NodeV * sinDeltaGT;
		Vq =  NodeV * cosDeltaGT;
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

bool CDynaGeneratorMustang::CalculatePower()
{
	double DeltaGT = Delta - DeltaV.Value();
	double NodeV = V.Value();
	double sinDeltaGT = sin(DeltaGT);
	double cosDeltaGT = cos(DeltaGT);
	double sp2 = ZeroGuardSlip(1.0 + Sv.Value());
	Vd = -NodeV * sinDeltaGT;
	Vq = NodeV * cosDeltaGT;
	Id = -zsq * (sp2 * Eqss - Vq) * xq2;
	Iq = -zsq * (Vd - sp2 * Edss) * xd2;
	P = sp2 * (Eqss * Iq + Edss * Id + Id * Iq * (xd2 - xq2));
	Q = Vd * Iq - Vq * Id;
	IfromDQ();
	return true;
}


const CDeviceContainerProperties CDynaGeneratorMustang::DeviceProperties()
{
	// пример "наследования" атрибутов контейнера
	
	// берем атрибуты родителя
	CDeviceContainerProperties props = CDynaGenerator3C::DeviceProperties();
	// добавляем свой тип. В списке типов уже есть все типы родительской цепочки 
	props.SetType(DEVTYPE_GEN_MUSTANG);
	// задаем имя типа устройства
	props.m_strClassName = CDeviceContainerProperties::m_cszNameGeneratorMustang;
	// задаем количество уравнений устройства
	props.nEquationsCount = CDynaGeneratorMustang::VARS::V_LAST;
	return props;
}