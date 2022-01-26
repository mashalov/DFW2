#include "stdafx.h"
#include "LimitedLag.h"
#include "DynaModel.h"

using namespace DFW2;

void CLimitedLag::BuildEquations(CDynaModel *pDynaModel)
{ 
	double on{ 1.0 / m_T };

	switch (GetCurrentState())
	{
	case eLIMITEDSTATES::Max:
		on = 0.0;
		pDynaModel->SetVariableNordsiek(m_Output, m_dMax);
		break;
	case eLIMITEDSTATES::Min:
		on = 0.0;
		pDynaModel->SetVariableNordsiek(m_Output, m_dMin);
		break;
	}

	if (!m_Device.IsStateOn())
		on = 0.0;

	pDynaModel->SetElement(m_Output, m_Output, -on);
	pDynaModel->SetElement(m_Output, m_Input, -on * m_K);
}


void CLimitedLag::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dLag{ (m_K * m_Input - m_Output) / m_T };
		switch (GetCurrentState())
		{
		case eLIMITEDSTATES::Max:
			dLag = 0.0;
			pDynaModel->SetVariableNordsiek(m_Output, m_dMax);
			break;
		case eLIMITEDSTATES::Min:
			dLag = 0.0;
			pDynaModel->SetVariableNordsiek(m_Output, m_dMin);
			break;
		}
		pDynaModel->SetFunctionDiff(m_Output, dLag);
	}
	else
		pDynaModel->SetFunctionDiff(m_Output, 0.0);
}

bool CLimitedLag::Init(CDynaModel *pDynaModel)
{
	if (Equal(m_K, 0.0))
	{
		m_Output = m_dMin = m_dMax = 0.0;
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
	}
	else
	{
		m_Input = m_Output / m_K;
	}

	bool bRes = CDynaPrimitiveLimited::Init(pDynaModel);

	if (bRes)
	{
		if (Equal(m_T,0.0))
		{
			m_Device.Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongPrimitiveTimeConstant,
																   GetVerbalName(), 
																   m_Device.GetVerbalName(), 
																   m_T));
			bRes = false;
		}
	}

	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));

	return bRes;
}


void CLimitedLag::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dLag{ (m_K * m_Input - m_Output) / m_T };
		switch (GetCurrentState())
		{
		case eLIMITEDSTATES::Max:
			dLag = 0.0;
			pDynaModel->SetVariableNordsiek(m_Output, m_dMax);
			break;
		case eLIMITEDSTATES::Min:
			dLag = 0.0;
			pDynaModel->SetVariableNordsiek(m_Output, m_dMin);
			break;
		}
		pDynaModel->SetDerivative(m_Output, dLag);
	}
	else
		pDynaModel->SetDerivative(m_Output, 0.0);
}

double CLimitedLag::OnStateMid(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_dMaxH - m_Output, m_Output, m_dMaxH, m_Output.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
	if (GetCurrentState() == eLIMITEDSTATES::Mid && !pDynaModel->GetZeroCrossingInRange(rH))
		if (CDynaPrimitive::ChangeState(pDynaModel, m_Output - m_dMinH, m_Output, m_dMinH, m_Output.Index, rH))
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Min);
	return rH;
}

double CLimitedLag::OnStateMin(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_Output - m_K * m_Input, m_Output, m_Output / m_K, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
	return rH;
}

double CLimitedLag::OnStateMax(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_K * m_Input - m_Output, m_Output, m_Output / m_K, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
	return rH;
}

eDEVICEFUNCTIONSTATUS CLimitedLag::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		const double dLag{ (m_K * m_Input - m_Output) / m_T };
		const eLIMITEDSTATES OldState{ GetCurrentState() };
		switch (OldState)
		{
		case eLIMITEDSTATES::Min:
			if (dLag > 0)
				SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
			else
				pDynaModel->SetVariableNordsiek(m_Output, m_dMin);
			break;
		case eLIMITEDSTATES::Max:
			if (dLag < 0)
				SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
			else
				pDynaModel->SetVariableNordsiek(m_Output, m_dMax);
			break;
		}

		if (OldState != GetCurrentState())
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

void CLimitedLag::ChangeMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K)
{
	SetMinMax(pDynaModel, dMin, dMax);

	const double CheckMax{ m_Output - m_dMax }, CheckMin{ m_dMin - m_Output };

	const double dLag{ (m_K * m_Input - m_Output) / m_T };

	if (CheckMax >= 0.0)
	{
		pDynaModel->SetVariableNordsiek(m_Output, m_dMax);

		if (dLag >= 0.0)
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
	}
	else
		if (CheckMin >= 0.0)
		{
			pDynaModel->SetVariableNordsiek(m_Output, m_dMin);

			if (dLag <= 0.0)
				SetCurrentState(pDynaModel, eLIMITEDSTATES::Min);
		}
		else
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
}

void CLimitedLag::SetMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K)
{
	SetMinMax(pDynaModel, dMin, dMax);
	m_T = T;
	m_K = K;
}

void CLimitedLag::ChangeTimeConstant(double TexcNew)
{
	m_T = TexcNew;
}

bool CLimitedLag::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double T(1E-4), K(1.0), dMin(-1E6), dMax(-dMin);
	CDynaPrimitive::UnserializeParameters({ T, K, dMin, dMax }, Parameters);
	SetMinMaxTK(pDynaModel, dMin, dMax, T, K);
	return true;
}