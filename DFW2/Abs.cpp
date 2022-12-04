#include "stdafx.h"
#include "Abs.h"
#include "DynaModel.h"

using namespace DFW2;

void CAbs::BuildEquations(CDynaModel *pDynaModel)
{
	pDynaModel->SetElement(Output_, Input_, Positive_ ? 1.0 : -1.0);
	pDynaModel->SetElement(Output_, Output_, 1.0);
}

void CAbs::BuildRightHand(CDynaModel *pDynaModel)
{
	if (Device_.IsStateOn())
	{
		pDynaModel->SetFunction(Output_, Output_ - (Positive_ ? Input_ : -Input_));
	}
	else
		pDynaModel->SetFunction(Output_, 0.0);
}



bool CAbs::Init(CDynaModel *pDynaModel)
{
	bool bRes{ CDynaPrimitive::Init(pDynaModel) };
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

eDEVICEFUNCTIONSTATUS CAbs::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		if (Input_ >= 0)
			Positive_ = true;
		else
			Positive_ = false;

		pDynaModel->CopyVariableNordsiek(Output_, Input_);
		Output_ = std::abs(Input_);
	}
	else
		pDynaModel->SetVariableNordsiek(Output_, 0.0);

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

// проверяем разрыв модуля

double CAbs::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (!Device_.IsStateOn())
		return rH;

	const double dHyst{ pDynaModel->GetHysteresis(Input_) };
	if (Positive_)
	{
		if (CDynaPrimitive::ChangeState(pDynaModel, Input_ + dHyst, Input_ + dHyst, -dHyst, Input_.Index, rH))
		{
			Positive_ = false;
			pDynaModel->DiscontinuityRequest(Device_, DiscontinuityLevel::Light);
		}
	}
	else
	{
		if (CDynaPrimitive::ChangeState(pDynaModel, dHyst - Input_, dHyst - Input_, dHyst, Input_.Index, rH))
		{
			Positive_ = true;
			pDynaModel->DiscontinuityRequest(Device_, DiscontinuityLevel::Light);
		}
	}

	return rH;
}