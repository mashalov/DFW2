#include "stdafx.h"
#include "Abs.h"
#include "DynaModel.h"

using namespace DFW2;



bool CAbs::BuildEquations(CDynaModel *pDynaModel)
{
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), m_bPositive ? 1.0 : -1.0);
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
	return true;
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

	return true;
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

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

// проверяем разрыв модуля

double CAbs::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (!m_pDevice->IsStateOn())
		return rH;

	double dInput = m_Input->Value();
	double dHyst = pDynaModel->GetHysteresis(dInput);
	if (m_bPositive)
	{
		if (CDynaPrimitive::ChangeState(pDynaModel, dInput + dHyst, dInput + dHyst, -dHyst, m_Input->Index(), rH))
		{
			m_bPositive = false;
			pDynaModel->DiscontinuityRequest();
		}
	}
	else
	{
		if (CDynaPrimitive::ChangeState(pDynaModel, dHyst - dInput, dHyst - dInput, dHyst, m_Input->Index(), rH))
		{
			m_bPositive = true;
			pDynaModel->DiscontinuityRequest();
		}
	}

	return rH;
}