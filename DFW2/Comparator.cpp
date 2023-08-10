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
	if (Device_.IsStateOn())
	{
		SetCurrentState(pDynaModel, (Input_ > Input1_) ? eRELAYSTATES::RS_ON : eRELAYSTATES::RS_OFF);
		pDynaModel->SetVariableNordsiek(Output_, (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
	}
	else
		pDynaModel->SetVariableNordsiek(Output_, 0.0);

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

double CComparator::OnStateOn(CDynaModel *pDynaModel)
{
	const double dCheck{ Input_ - Input1_ };
	double rH{ 1.0 };

	if (dCheck < 0)
	{
		const RightVector* pRightVector1{ pDynaModel->GetRightVector(Input_) };
		const RightVector* pRightVector2{ pDynaModel->GetRightVector(Input1_) };

		rH = pDynaModel->FindZeroCrossingOfDifference(pRightVector1, pRightVector2);
		if (pDynaModel->GetZeroCrossingInRange(rH))
		{
			const double derr{ std::abs(pRightVector1->GetWeightedError(dCheck, Input1_)) };
			if (derr < pDynaModel->ZeroCrossingTolerance())
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
	const double dCheck{ Input1_ - Input_ };
	double rH{ 1.0 };

	if (dCheck < 0)
	{
		const RightVector* pRightVector1{ pDynaModel->GetRightVector(Input_) };
		const RightVector* pRightVector2{ pDynaModel->GetRightVector(Input1_) };

		rH = pDynaModel->FindZeroCrossingOfDifference(pRightVector1, pRightVector2);
		if (pDynaModel->GetZeroCrossingInRange(rH))
		{
			const double derr{ std::abs(pRightVector1->GetWeightedError(dCheck, Input1_)) };
			if (derr < pDynaModel->ZeroCrossingTolerance())
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