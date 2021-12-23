#include "stdafx.h"
#include "DeadBand.h"
#include "DynaModel.h"

using namespace DFW2;

bool CDeadBand::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	switch (m_eDbState)
	{
	case eDFW2DEADBANDSTATES::DBS_MAX:
	case eDFW2DEADBANDSTATES::DBS_MIN:
	case eDFW2DEADBANDSTATES::DBS_OFF:
		pDynaModel->SetElement(m_Output, m_Input, 1.0);
		break;
	case eDFW2DEADBANDSTATES::DBS_ZERO:
		pDynaModel->SetElement(m_Output, m_Input, 0.0);
		break;
	}

	pDynaModel->SetElement(m_Output, m_Output, 1.0);

	return true;
}

bool CDeadBand::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		switch (m_eDbState)
		{
		case eDFW2DEADBANDSTATES::DBS_MIN:
			pDynaModel->SetFunction(m_Output, m_Output - m_Input - m_Db);
			break;
		case eDFW2DEADBANDSTATES::DBS_ZERO:
			pDynaModel->SetFunction(m_Output, 0.0);
			m_Output = 0.0;
			break;
		case eDFW2DEADBANDSTATES::DBS_OFF:
			pDynaModel->SetFunction(m_Output, m_Output - m_Input);
			break;
		case eDFW2DEADBANDSTATES::DBS_MAX:
			pDynaModel->SetFunction(m_Output, m_Output - m_Input + m_Db);
			break;
		}
	}
	else
		pDynaModel->SetFunction(m_Output, 0.0);

	return true;
}



bool CDeadBand::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitive::Init(pDynaModel);
	if (m_Db < 0)
	{
		m_Device.Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongDeadBandParameter, GetVerbalName(), m_Device.GetVerbalName(), m_Db));
		bRes = false;
	}
	else
		if (Equal(m_Db,0.0))
			m_eDbState = eDFW2DEADBANDSTATES::DBS_OFF;
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
	if (m_Device.IsStateOn())
	{
		if (m_eDbState != eDFW2DEADBANDSTATES::DBS_OFF)
		{
			m_Output = 0.0;

			m_eDbState = eDFW2DEADBANDSTATES::DBS_ZERO;

			if (m_Input >= m_Db)
			{
				m_Output = m_Input - m_Db;
				m_eDbState = eDFW2DEADBANDSTATES::DBS_MAX;
			}
			else
				if (m_Input <= -m_Db)
				{
					m_Output = m_Input + m_Db;
					m_eDbState = eDFW2DEADBANDSTATES::DBS_MIN;
				}
		}
		else
			m_Output = m_Input;
	}
	else
		m_Output = 0.0;

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

double CDeadBand::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	if (m_Device.IsStateOn())
	{
		eDFW2DEADBANDSTATES OldState = m_eDbState;

		switch (m_eDbState)
		{
		case eDFW2DEADBANDSTATES::DBS_MAX:
			rH = OnStateMax(pDynaModel);
			break;
		case eDFW2DEADBANDSTATES::DBS_MIN:
			rH = OnStateMin(pDynaModel);
			break;
		case eDFW2DEADBANDSTATES::DBS_ZERO:
			rH = OnStateZero(pDynaModel);
			break;
		case eDFW2DEADBANDSTATES::DBS_OFF:
			break;
		}
		if (OldState != m_eDbState)
		{
			pDynaModel->Log(DFW2MessageStatus::DFW2LOG_DEBUG,
							fmt::format("t={:15.012f} {:>3} Примитив {} из {} изменяет состояние {} {} {} с {} на {}", 
											pDynaModel->GetCurrentTime(), 
											pDynaModel->GetIntegrationStepNumber(), 
											GetVerbalName(), 
											m_Device.GetVerbalName(), 
											/*(const double)*/m_Output, 
											m_DbMin, 
											m_DbMax, 
											OldState, 
											m_eDbState)
							);
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
		}
	}

	return rH;
}

bool CDeadBand::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	return CDynaPrimitive::UnserializeParameters({ {m_Db, 0.0} }, Parameters);
}


void CDeadBand::SetCurrentState(eDFW2DEADBANDSTATES CurrentState)
{
	m_eDbState = CurrentState;
}

double CDeadBand::OnStateMin(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(m_Input);
	double CheckMin = -m_Db - m_Input;

	if (CheckMin < 0.0)
	{
		double derr = std::abs(pRightVector1->GetWeightedError(CheckMin, m_Input));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(eDFW2DEADBANDSTATES::DBS_ZERO);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, -m_Db);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(eDFW2DEADBANDSTATES::DBS_ZERO);
		}
	}

	return rH;
}

double CDeadBand::OnStateMax(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(m_Input);
	double CheckMax = m_Input - m_Db;

	if (CheckMax < 0.0)
	{
		double derr = std::abs(pRightVector1->GetWeightedError(CheckMax, m_Input));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(eDFW2DEADBANDSTATES::DBS_ZERO);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, m_Db);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(eDFW2DEADBANDSTATES::DBS_ZERO);
		}
	}
	return rH;
}

double CDeadBand::OnStateZero(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(m_Input);

	double CheckMax = m_Input - m_DbMax;
	double CheckMin = m_DbMin - m_Input;

	if (CheckMax >= 0.0)
	{
		double derr = std::abs(pRightVector1->GetWeightedError(CheckMax, m_Input));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(eDFW2DEADBANDSTATES::DBS_MAX);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, m_DbMax);
			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetCurrentState(eDFW2DEADBANDSTATES::DBS_MAX);
			}
		}
	}
	else
		if (CheckMin >= 0.0)
		{
			double derr = std::abs(pRightVector1->GetWeightedError(CheckMin, m_Input));
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				SetCurrentState(eDFW2DEADBANDSTATES::DBS_MIN);
			}
			else
			{
				rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, m_DbMin);
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					SetCurrentState(eDFW2DEADBANDSTATES::DBS_MIN);
				}
			}
		}
	return rH;
}