#include "stdafx.h"
#include "Relay.h"
#include "DynaModel.h"

using namespace DFW2;

bool CRelay::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	SetRefs(pDynaModel, m_dUpper, m_dLower, m_bMaxRelay);
	eCurrentState = GetInstantState();
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

double CRelay::OnStateOff(CDynaModel *pDynaModel)
{
	double OnBound = m_dUpperH;
	double Check = m_Input->Value() - m_dUpperH;

	if (!m_bMaxRelay)
	{
		OnBound = m_dLowerH;
		Check = m_dLowerH - m_Input->Value();
	}

	return ChangeState(pDynaModel, Check, OnBound, RS_ON);
}

double CRelay::ChangeState(CDynaModel *pDynaModel, double Check, double OnBound, eRELAYSTATES SetState)
{
	RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index())); 
	double rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector, OnBound);

	if (pDynaModel->GetZeroCrossingInRange(rH))
	{
		if (Check >= 0)
		{
			double derr = fabs(pRightVector->GetWeightedError(Check, m_Input->Value()));
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				SetCurrentState(pDynaModel, SetState);
				rH = 1.0;
			}
		}
		else
		{
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(pDynaModel, SetState);
		}
	}
	else if (Check >= 0)
	{
		SetCurrentState(pDynaModel, SetState);
		rH = 1.0;
		_ASSERTE(0); // ����� ���, �� ���� ��������� !
	}
	else
		rH = 1.0;

	return rH;

}

double CRelay::OnStateOn(CDynaModel *pDynaModel)
{
	double OnBound = m_dLowerH;
	double Check = m_dLowerH - m_Input->Value();

	if (!m_bMaxRelay)
	{
		OnBound = m_dUpperH;
		Check = m_Input->Value() - m_dUpperH;
	}

	return ChangeState(pDynaModel, Check, OnBound, RS_OFF);
}


CRelay::eRELAYSTATES CRelay::GetInstantState()
{
	CRelay::eRELAYSTATES State = eCurrentState;
	double dInput = m_Input->Value();
	if (m_bMaxRelay)
	{
		if (eCurrentState == RS_OFF)
		{
			if (dInput > m_dUpper)
				State = RS_ON;
		}
		else
		{
			if (dInput <= m_dLower)
				State = RS_OFF;
		}
	}
	else
	{
		if (eCurrentState == RS_OFF)
		{
			if (dInput < m_dLower)
				State = RS_ON;
		}
		else
		{
			if (dInput >= m_dUpper)
				State = RS_OFF;
		}
	}
	return State;
}

eDEVICEFUNCTIONSTATUS CRelay::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		CRelay::eRELAYSTATES State = GetInstantState();
		SetCurrentState(pDynaModel, State);
		*m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
	}
	return DFS_OK;
}

bool CRelay::UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount)
{
	bool bRes = true;
	double RelayTol[2] = {0, 1};

	nParametersCount = min(nParametersCount, sizeof(RelayTol) / sizeof(RelayTol[0]) );

	for (size_t i = 0; i < nParametersCount; i++)
		RelayTol[i] = pParameters[i];

	SetRefs(pDynaModel, RelayTol[0], RelayTol[0] * RelayTol[1], true);
	return bRes;
}

void CRelay::SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay)
{
	m_dUpper = dUpper;
	m_dLower = dLower;
	m_bMaxRelay = MaxRelay;
	m_dUpperH = m_dUpper + pDynaModel->GetHysteresis(m_dUpper);
	m_dLowerH = m_dLower - pDynaModel->GetHysteresis(m_dLower);

}


void CRelayDelay::SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay, double dDelay)
{
	CRelay::SetRefs(pDynaModel, dUpper, dLower, MaxRelay);
	m_dDelay = dDelay;
}

bool CRelayDelay::UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount)
{
	bool bRes = true;
	double RelayDelayTol[3] = { 0.0, 0.0, 1.0 };

	nParametersCount = min(nParametersCount, sizeof(RelayDelayTol) / sizeof(RelayDelayTol[0]));

	for (size_t i = 0; i < nParametersCount; i++)
		RelayDelayTol[i] = pParameters[i];
	SetRefs(pDynaModel, RelayDelayTol[0], RelayDelayTol[0] * RelayDelayTol[2], true, RelayDelayTol[1]);
	return bRes;
}

bool CRelayDelay::Init(CDynaModel *pDynaModel)
{

	bool bRes = true;

	eCurrentState = RS_OFF;
	if (m_pDevice->IsStateOn())
	{
		eCurrentState = GetInstantState();
		bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	}
	return bRes && CDiscreteDelay::Init(pDynaModel);
}

void CRelayDelay::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	if (Equal(m_dDelay, 0.0))
	{
		CRelay::SetCurrentState(pDynaModel, CurrentState);
	}
	else
	{
		switch (eCurrentState)
		{
		case RS_ON:
			if (CurrentState == RS_OFF)
			{
				pDynaModel->RemoveStateDiscontinuity(this);

				if (*m_Output > 0.0)
					pDynaModel->DiscontinuityRequest();
			}
			break;
		case RS_OFF:
			if (CurrentState == RS_ON)
			{
				pDynaModel->SetStateDiscontinuity(this, m_dDelay);
			}
			break;
		}

		eCurrentState = CurrentState;
	}
}

eDEVICEFUNCTIONSTATUS CRelayDelay::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		CRelay::eRELAYSTATES OldState(eCurrentState);
		CRelay::eRELAYSTATES State = GetInstantState();

		SetCurrentState(pDynaModel, State);

		if ((m_dDelay > 0 && !pDynaModel->CheckStateDiscontinuity(this)) || m_dDelay == 0)
		{
			*m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
		}
	}
	return DFS_OK;
}


void CRelayDelay::RequestZCDiscontinuity(CDynaModel* pDynaModel)
{
	if (Equal(m_dDelay, 0.0))
		pDynaModel->DiscontinuityRequest();
}

bool CRelayDelay::NotifyDelay(CDynaModel *pDynaModel)
{
	pDynaModel->RemoveStateDiscontinuity(this);
	*m_Output = (GetCurrentState() == RS_ON) ? 1.0 : 0.0;
	return true;
}


bool CRelayDelayLogic::Init(CDynaModel *pDynaModel)
{
	bool bRes = CRelayDelay::Init(pDynaModel);
	if (bRes)
	{
		if (m_pDevice->IsStateOn())
		{
			if (eCurrentState == RS_ON && m_dDelay > 0)
			{
				pDynaModel->SetStateDiscontinuity(this, m_dDelay);
				eCurrentState = RS_OFF;
				*m_Output = 0;
			}
		}
	}
	return bRes;
}

bool CRelayDelayLogic::NotifyDelay(CDynaModel *pDynaModel)
{
	double dOldOut = *m_Output;
	CRelayDelay::NotifyDelay(pDynaModel);
	if (*m_Output != dOldOut)
		pDynaModel->NotifyRelayDelay(this);
	return true;
}


bool CDiscreteDelay::Init(CDynaModel *pDynaModel)
{
	return true;
}