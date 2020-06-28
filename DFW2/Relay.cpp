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
	double Check = m_dUpperH - m_Input->Value();

	if (!m_bMaxRelay)
	{
		OnBound = m_dLowerH;
		Check = m_Input->Value() - m_dLowerH;
	}

	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, Check, m_Input->Value(), OnBound, m_Input->Index(), rH))
		SetCurrentState(pDynaModel, RS_ON);

	return rH; 
}

double CRelay::OnStateOn(CDynaModel *pDynaModel)
{
	double OnBound = m_dLowerH;
	double Check = m_Input->Value() - m_dLowerH;

	if (!m_bMaxRelay)
	{
		OnBound = m_dUpperH;
		Check = m_dUpperH - m_Input->Value();
	}

	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, Check, m_Input->Value(), OnBound, m_Input->Index(), rH))
		SetCurrentState(pDynaModel, RS_OFF);
	return rH;

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
		m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CRelay::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double Rt0(0.0), Rt1(1.0);
	CDynaPrimitive::UnserializeParameters({ Rt0, Rt1 }, Parameters);
	SetRefs(pDynaModel, Rt0, Rt0 * Rt1, true);
	return true;
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

bool CRelayDelay::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double Rt0(0.0), Rt1(0.0), Rt2(1.0);
	CDynaPrimitive::UnserializeParameters({Rt0, Rt1, Rt2}, Parameters);
	SetRefs(pDynaModel, Rt0, Rt0 * Rt2, true, Rt1);
	return true;
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

				if (m_Output > 0.0)
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
			m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
		}
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


void CRelayDelay::RequestZCDiscontinuity(CDynaModel* pDynaModel)
{
	if (Equal(m_dDelay, 0.0))
		pDynaModel->DiscontinuityRequest();
}

bool CRelayDelay::NotifyDelay(CDynaModel *pDynaModel)
{
	pDynaModel->RemoveStateDiscontinuity(this);
	m_Output = (GetCurrentState() == RS_ON) ? 1.0 : 0.0;
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
				m_Output = 0;
			}
		}
	}
	return bRes;
}

bool CRelayDelayLogic::NotifyDelay(CDynaModel *pDynaModel)
{
	double dOldOut = m_Output;
	CRelayDelay::NotifyDelay(pDynaModel);
	if (m_Output != dOldOut)
		pDynaModel->NotifyRelayDelay(this);
	return true;
}


bool CDiscreteDelay::Init(CDynaModel *pDynaModel)
{
	return true;
}