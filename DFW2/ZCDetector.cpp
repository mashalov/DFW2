#include "stdafx.h"
#include "ZCDetector.h"
#include "DynaModel.h"

using namespace DFW2;

double CZCDetector::OnStateOff(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));
	double dValue = m_Input->Value();
	double dHyst = pDynaModel->GetHysteresis(dValue);

	if (dValue >= dHyst)
	{
		double derr = fabs(pRightVector1->GetWeightedError(dValue, dValue));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(pDynaModel, RS_ON);
		}
		else
		{
			rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector1, dHyst);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(pDynaModel, RS_ON);
		}
	}

	return rH;

}

double CZCDetector::OnStateOn(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));
	double dValue = m_Input->Value();
	double dHyst = pDynaModel->GetHysteresis(dValue);

	if (dValue <= 0)
	{
		double derr = fabs(pRightVector1->GetWeightedError(dValue, dValue));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(pDynaModel, RS_OFF);
		}
		else
		{
			rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector1, 0.0);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(pDynaModel, RS_OFF);
		}
	}

	return rH;
}

bool CZCDetector::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	eCurrentState = RS_OFF;
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));

	return bRes;
}

eDEVICEFUNCTIONSTATUS CZCDetector::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		if (m_Input->Value() > 0)
			SetCurrentState(pDynaModel, RS_ON);
		else
			SetCurrentState(pDynaModel, RS_OFF);

		*m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
	}
	return DFS_OK;
}
