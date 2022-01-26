#include "stdafx.h"
#include "Expand.h"
#include "DynaModel.h"

using namespace DFW2;

bool CExpand::Init(CDynaModel *pDynaModel)
{
	eCurrentState = eRELAYSTATES::RS_OFF;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));;
}

CDynaPrimitiveBinary::eRELAYSTATES CExpand::GetInstantState()
{
	return m_Input > 0 ? eRELAYSTATES::RS_ON : eRELAYSTATES::RS_OFF;
}

eDEVICEFUNCTIONSTATUS CExpand::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		const CRelay::eRELAYSTATES State{ GetInstantState() };
		SetCurrentState(pDynaModel, State);
		pDynaModel->SetVariableNordsiek(m_Output, (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
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
			pDynaModel->SetVariableNordsiek(m_Output, 1.0);
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
	return pDynaModel->RemoveStateDiscontinuity(this);;
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
		const CRelay::eRELAYSTATES State{ GetInstantState() };
		SetCurrentState(pDynaModel, State);
		pDynaModel->SetVariableNordsiek(m_Output, (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
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
			const RightVector* pRightVector{ pDynaModel->GetRightVector(m_Input) };
			if (pRightVector->SavedNordsiek[0] <= 0)
			{
				pDynaModel->SetStateDiscontinuity(this, m_dDelay);
				eCurrentState = CurrentState;
				pDynaModel->SetVariableNordsiek(m_Output, 1.0);
			}
		}
		break;
	}
}