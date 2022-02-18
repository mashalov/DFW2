#include "stdafx.h"
#include "Sum.h"
#include "DynaModel.h"

using namespace DFW2;

void CSum::BuildEquations(CDynaModel *pDynaModel)
{
	pDynaModel->SetElement(Output_, Input_, K1_);
	pDynaModel->SetElement(Output_, Input1_, K2_);
	pDynaModel->SetElement(Output_, Output_, -1.0);
}


void CSum::BuildRightHand(CDynaModel *pDynaModel)
{
	pDynaModel->SetFunction(Output_, K1_ * Input_ + K2_ * Input1_ - Output_);
}


