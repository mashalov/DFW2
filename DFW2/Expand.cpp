#include "stdafx.h"
#include "Expand.h"
#include "DynaModel.h"

using namespace DFW2;

bool CExpand::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	eCurrentState = eRELAYSTATES::RS_OFF;
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

CDynaPrimitiveBinary::eRELAYSTATES CExpand::GetInstantState()
{
	return m_Input > 0 ? eRELAYSTATES::RS_ON : eRELAYSTATES::RS_OFF;
}

eDEVICEFUNCTIONSTATUS CExpand::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		CRelay::eRELAYSTATES State = GetInstantState();
		SetCurrentState(pDynaModel, State);
		m_Output = (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0;
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


void CExpand::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	switch (eCurrentState)
	{
	case eRELAYSTATES::RS_ON:
		if (CurrentState == eRELAYSTATES::RS_OFF)
		{
			if (m_dDelay > 0.0 && !pDynaModel->CheckStateDiscontinuity(this))
				eCurrentState = CurrentState;
		}
		break;
	case eRELAYSTATES::RS_OFF:
		if (CurrentState == eRELAYSTATES::RS_ON)
		{
			if (m_dDelay > 0.0)
			{
				pDynaModel->RemoveStateDiscontinuity(this);
				pDynaModel->SetStateDiscontinuity(this, m_dDelay);
			}
			eCurrentState = CurrentState;
			m_Output = 1.0;
		}
		break;
	}
}

void CExpand::RequestZCDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_dDelay <= 0.0)
		pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
}

bool CExpand::NotifyDelay(CDynaModel *pDynaModel)
{
	pDynaModel->RemoveStateDiscontinuity(this);
	return true;
}

bool CExpand::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	return CDynaPrimitive::UnserializeParameters({ {m_dDelay,0.0} }, Parameters);
}



bool CShrink::NotifyDelay(CDynaModel *pDynaModel)
{
	SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);
	return true;
}


eDEVICEFUNCTIONSTATUS CShrink::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		CRelay::eRELAYSTATES State = GetInstantState();
		SetCurrentState(pDynaModel, State);
		m_Output = (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0;
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


void CShrink::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	switch (eCurrentState)
	{
	case eRELAYSTATES::RS_ON:
		if (CurrentState == eRELAYSTATES::RS_OFF)
		{
			pDynaModel->RemoveStateDiscontinuity(this);
			eCurrentState = CurrentState;
		}
		break;
	case eRELAYSTATES::RS_OFF:
		if (CurrentState == eRELAYSTATES::RS_ON)
		{
			RightVector *pRightVector = pDynaModel->GetRightVector(m_Input);
			if (pRightVector->SavedNordsiek[0] <= 0)
			{
				pDynaModel->SetStateDiscontinuity(this, m_dDelay);
				eCurrentState = CurrentState;
				m_Output = 1.0;
			}
		}
		break;
	}
}