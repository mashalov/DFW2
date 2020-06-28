#include "stdafx.h"
#include "Expand.h"
#include "DynaModel.h"

using namespace DFW2;

bool CExpand::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	eCurrentState = RS_OFF;
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

CDynaPrimitiveBinary::eRELAYSTATES CExpand::GetInstantState()
{
	return m_Input->Value() > 0 ? RS_ON : RS_OFF;
}

eDEVICEFUNCTIONSTATUS CExpand::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		CRelay::eRELAYSTATES State = GetInstantState();
		SetCurrentState(pDynaModel, State);
		m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


void CExpand::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	switch (eCurrentState)
	{
	case RS_ON:
		if (CurrentState == RS_OFF)
		{
			if (m_dDelay > 0.0 && !pDynaModel->CheckStateDiscontinuity(this))
				eCurrentState = CurrentState;
		}
		break;
	case RS_OFF:
		if (CurrentState == RS_ON)
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
		pDynaModel->DiscontinuityRequest();
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
	SetCurrentState(pDynaModel, RS_OFF);
	return true;
}


eDEVICEFUNCTIONSTATUS CShrink::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		CRelay::eRELAYSTATES State = GetInstantState();
		SetCurrentState(pDynaModel, State);
		m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


void CShrink::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	switch (eCurrentState)
	{
	case RS_ON:
		if (CurrentState == RS_OFF)
		{
			pDynaModel->RemoveStateDiscontinuity(this);
			eCurrentState = CurrentState;
		}
		break;
	case RS_OFF:
		if (CurrentState == RS_ON)
		{
			RightVector *pRightVector = pDynaModel->GetRightVector(m_Input->Index());
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