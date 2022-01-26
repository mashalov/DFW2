#include "stdafx.h"
#include "Comparator.h"
#include "DynaModel.h"

using namespace DFW2;

bool CComparator::Init(CDynaModel *pDynaModel)
{
	eCurrentState = eRELAYSTATES::RS_OFF;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));;
}

eDEVICEFUNCTIONSTATUS CComparator::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		SetCurrentState(pDynaModel, (m_Input > m_Input1) ? eRELAYSTATES::RS_ON : eRELAYSTATES::RS_OFF);
		pDynaModel->SetVariableNordsiek(m_Output, (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
	}
	else
		pDynaModel->SetVariableNordsiek(m_Output, 0.0);

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

double CComparator::OnStateOn(CDynaModel *pDynaModel)
{
	const RightVector* pRightVector1{ pDynaModel->GetRightVector(m_Input) };
	const RightVector* pRightVector2{ pDynaModel->GetRightVector(m_Input1) };
	const double dCheck{ m_Input - m_Input1 };
	double rH(1.0);

	if (dCheck < 0)
	{
		rH = CDynaPrimitiveBinaryOutput::FindZeroCrossingOfDifference(pDynaModel, pRightVector1, pRightVector2);
		if (pDynaModel->GetZeroCrossingInRange(rH))
		{
			const double derr{ std::abs(pRightVector1->GetWeightedError(dCheck, m_Input1)) };
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);
				rH = 1.0;
			}
			else
			{
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);
					rH = 1.0;
				}
			}
		}
		else
		{
			SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);
			rH = 1.0;
			_ASSERTE(0); // корня нет, но знак изменился !
		}
	}

	return rH;
}

double CComparator::OnStateOff(CDynaModel *pDynaModel)
{
	const RightVector* pRightVector1{ pDynaModel->GetRightVector(m_Input) };
	const RightVector* pRightVector2{ pDynaModel->GetRightVector(m_Input1) };
	double dCheck = m_Input1 - m_Input;
	double rH(1.0);

	if (dCheck < 0)
	{
		rH = CDynaPrimitiveBinaryOutput::FindZeroCrossingOfDifference(pDynaModel, pRightVector1, pRightVector2);
		if (pDynaModel->GetZeroCrossingInRange(rH))
		{
			const double derr{ std::abs(pRightVector1->GetWeightedError(dCheck, m_Input1)) };
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				SetCurrentState(pDynaModel, eRELAYSTATES::RS_ON);
				rH = 1.0;
			}
			else
			{
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					SetCurrentState(pDynaModel, eRELAYSTATES::RS_ON);
					rH = 1.0;
				}
			}
		}
		else
		{
			SetCurrentState(pDynaModel, eRELAYSTATES::RS_ON);
			rH = 1.0;
			_ASSERTE(0); // корня нет, но знак изменился !
		}
	}

	return rH;
}