#include "stdafx.h"
#include "LimiterConst.h"
#include "DynaModel.h"

using namespace DFW2;

bool CLimiterConst::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double dInput = m_Input->Value();
	double dOdI = -1.0;

	switch (GetCurrentState())
	{
	case LS_MAX:
		m_Output = dInput = m_dMax;
		dOdI = 0.0;
		break;
	case LS_MIN:
		m_Output = dInput = m_dMin;
		dOdI = 0.0;
		break;
	}

	if (!m_Device.IsStateOn())
		dOdI = 0.0;

	pDynaModel->SetElement(m_Output, m_Output, 1.0);
	pDynaModel->SetElement(m_Output.Index, m_Input->Index(), dOdI);
	return true;
}

bool CLimiterConst::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dInput = m_Input->Value();
		switch (GetCurrentState())
		{
		case LS_MAX:
			m_Output = dInput = m_dMax;
			break;
		case LS_MIN:
			m_Output = dInput = m_dMin;
			break;
		}
		pDynaModel->SetFunction(m_Output, m_Output - dInput);
	}
	else
		pDynaModel->SetFunction(m_Output, 0.0);

	return true;
}



bool CLimiterConst::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitiveLimited::Init(pDynaModel);
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

// контроль зерокроссинга для состояния вне ограничения
double CLimiterConst::OnStateMid(CDynaModel *pDynaModel)
{
	double dInput = m_Input->Value();
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_dMaxH - dInput, dInput, m_dMaxH, m_Input->Index(), rH))
		SetCurrentState(pDynaModel, LS_MAX);
	if (GetCurrentState() == LS_MID && !pDynaModel->GetZeroCrossingInRange(rH))
		if (CDynaPrimitive::ChangeState(pDynaModel, dInput- m_dMinH, dInput, m_dMinH, m_Input->Index(), rH))
			SetCurrentState(pDynaModel, LS_MIN);
	return rH;

}

double CLimiterConst::OnStateMin(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_dMin - m_Input->Value(), m_Input->Value(), m_dMin, m_Input->Index(), rH))
		SetCurrentState(pDynaModel, LS_MID);
	return rH;
}

double CLimiterConst::OnStateMax(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_Input->Value() - m_dMax, m_Input->Value(), m_dMax, m_Input->Index(), rH))
		SetCurrentState(pDynaModel, LS_MID);
	return rH;
}

eDEVICEFUNCTIONSTATUS CLimiterConst::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dInput   = m_Input->Value();
		double CheckMax = dInput - m_dMax;
		double CheckMin = m_dMin - dInput;
		m_Output = m_Input->Value();

		// Bigger or Equal - very important !

		eLIMITEDSTATES OldState = GetCurrentState();

		if (CheckMax >= 0)
		{
			SetCurrentState(pDynaModel, LS_MAX);
			m_Output = m_dMax;
		}
		else if (CheckMin >= 0)
		{
			SetCurrentState(pDynaModel, LS_MIN);
			m_Output = m_dMin;
		}
		else
		{
			SetCurrentState(pDynaModel, LS_MID);
		}

		if (OldState != GetCurrentState())
			pDynaModel->DiscontinuityRequest();
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CLimiterConst::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double dMin(-1E6), dMax(-dMin);
	CDynaPrimitive::UnserializeParameters({ dMin, dMax }, Parameters);
	SetMinMax(pDynaModel, dMin, dMax);
	return true;
}

