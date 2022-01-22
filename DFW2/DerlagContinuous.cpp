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
		RightVector *pRightVector = pDynaModel->GetRightVector(m_Output);
		pRightVector->PrimitiveBlock = PBT_DERLAG;
	}

	if (m_Device.IsStateOn())
	{
		//if (!pDynaModel->IsInDiscontinuityMode())
		{
			// dOutput/ dOutput
			pDynaModel->SetElement(m_Output, m_Output, 1.0);

			if (pDynaModel->IsInDiscontinuityMode())
			{
				// dOutput / dY2
				pDynaModel->SetElement(m_Output, m_Y2, 0.0);
				// dOutput / dInput
				pDynaModel->SetElement(m_Output, m_Input, 0.0);
			}
			else
			{
				// dOutput / dY2
				pDynaModel->SetElement(m_Output, m_Y2, m_T * m_K);
				// dOutput / dInput
				pDynaModel->SetElement(m_Output, m_Input, -m_K * m_T);
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
		pDynaModel->SetElement(m_Output, m_Output, 1.0);
		pDynaModel->SetElement(m_Output, m_Y2, 0.0);
		pDynaModel->SetElement(m_Output, m_Input, 0.0);
		T = 0.0;
	}

	// dY2 / dY2
	//pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), 1.0 + hb0 * m_T);
	pDynaModel->SetElement(m_Y2, m_Y2, -T);
	// dY2 / dInput
	//pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_Input->Index()), -hb0 * m_T);
	pDynaModel->SetElement(m_Y2, m_Input, -T);

	return true;
}

bool CDerlagContinuous::BuildRightHand(CDynaModel *pDynaModel)
{

	if (m_Device.IsStateOn())
	{
		double dY2 = (m_Input - m_Y2) * m_T;
		//double dOut = m_Output + m_K * m_T * (m_Y2 - m_Input);
		double dOut = m_Output - m_K * dY2;

		if (pDynaModel->IsInDiscontinuityMode())
		{
			ProcessDiscontinuity(pDynaModel);
			dOut = 0.0;
		}

		pDynaModel->SetFunction(m_Output, dOut);
		pDynaModel->SetFunctionDiff(m_Y2, dY2);
	}
	else
	{
		pDynaModel->SetFunction(m_Output, 0.0);
		pDynaModel->SetFunctionDiff(m_Y2, 0.0);
	}

	return true;
}

bool CDerlagContinuous::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (Equal(m_K, 0.0))
		m_T = 0.0;
	else
	{
		_ASSERTE(!Equal(m_T, 0.0));
		m_T = 1.0 / m_T;
	}
	m_Y2 = m_Input;
	m_Output = 0.0;
	return bRes;
}


bool CDerlagContinuous::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dY2 = (m_Input - m_Y2) * m_T;
		pDynaModel->SetDerivative(m_Y2, dY2);
	}
	else
	{
		pDynaModel->SetDerivative(m_Y2, 0.0);
	}

	return true;
}

eDEVICEFUNCTIONSTATUS CDerlagContinuous::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		if (Equal(m_K, 0.0))
			m_Y2 = 0.0;
		else
		{
			// Пока не ясно, надо делать на РДЗ скачок на выходе,
			// или пытаться подогнать лаг ко входу
			m_Y2 = m_Input - m_Output / m_T / m_K;			// подгонка лага ко входу
			//m_Output = m_K * m_T * (m_Input - m_Y2);		// выход по входу
		}
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

// Вариант функции для РДЗ с подавлением разрыва на выходе
#if defined DERLAG_CONTINUOUSMOOTH || defined DERLAG_CONTINUOUS
eDEVICEFUNCTIONSTATUS CDerlagContinuousSmooth::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		if (Equal(m_K, 0.0))
			m_Y2 = 0.0;
		else
		{
			// рассчитываем выход лага с условием сохранения значения на выходе РДЗ
			m_Y2 = m_Input  - m_Output / m_K / m_T;
			double dOut = m_Output + m_K * m_T * (m_Y2 - m_Input);
		}
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}
#endif


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
	if (Equal(m_K, 0.0))
		m_T = 0.0;
	else
	{
		_ASSERTE(!Equal(m_T, 0.0));
		m_T = 1.0 / m_T;
	}
	m_Output = 0.0;
	return true;
}

bool CDerlagNordsieck::BuildRightHand(CDynaModel *pDynaModel)
{

	if (m_Device.IsStateOn())
	{
		const RightVector* pRightVector{ pDynaModel->GetRightVector(m_Input.Index) };

		if (pDynaModel->IsInDiscontinuityMode())
			pDynaModel->SetFunctionDiff(m_Output, 0.0);
		else
		{
			//if(pDynaModel->IsNordsiekReset())
				pDynaModel->SetFunctionDiff(m_Output, ((m_Input - pRightVector->Nordsiek[0]) / pDynaModel->GetH() * m_K - m_Output) * m_T);
			//else
			//	pDynaModel->SetFunctionDiff(m_Output, (pRightVector->Nordsiek[1] / pDynaModel->GetH() * m_K - m_Output) * m_T);
				
		}

		pDynaModel->SetFunction(m_Y2, 0.0);
	}
	else
	{
		pDynaModel->SetFunctionDiff(m_Output, 0.0);
		pDynaModel->SetFunction(m_Y2, 0.0);
	}

	return true;
}

bool CDerlagNordsieck::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		const RightVector* pRightVector{ pDynaModel->GetRightVector(m_Input.Index) };
		pDynaModel->SetDerivative(m_Output, (pRightVector->Nordsiek[1] / pDynaModel->GetH() * m_K - m_Output) * m_T);
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


#if defined DERLAG_NORDSIEK
eDEVICEFUNCTIONSTATUS CDerlagNordsieck::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		if (Equal(m_K, 0.0))
			m_Y2 = 0.0;
		else
		{
			const RightVector* pRightVector{ pDynaModel->GetRightVector(m_Input.Index) };
			//m_Output = pRightVector->Nordsiek[1] / pDynaModel->GetH() * m_K;
			//// рассчитываем выход лага с условием сохранения значения на выходе РДЗ
			//m_Y2 = (m_Input * m_K * m_T - m_Output) / m_K / m_T;
			//double dOut = m_Output + m_K * m_T * (m_Y2 - m_Input);
		}
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}
#endif

bool CDerlagNordsieck::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (m_Device.IsStateOn())
	{

		const RightVector* pRightVector{ pDynaModel->GetRightVector(m_Input) };

		double dIdO{ 0.0 };

		if (!pDynaModel->IsInDiscontinuityMode())
		{
			// dOut / dOut
			pDynaModel->SetElement(m_Output, m_Output, -m_T);
			//if (pDynaModel->IsNordsiekReset())
				dIdO = -m_Input / pDynaModel->GetH() * m_K * m_T;
		}
		else
		{
			// dOut / dOut
			pDynaModel->SetElement(m_Output, m_Output, 1.0);
		}

		pDynaModel->SetElement(m_Output, m_Input, dIdO);
		pDynaModel->SetElement(m_Y2, m_Y2, 1.0);
	}
	else
	{
		pDynaModel->SetElement(m_Output, m_Output, 1.0);
		pDynaModel->SetElement(m_Y2, m_Y2, 1.0);
	}

	return true;
}

