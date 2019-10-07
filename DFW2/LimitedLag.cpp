#include "stdafx.h"
#include "LimitedLag.h"
#include "DynaModel.h"

using namespace DFW2;

bool CLimitedLag::BuildEquations(CDynaModel *pDynaModel)
{ 
	bool bRes = true;

	double on = 1.0 / m_T;

	switch (GetCurrentState())
	{
	case LS_MAX:
		on = 0.0;
		*m_Output = m_dMax;
		break;
	case LS_MIN:
		on = 0.0;
		*m_Output = m_dMin;
		break;
	}

	if (!m_pDevice->IsStateOn())
		on = 0.0;

	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), -on);
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), -on * m_K);

	return true;
}


bool CLimitedLag::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dLag = (m_K * m_Input->Value() - *m_Output) / m_T;
		switch (GetCurrentState())
		{
		case LS_MAX:
			dLag = 0.0;
			*m_Output = m_dMax;
			break;
		case LS_MIN:
			dLag = 0.0;
			*m_Output = m_dMin;
			break;
		}
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex), dLag);
	}
	else
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex), 0.0);

	return true;
}

bool CLimitedLag::Init(CDynaModel *pDynaModel)
{
	if (Equal(m_K, 0.0))
	{
		*m_Output = m_dMin = m_dMax = 0.0;
		SetCurrentState(pDynaModel, LS_MAX);
	}
	else
		m_Input->Value() = *m_Output / m_K;

	bool bRes = CDynaPrimitiveLimited::Init(pDynaModel);

	if (bRes)
	{
		if (Equal(m_T,0.0))
		{
			m_pDevice->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongPrimitiveTimeConstant, GetVerbalName(), m_pDevice->GetVerbalName(), m_T));
			bRes = false;
		}
	}

	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));

	return bRes;
}


bool CLimitedLag::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dLag = (m_K * m_Input->Value() - *m_Output) / m_T;
		switch (GetCurrentState())
		{
		case LS_MAX:
			dLag = 0.0;
			*m_Output = m_dMax;
			break;
		case LS_MIN:
			dLag = 0.0;
			*m_Output = m_dMin;
			break;
		}
		pDynaModel->SetDerivative(A(m_OutputEquationIndex), dLag);
	}
	else
		pDynaModel->SetDerivative(A(m_OutputEquationIndex), 0.0);

	return true;
}

double CLimitedLag::OnStateMid(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_dMaxH - *m_Output, *m_Output, m_dMaxH, m_OutputEquationIndex, rH))
		SetCurrentState(pDynaModel, LS_MAX);
	if (GetCurrentState() == LS_MID && !pDynaModel->GetZeroCrossingInRange(rH))
		if (CDynaPrimitive::ChangeState(pDynaModel, *m_Output - m_dMinH, *m_Output, m_dMinH, m_OutputEquationIndex, rH))
			SetCurrentState(pDynaModel, LS_MIN);
	return rH;
}

double CLimitedLag::OnStateMin(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, *m_Output - m_K * m_Input->Value(), *m_Output, *m_Output / m_K, m_Input->Index(), rH))
		SetCurrentState(pDynaModel, LS_MID);
	return rH;
}

double CLimitedLag::OnStateMax(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, m_K * m_Input->Value() - *m_Output, *m_Output, *m_Output / m_K, m_Input->Index(), rH))
		SetCurrentState(pDynaModel, LS_MID);
	return rH;
}

eDEVICEFUNCTIONSTATUS CLimitedLag::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dLag = (m_K * m_Input->Value() - *m_Output) / m_T;
		eLIMITEDSTATES OldState = GetCurrentState();
		switch (OldState)
		{
		case LS_MIN:
			if (dLag > 0)
				SetCurrentState(pDynaModel, LS_MID);
			else
				*m_Output = m_dMin;
			break;
		case LS_MAX:
			if (dLag < 0)
				SetCurrentState(pDynaModel, LS_MID);
			else
				*m_Output = m_dMax;
			break;
		}

		if (OldState != GetCurrentState())
			pDynaModel->DiscontinuityRequest();
	}

	return DFS_OK;
}

void CLimitedLag::ChangeMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K)
{
	SetMinMax(pDynaModel, dMin, dMax);

	double CheckMax = *m_Output - m_dMax;
	double CheckMin = m_dMin - *m_Output;

	double dLag = (m_K * m_Input->Value() - *m_Output) / m_T;

	if (CheckMax >= 0.0)
	{
		*m_Output = m_dMax;

		if(dLag >= 0.0)
			SetCurrentState(pDynaModel, LS_MAX);
	}
	else
		if (CheckMin >= 0.0 )
		{
			*m_Output = m_dMin;

			if (dLag <= 0.0)
				SetCurrentState(pDynaModel, LS_MIN);
		}
		else
			SetCurrentState(pDynaModel, LS_MID);
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

bool CLimitedLag::UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount)
{
	bool bRes = true;
	double TKMinMax[4] = { 1E-4, 1.0, -1E6, 1E6};

	nParametersCount = min(nParametersCount, sizeof(TKMinMax) / sizeof(TKMinMax[0]));

	for (size_t i = 0; i < nParametersCount; i++)
		TKMinMax[i] = pParameters[i];
	SetMinMaxTK(pDynaModel, TKMinMax[2], TKMinMax[3], TKMinMax[0], TKMinMax[1]);
	return bRes;
}