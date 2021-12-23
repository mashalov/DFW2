﻿#include "stdafx.h"
#include "LimiterConst.h"
#include "DynaModel.h"

using namespace DFW2;

bool CLimiterConst::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double dOdI = -1.0;

	switch (GetCurrentState())
	{
	case eLIMITEDSTATES::LS_MAX:
		m_Output = m_dMax;
		dOdI = 0.0;
		break;
	case eLIMITEDSTATES::LS_MIN:
		m_Output = m_dMin;
		dOdI = 0.0;
		break;
	}

	if (!m_Device.IsStateOn())
		dOdI = 0.0;

	pDynaModel->SetElement(m_Output, m_Output, 1.0);
	pDynaModel->SetElement(m_Output, m_Input, dOdI);
	return true;
}

bool CLimiterConst::BuildRightHand(CDynaModel *pDynaModel)
{
	double dOdI = m_Output - m_Input;

	if (m_Device.IsStateOn())
	{
		switch (GetCurrentState())
		{
		case eLIMITEDSTATES::LS_MAX:
			m_Output = m_dMax;
			dOdI = 0.0;
			break;
		case eLIMITEDSTATES::LS_MIN:
			m_Output = m_dMin;
			dOdI = 0.0;
			break;
		}
		pDynaModel->SetFunction(m_Output, dOdI);
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
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_dMaxH - m_Input, m_Input, m_dMaxH, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::LS_MAX);
	if (GetCurrentState() == eLIMITEDSTATES::LS_MID && !pDynaModel->GetZeroCrossingInRange(rH))
		if (CDynaPrimitive::ChangeState(pDynaModel, m_Input - m_dMinH, m_Input, m_dMinH, m_Input.Index, rH))
			SetCurrentState(pDynaModel, eLIMITEDSTATES::LS_MIN);
	return rH;

}

double CLimiterConst::OnStateMin(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_dMin - m_Input, m_Input, m_dMin, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::LS_MID);
	return rH;
}

double CLimiterConst::OnStateMax(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_Input - m_dMax, m_Input, m_dMax, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::LS_MID);
	return rH;
}

eDEVICEFUNCTIONSTATUS CLimiterConst::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double CheckMax = m_Input - m_dMax;
		double CheckMin = m_dMin - m_Input;
		m_Output = m_Input;

		// Bigger or Equal - very important !

		eLIMITEDSTATES OldState = GetCurrentState();

		if (CheckMax >= 0)
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::LS_MAX);
			m_Output = m_dMax;
		}
		else if (CheckMin >= 0)
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::LS_MIN);
			m_Output = m_dMin;
		}
		else
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::LS_MID);
		}

		if (OldState != GetCurrentState())
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
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

