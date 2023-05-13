#include "stdafx.h"
#include "LimitedLag.h"
#include "DynaModel.h"

using namespace DFW2;

void CLimitedLag::BuildEquations(CDynaModel *pDynaModel)
{ 
	double on{ 1.0 / T_ };

	switch (GetCurrentState())
	{
	case eLIMITEDSTATES::Max:
		on = 0.0;
		pDynaModel->SetVariableNordsiek(Output_, Max_);
		break;
	case eLIMITEDSTATES::Min:
		on = 0.0;
		pDynaModel->SetVariableNordsiek(Output_, Min_);
		break;
	}

	if (!Device_.IsStateOn())
		on = 0.0;

	pDynaModel->SetElement(Output_, Output_, -on);
	pDynaModel->SetElement(Output_, Input_, -on * K_);
}


void CLimitedLag::BuildRightHand(CDynaModel *pDynaModel)
{
	if (Device_.IsStateOn())
	{
		double dLag{ (K_ * Input_ - Output_) / T_ };
		switch (GetCurrentState())
		{
		case eLIMITEDSTATES::Max:
			dLag = 0.0;
			pDynaModel->SetVariableNordsiek(Output_, Max_);
			break;
		case eLIMITEDSTATES::Min:
			dLag = 0.0;
			pDynaModel->SetVariableNordsiek(Output_, Min_);
			break;
		}
		pDynaModel->SetFunctionDiff(Output_, dLag);
	}
	else
		pDynaModel->SetFunctionDiff(Output_, 0.0);
}

bool CLimitedLag::Init(CDynaModel *pDynaModel)
{
	if (Equal(K_, 0.0))
	{
		Output_ = Min_ = Max_ = 0.0;
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
	}
	else
	{
		Input_ = Output_ / K_;
	}

	bool bRes{ CDynaPrimitiveLimited::Init(pDynaModel) };
	bRes = bRes && CheckTimeConstant(T_);
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}


void CLimitedLag::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (Device_.IsStateOn())
	{
		double dLag{ (K_ * Input_ - Output_) / T_ };
		switch (GetCurrentState())
		{
		case eLIMITEDSTATES::Max:
			dLag = 0.0;
			pDynaModel->SetVariableNordsiek(Output_, Max_);
			break;
		case eLIMITEDSTATES::Min:
			dLag = 0.0;
			pDynaModel->SetVariableNordsiek(Output_, Min_);
			break;
		}
		pDynaModel->SetDerivative(Output_, dLag);
	}
	else
		pDynaModel->SetDerivative(Output_, 0.0);
}

double CLimitedLag::OnStateMid(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, MaxH_ - Output_, Output_, MaxH_, Output_.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
	if (GetCurrentState() == eLIMITEDSTATES::Mid && !pDynaModel->GetZeroCrossingInRange(rH))
		if (CDynaPrimitive::ChangeState(pDynaModel, Output_ - MinH_, Output_, MinH_, Output_.Index, rH))
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Min);
	return rH;
}

double CLimitedLag::OnStateMin(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, Output_ - K_ * Input_, Output_, Output_ / K_, Input_.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
	return rH;
}

double CLimitedLag::OnStateMax(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, K_ * Input_ - Output_, Output_, Output_ / K_, Input_.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
	return rH;
}

eDEVICEFUNCTIONSTATUS CLimitedLag::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double dLag{ (K_ * Input_ - Output_) / T_ };
		const eLIMITEDSTATES OldState{ GetCurrentState() };
		switch (OldState)
		{
		case eLIMITEDSTATES::Min:
			if (dLag > 0)
				SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
			else
				pDynaModel->SetVariableNordsiek(Output_, Min_);
			break;
		case eLIMITEDSTATES::Max:
			if (dLag < 0)
				SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
			else
				pDynaModel->SetVariableNordsiek(Output_, Max_);
			break;
		}

		if (OldState != GetCurrentState())
			pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

void CLimitedLag::ChangeMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K)
{
	SetMinMax(pDynaModel, dMin, dMax);

	const double CheckMax{ Output_ - Max_ }, CheckMin{ Min_ - Output_ };

	const double dLag{ (K_ * Input_ - Output_) / T_ };

	if (CheckMax >= 0.0)
	{
		pDynaModel->SetVariableNordsiek(Output_, Max_);

		if (dLag >= 0.0)
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
	}
	else
		if (CheckMin >= 0.0)
		{
			pDynaModel->SetVariableNordsiek(Output_, Min_);

			if (dLag <= 0.0)
				SetCurrentState(pDynaModel, eLIMITEDSTATES::Min);
		}
		else
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
}

void CLimitedLag::SetMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K)
{
	SetMinMax(pDynaModel, dMin, dMax);
	T_ = T;
	K_ = K;
}

void CLimitedLag::ChangeTimeConstant(double TexcNew)
{
	T_ = TexcNew;
}

bool CLimitedLag::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double T(1E-4), K(1.0), dMin(-1E6), dMax(-dMin);
	CDynaPrimitive::UnserializeParameters({ T, K, dMin, dMax }, Parameters);
	SetMinMaxTK(pDynaModel, dMin, dMax, T, K);
	return true;
}