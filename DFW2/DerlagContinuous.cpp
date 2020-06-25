#include "stdafx.h"
#include "DerlagContinuous.h"
#include "DynaModel.h"
using namespace DFW2;

bool CDerlagContinuous::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double T = m_T;

	if (pDynaModel->EstimateBuild())
	{
		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_OutputEquationIndex));
		pRightVector->PrimitiveBlock = PBT_DERLAG;
	}

	if (m_pDevice->IsStateOn())
	{
		//if (!pDynaModel->IsInDiscontinuityMode())
		{
			// dOutput/ dOutput
			pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);

			if (pDynaModel->IsInDiscontinuityMode())
			{
				// dOutput / dY2
				pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex + 1), 0.0);
				// dOutput / dInput
				pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), 0.0);
			}
			else
			{
				// dOutput / dY2
				pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex + 1), m_T * m_K);
				// dOutput / dInput
				pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), -m_K * m_T);
			}
		}
		//else
		//{
		//	// dOutput / dOutput
		//	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
		//	// dOutput / dY2
		//	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex + 1), 0.0);
		//	// dOutput / dInput
		//	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), 0.0);
		//}
	}
	else
	{
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex + 1), 0.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), 0.0);
		T = 0.0;
	}

	// dY2 / dY2
	//pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), 1.0 + hb0 * m_T);
	pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), -T);
	// dY2 / dInput
	//pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_Input->Index()), -hb0 * m_T);
	pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_Input->Index()), -T);

	return true;
}

bool CDerlagContinuous::BuildRightHand(CDynaModel *pDynaModel)
{

	if (m_pDevice->IsStateOn())
	{
		double Input = m_Input->Value();
		double dY2 = (Input - *m_Y2) * m_T;
		double dOut = *m_Output + m_K * m_T * (*m_Y2 - Input);

		

		if (pDynaModel->IsInDiscontinuityMode())
		{
			if (m_pDevice->GetId() == 1319)
				m_pDevice->GetId();

			ProcessDiscontinuity(pDynaModel);
			dOut = 0.0;
		}

		pDynaModel->SetFunction(A(m_OutputEquationIndex), dOut);
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex + 1), dY2);
	}
	else
	{
		pDynaModel->SetFunction(A(m_OutputEquationIndex), 0.0);
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex + 1), 0.0);
	}

	return true;
}

bool CDerlagContinuous::Init(CDynaModel *pDynaModel)
{
	if (m_pDevice->GetId() == 102401 && m_OutputEquationIndex == 9)
		m_pDevice->GetId();

	bool bRes = true;

	if (Equal(m_K, 0.0))
		m_T = 0.0;
	else
	{
		_ASSERTE(!Equal(m_T, 0.0));
		m_T = 1.0 / m_T;
	}

	double Input = m_Input->Value();
	*m_Y2 = Input;
	*m_Output = 0.0;
	return bRes;
}


bool CDerlagContinuous::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_pDevice->GetId() == 108701 && m_OutputEquationIndex == 9)
		m_pDevice->GetId();

	if (m_pDevice->IsStateOn())
	{
		double Input = m_Input->Value();
		double dY2 = (Input - *m_Y2) * m_T;
		pDynaModel->SetDerivative(A(m_OutputEquationIndex + 1), dY2);
	}
	else
	{
		pDynaModel->SetDerivative(A(m_OutputEquationIndex + 1), 0.0);
	}

	return true;
}

eDEVICEFUNCTIONSTATUS CDerlagContinuous::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double Input = m_Input->Value();
		if (Equal(m_K, 0.0))
			*m_Y2 = 0.0;
		else
		{
			// Пока не ясно, надо делать на РДЗ скачок на выходе,
			// или пытаться подогнать лаг ко входу
			if (m_pDevice->GetId() == 1319)
				m_pDevice->GetId();
			*m_Y2 = Input - *m_Output / m_T / m_K;			// подгонка лага ко входу
			//*m_Output = m_K * m_T * (Input - *m_Y2);		// выход по входу
		}
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CDerlagContinuous::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double T1(1E-4), T2(1.0);
	CDynaPrimitive::UnserializeParameters({T1,T2}, Parameters);
	SetTK(T1, T2);
	return true;
}

// ----------------------------- Derlag Nordsieck --------------------------------------
bool CDerlagNordsieck::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (Equal(m_K, 0.0))
		m_T = 0.0;
	else
	{
		_ASSERTE(!Equal(m_T, 0.0));
		m_T = 1.0 / m_T;
	}
	*m_Output = 0.0;
	return bRes;
}

bool CDerlagNordsieck::BuildRightHand(CDynaModel *pDynaModel)
{

	if (m_pDevice->IsStateOn())
	{
		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
		double hOld = pDynaModel->GetOldH();

		if (pDynaModel->IsInDiscontinuityMode())
			pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex), 0.0);
		else
			pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex), (pRightVector->SavedNordsiek[1] / hOld * m_K - *m_Output) * m_T);

		pDynaModel->SetFunction(A(m_OutputEquationIndex + 1), 0.0);
		pDynaModel->SetFunction(A(m_OutputEquationIndex + 2), 0.0);
	}
	else
	{
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex), 0.0);
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex + 1), 0.0);
		pDynaModel->SetFunction(A(m_OutputEquationIndex + 2), 0.0);
	}

	return true;
}

bool CDerlagNordsieck::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
		double hOld = pDynaModel->GetOldH();
		pDynaModel->SetDerivative(A(m_OutputEquationIndex), (pRightVector->SavedNordsiek[1] / hOld * m_K - *m_Output) * m_T);
	}
	else
	{
		pDynaModel->SetDerivative(A(m_OutputEquationIndex), 0.0);
	}

	return true;
}

bool CDerlagNordsieck::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double T1(1E-4), T2(1.0);
	CDynaPrimitive::UnserializeParameters({ T1,T2 }, Parameters);
	SetTK(T1, T2);
	return true;
}

bool CDerlagNordsieck::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (m_pDevice->IsStateOn())
	{

		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
		double hOld = pDynaModel->GetOldH();


		
		if (!pDynaModel->IsInDiscontinuityMode())
		{
			// dOut / dOut
			pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), -m_T);
		}
		else
		{
			// dOut / dOut
			pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);

		}
		pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), 1.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex + 2), A(m_OutputEquationIndex + 2), 1.0);
	}
	else
	{
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), 1.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex + 2), A(m_OutputEquationIndex + 2), 1.0);
	}

	return true;
}


