#include "stdafx.h"
#include "LeadLag.h"
#include "DynaModel.h"

using namespace DFW2;

void CLeadLag::BuildEquations(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		// Output_ - Y2_ - T1_ * (Input_ - Y2_) / T2_;
		pDynaModel->SetElement(Output_, Output_, 1.0);
		pDynaModel->SetElement(Output_, Y2_, T1_ / T2_ - 1.0);
		pDynaModel->SetElement(Output_, Input_, -T1_ / T2_);
		// der(Y2_)  =  (Input_  -  Y2_)  / T2_;
		pDynaModel->SetElement(Y2_, Y2_, -1.0 / T2_);
		pDynaModel->SetElement(Y2_, Input_, -1.0 / T2_);
	}
	else
	{
		pDynaModel->SetElement(Output_, Output_, 0.0);
		pDynaModel->SetElement(Output_, Input_, 0.0);
		pDynaModel->SetElement(Output_, Y2_, 0.0);
		pDynaModel->SetElement(Y2_, Y2_, 0.0);
		pDynaModel->SetElement(Y2_, Input_, 0.0);
	}
}


void CLeadLag::BuildRightHand(CDynaModel* pDynaModel)
{
	// der(Y2_)  =  (Input_  -  Y2_)  / T2_;
	//  Output_  = Y2_ + T1 * (Input_  - Y2_) / T2_;

	if (Device_.IsStateOn())
	{
		const double Diff{ (Input_ - Y2_) / T2_ };
		pDynaModel->SetFunctionDiff(Y2_, Diff);
		pDynaModel->SetFunction(Output_, Output_  - (T1_ * Diff + Y2_));
	}
	else
	{
		pDynaModel->SetFunctionDiff(Y2_, 0.0);
		pDynaModel->SetFunction(Output_, 0.0);
	}
}

bool CLeadLag::Init(CDynaModel* pDynaModel)
{
	Output_ = Y2_ = Input_;
	bool bRes{ CDynaPrimitive::Init(pDynaModel) };
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

void CLeadLag::BuildDerivatives(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double Diff{ (Input_ - Y2_) / T2_ };
		pDynaModel->SetFunctionDiff(Y2_, Diff);
	}
	else
		pDynaModel->SetFunctionDiff(Y2_, 0.0);
}

eDEVICEFUNCTIONSTATUS CLeadLag::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	Output_ = 0.0;
	if (Device_.IsStateOn())
		Output_ = Y2_ - (Input_ - Y2_) / T2_ * T1_;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CLeadLag::ChangeTimeConstants(double T1, double  T2)
{
	const bool bRes{ CheckTimeConstant(T2) };
	if(bRes)
	{
		T1_ = T1;
		T2_ = T2;
	}
	return bRes;
}

bool CLeadLag::UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double T1{ 1e-4 }, T2{ 1e-4 };
	CDynaPrimitive::UnserializeParameters({ T1, T2 }, Parameters);
	return ChangeTimeConstants(T1, T2);
}