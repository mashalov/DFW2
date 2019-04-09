#include "stdafx.h"
#include "Comparator.h"
#include "DynaModel.h"

using namespace DFW2;


CComparator::CComparator(CDevice *pDevice, 
						 double* pOutput, 
						 ptrdiff_t nOutputIndex, 
						 PrimitiveVariableBase* Input1, 
						 PrimitiveVariableBase* Input2) : CDynaPrimitiveBinaryOutput(pDevice, pOutput, nOutputIndex, Input1),
														  m_Input2(Input2)
																												
{

}


bool CComparator::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;

	eCurrentState = RS_OFF;
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));

	return bRes;
}

eDEVICEFUNCTIONSTATUS CComparator::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		SetCurrentState(pDynaModel, (m_Input->Value() > m_Input2->Value()) ? RS_ON : RS_OFF);
		*m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
	}
	else
		*m_Output = 0.0;

	return DFS_OK;
}

double CComparator::OnStateOn(CDynaModel *pDynaModel)
{
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));
	RightVector *pRightVector2 = pDynaModel->GetRightVector(A(m_Input2->Index()));
	double dValue2 = m_Input2->Value();
	double dCheck = m_Input->Value() - dValue2;
	double rH = 1.0;

	if (dCheck < 0)
	{
		double derr = fabs(pRightVector1->GetWeightedError(dCheck, dValue2));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(pDynaModel, RS_OFF);
		}
		else
		{
			rH = CDynaPrimitiveBinaryOutput::FindZeroCrossingOfDifference(pDynaModel, pRightVector1, pRightVector2);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(pDynaModel, RS_OFF);
		}
	}
	return rH;
}

double CComparator::OnStateOff(CDynaModel *pDynaModel)
{
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));
	RightVector *pRightVector2 = pDynaModel->GetRightVector(A(m_Input2->Index()));
	double dValue1 = m_Input->Value();
	double dCheck = m_Input2->Value() - dValue1;
	double rH = 1.0;

	if (dCheck < 0)
	{
		double derr = fabs(pRightVector1->GetWeightedError(dCheck, dValue1));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(pDynaModel, RS_ON);
		}
		else
		{
			rH = CDynaPrimitiveBinaryOutput::FindZeroCrossingOfDifference(pDynaModel, pRightVector1, pRightVector2);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(pDynaModel, RS_ON);
		}
	}
	return rH;
}