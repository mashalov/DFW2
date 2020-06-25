#include "stdafx.h"
#include "DeadBand.h"
#include "DynaModel.h"

using namespace DFW2;

bool CDeadBand::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	switch (m_eDbState)
	{
	case DBS_MAX:
	case DBS_MIN:
	case DBS_OFF:
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), 1.0);
		break;
	case DBS_ZERO:
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), 0.0);
		break;
	}

	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);

	return true;
}

bool CDeadBand::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dInput = m_Input->Value();

		switch (m_eDbState)
		{
		case DBS_MIN:
			pDynaModel->SetFunction(A(m_OutputEquationIndex), *m_Output - dInput - m_Db);
			break;
		case DBS_ZERO:
			pDynaModel->SetFunction(A(m_OutputEquationIndex), 0.0);
			*m_Output = 0.0;
			break;
		case DBS_OFF:
			pDynaModel->SetFunction(A(m_OutputEquationIndex), *m_Output - dInput);
			break;
		case DBS_MAX:
			pDynaModel->SetFunction(A(m_OutputEquationIndex), *m_Output - dInput + m_Db);
			break;
		}
	}
	else
		pDynaModel->SetFunction(A(m_OutputEquationIndex), 0.0);

	return true;
}



bool CDeadBand::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitive::Init(pDynaModel);
	if (m_Db < 0)
	{
		m_pDevice->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongDeadBandParameter, GetVerbalName(), m_pDevice->GetVerbalName(), m_Db));
		bRes = false;
	}
	else
		if (Equal(m_Db,0.0))
			m_eDbState = DBS_OFF;
	else
	{
		m_DbMax = pDynaModel->GetHysteresis(m_Db);
		m_DbMin = -m_Db - m_DbMax;
		m_DbMax += m_Db;
	}

	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

eDEVICEFUNCTIONSTATUS CDeadBand::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dInput = m_Input->Value();

		if (m_eDbState != DBS_OFF)
		{
			*m_Output = 0.0;

			m_eDbState = DBS_ZERO;

			if (dInput >= m_Db)
			{
				*m_Output = dInput - m_Db;
				m_eDbState = DBS_MAX;
			}
			else
				if (dInput <= -m_Db)
				{
					*m_Output = dInput + m_Db;
					m_eDbState = DBS_MIN;
				}
		}
		else
			*m_Output = dInput;
	}
	else
		*m_Output = 0.0;

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

double CDeadBand::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	if (m_pDevice->IsStateOn())
	{
		DFW2DEADBANDSTATES OldState = m_eDbState;

		switch (m_eDbState)
		{
		case DBS_MAX:
			rH = OnStateMax(pDynaModel);
			break;
		case DBS_MIN:
			rH = OnStateMin(pDynaModel);
			break;
		case DBS_ZERO:
			rH = OnStateZero(pDynaModel);
			break;
		case DBS_OFF:
			break;
		}
		if (OldState != m_eDbState)
		{
			pDynaModel->Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("t=%.12g (%d) Примитив %s из %s изменяет состояние %g %g %g с %d на %d"), pDynaModel->GetCurrentTime(), pDynaModel->GetIntegrationStepNumber(), GetVerbalName(), m_pDevice->GetVerbalName(), *m_Output, m_DbMin, m_DbMax, OldState, m_eDbState);
			pDynaModel->DiscontinuityRequest();
		}
	}

	return rH;
}

bool CDeadBand::UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount)
{
	m_Db = 0.0;

	if (nParametersCount == 1)
		m_Db = pParameters[0];

	return true;
}


void CDeadBand::SetCurrentState(DFW2DEADBANDSTATES CurrentState)
{
	m_eDbState = CurrentState;
}

double CDeadBand::OnStateMin(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));
	double dInput = m_Input->Value();
	double CheckMin = -m_Db - dInput;

	if (CheckMin < 0.0)
	{
		double derr = fabs(pRightVector1->GetWeightedError(CheckMin, dInput));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(DBS_ZERO);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, -m_Db);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(DBS_ZERO);
		}
	}

	return rH;
}

double CDeadBand::OnStateMax(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));

	double dInput = m_Input->Value();
	double CheckMax = dInput - m_Db;

	if (CheckMax < 0.0)
	{
		double derr = fabs(pRightVector1->GetWeightedError(CheckMax, dInput));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(DBS_ZERO);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, m_Db);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(DBS_ZERO);
		}
	}
	return rH;
}

double CDeadBand::OnStateZero(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));

	double dInput = m_Input->Value();

	double CheckMax = dInput - m_DbMax;
	double CheckMin = m_DbMin - dInput;

	if (CheckMax >= 0.0)
	{
		double derr = fabs(pRightVector1->GetWeightedError(CheckMax, dInput));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(DBS_MAX);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, m_DbMax);
			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetCurrentState(DBS_MAX);
			}
		}
	}
	else
		if (CheckMin >= 0.0)
		{
			double derr = fabs(pRightVector1->GetWeightedError(CheckMin, dInput));
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				SetCurrentState(DBS_MIN);
			}
			else
			{
				rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, m_DbMin);
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					SetCurrentState(DBS_MIN);
				}
			}
		}
	return rH;
}