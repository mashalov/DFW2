#include "stdafx.h"
#include "Abs.h"
#include "DynaModel.h"

using namespace DFW2;

CAbs::CAbs(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : 
									CDynaPrimitive(pDevice,pOutput,nOutputIndex,Input) 
									{}

bool CAbs::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), m_bPositive ? 1.0 : -1.0);
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);

	return bRes && pDynaModel->Status();
}

bool CAbs::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dInput = m_Input->Value();
		pDynaModel->SetFunction(A(m_OutputEquationIndex), *m_Output - (m_bPositive ? dInput : -dInput));
	}
	else
		pDynaModel->SetFunction(A(m_OutputEquationIndex), 0.0);

	return pDynaModel->Status();
}



bool CAbs::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitive::Init(pDynaModel);
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

eDEVICEFUNCTIONSTATUS CAbs::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dInput = m_Input->Value();

		if (dInput >= 0)
			m_bPositive = true;
		else
			m_bPositive = false;

		*m_Output = fabs(dInput);
	}
	else
		*m_Output = 0.0;

	return DFS_OK;
}

double CAbs::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	if (m_pDevice->IsStateOn())
	{
		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
		double dInput = m_Input->Value();

		bool bReadyToChangeState = false;

		if (m_bPositive)
		{
			if (dInput < 0)
				bReadyToChangeState = true;
		}
		else
		{
			if (dInput > 0)
				bReadyToChangeState = true;
		}

		if (bReadyToChangeState)
		{
			double derr = fabs(pRightVector->GetWeightedError(dInput, dInput));
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				m_bPositive = !m_bPositive;
				pDynaModel->DiscontinuityRequest();
			}
			else
			{
				rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector, 0.0);
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					m_bPositive = !m_bPositive;
					pDynaModel->DiscontinuityRequest();
				}
			}
		}
	}
	
	return rH;
}