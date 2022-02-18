#include "stdafx.h"
#include "DeadBand.h"
#include "DynaModel.h"

using namespace DFW2;

void CDeadBand::BuildEquations(CDynaModel *pDynaModel)
{
	switch (eDbState_)
	{
	case eDFW2DEADBANDSTATES::DBS_MAX:
	case eDFW2DEADBANDSTATES::DBS_MIN:
	case eDFW2DEADBANDSTATES::DBS_OFF:
		pDynaModel->SetElement(Output_, Input_, 1.0);
		break;
	case eDFW2DEADBANDSTATES::DBS_ZERO:
		pDynaModel->SetElement(Output_, Input_, 0.0);
		break;
	}

	pDynaModel->SetElement(Output_, Output_, 1.0);
}

void CDeadBand::BuildRightHand(CDynaModel *pDynaModel)
{
	if (Device_.IsStateOn())
	{
		switch (eDbState_)
		{
		case eDFW2DEADBANDSTATES::DBS_MIN:
			pDynaModel->SetFunction(Output_, Output_ - Input_ - Db_);
			break;
		case eDFW2DEADBANDSTATES::DBS_ZERO:
			pDynaModel->SetFunction(Output_, 0.0);
			pDynaModel->SetVariableNordsiek(Output_, 0.0);
			break;
		case eDFW2DEADBANDSTATES::DBS_OFF:
			pDynaModel->SetFunction(Output_, Output_ - Input_);
			break;
		case eDFW2DEADBANDSTATES::DBS_MAX:
			pDynaModel->SetFunction(Output_, Output_ - Input_ + Db_);
			break;
		}
	}
	else
		pDynaModel->SetFunction(Output_, 0.0);
}



bool CDeadBand::Init(CDynaModel *pDynaModel)
{
	bool bRes{ CDynaPrimitive::Init(pDynaModel) };
	if (Db_ < 0)
	{
		Device_.Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongDeadBandParameter, GetVerbalName(), Device_.GetVerbalName(), Db_));
		bRes = false;
	}
	else
		if (Equal(Db_,0.0))
			eDbState_ = eDFW2DEADBANDSTATES::DBS_OFF;
	else
	{
		DbMax_ = pDynaModel->GetHysteresis(Db_);
		DbMin_ = -Db_ - DbMax_;
		DbMax_ += Db_;
	}

	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

eDEVICEFUNCTIONSTATUS CDeadBand::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		if (eDbState_ != eDFW2DEADBANDSTATES::DBS_OFF)
		{
			pDynaModel->SetVariableNordsiek(Output_, 0.0);

			eDbState_ = eDFW2DEADBANDSTATES::DBS_ZERO;

			if (Input_ >= Db_)
			{
				pDynaModel->SetVariableNordsiek(Output_, Input_ - Db_);
				eDbState_ = eDFW2DEADBANDSTATES::DBS_MAX;
			}
			else
				if (Input_ <= -Db_)
				{
					pDynaModel->SetVariableNordsiek(Output_, Input_ + Db_);
					eDbState_ = eDFW2DEADBANDSTATES::DBS_MIN;
				}
		}
		else
			pDynaModel->CopyVariableNordsiek(Output_, Input_);
	}
	else
		Output_ = 0.0;

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

double CDeadBand::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };

	if (Device_.IsStateOn())
	{
		eDFW2DEADBANDSTATES OldState{ eDbState_ };

		switch (eDbState_)
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
		if (OldState != eDbState_)
		{
			pDynaModel->Log(DFW2MessageStatus::DFW2LOG_DEBUG,
							fmt::format("t={:15.012f} {:>3} Примитив {} из {} изменяет состояние {} {} {} с {} на {}", 
											pDynaModel->GetCurrentTime(), 
											pDynaModel->GetIntegrationStepNumber(), 
											GetVerbalName(), 
											Device_.GetVerbalName(), 
											Output_, 
											DbMin_, 
											DbMax_, 
											OldState, 
											eDbState_)
							);
			pDynaModel->DiscontinuityRequest(Device_, DiscontinuityLevel::Light);
		}
	}

	return rH;
}

bool CDeadBand::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	return CDynaPrimitive::UnserializeParameters({ {Db_, 0.0} }, Parameters);
}


void CDeadBand::SetCurrentState(eDFW2DEADBANDSTATES CurrentState)
{
	eDbState_ = CurrentState;
}

double CDeadBand::OnStateMin(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	RightVector* pRightVector1{ pDynaModel->GetRightVector(Input_) };
	const double CheckMin{ -Db_ - Input_ };

	if (CheckMin < 0.0)
	{
		const double derr{ std::abs(pRightVector1->GetWeightedError(CheckMin, Input_)) };
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(eDFW2DEADBANDSTATES::DBS_ZERO);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, -Db_);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(eDFW2DEADBANDSTATES::DBS_ZERO);
		}
	}

	return rH;
}

double CDeadBand::OnStateMax(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	RightVector *pRightVector1 = pDynaModel->GetRightVector(Input_);
	const double CheckMax{ Input_ - Db_ };

	if (CheckMax < 0.0)
	{
		const double derr{ std::abs(pRightVector1->GetWeightedError(CheckMax, Input_)) };
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(eDFW2DEADBANDSTATES::DBS_ZERO);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, Db_);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(eDFW2DEADBANDSTATES::DBS_ZERO);
		}
	}
	return rH;
}

double CDeadBand::OnStateZero(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	RightVector *pRightVector1 = pDynaModel->GetRightVector(Input_);

	const double CheckMax{ Input_ - DbMax_ };
	const double CheckMin{ DbMin_ - Input_ };

	if (CheckMax >= 0.0)
	{
		const double derr{ std::abs(pRightVector1->GetWeightedError(CheckMax, Input_)) };
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(eDFW2DEADBANDSTATES::DBS_MAX);
		}
		else
		{
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, DbMax_);
			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetCurrentState(eDFW2DEADBANDSTATES::DBS_MAX);
			}
		}
	}
	else
		if (CheckMin >= 0.0)
		{
			const double derr{ std::abs(pRightVector1->GetWeightedError(CheckMin, Input_)) };
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				SetCurrentState(eDFW2DEADBANDSTATES::DBS_MIN);
			}
			else
			{
				rH = FindZeroCrossingToConst(pDynaModel, pRightVector1, DbMin_);
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					SetCurrentState(eDFW2DEADBANDSTATES::DBS_MIN);
				}
			}
		}
	return rH;
}