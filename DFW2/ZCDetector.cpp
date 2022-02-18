#include "stdafx.h"
#include "ZCDetector.h"
#include "DynaModel.h"

using namespace DFW2;

double CZCDetector::OnStateOff(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	const double dHyst{ pDynaModel->GetHysteresis(Input_) };
	if (CDynaPrimitive::ChangeState(pDynaModel, dHyst - Input_, dHyst - Input_, dHyst, Input_.Index, rH))
		SetCurrentState(pDynaModel, eRELAYSTATES::RS_ON);
	return rH;
}

double CZCDetector::OnStateOn(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	const double dHyst{ pDynaModel->GetHysteresis(Input_) };
	if (CDynaPrimitive::ChangeState(pDynaModel, Input_ + dHyst, Input_ + dHyst, -dHyst, Input_.Index, rH))
		SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);
	return rH;
}

bool CZCDetector::Init(CDynaModel *pDynaModel)
{
	eCurrentState = eRELAYSTATES::RS_OFF;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));;
}

eDEVICEFUNCTIONSTATUS CZCDetector::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		if (Input_ > 0)
			SetCurrentState(pDynaModel, eRELAYSTATES::RS_ON);
		else
			SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);

		pDynaModel->SetVariableNordsiek(Output_, (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}
