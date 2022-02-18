#include "stdafx.h"
#include "DerlagContinuous.h"
#include "DynaModel.h"
using namespace DFW2;

void CDerlagContinuous::BuildEquations(CDynaModel *pDynaModel)
{
	double T{ T_ };

	if (pDynaModel->EstimateBuild())
	{
		RightVector *pRightVector = pDynaModel->GetRightVector(Output_);
		pRightVector->PrimitiveBlock = PBT_DERLAG;
	}

	if (Device_.IsStateOn())
	{
		//if (!pDynaModel->IsInDiscontinuityMode())
		{
			// dOutput/ dOutput
			pDynaModel->SetElement(Output_, Output_, 1.0);

			if (pDynaModel->IsInDiscontinuityMode())
			{
				// dOutput / dY2
				pDynaModel->SetElement(Output_, Y2_, 0.0);
				// dOutput / dInput
				pDynaModel->SetElement(Output_, Input_, 0.0);
			}
			else
			{
				// dOutput / dY2
				pDynaModel->SetElement(Output_, Y2_, T_ * K_);
				// dOutput / dInput
				pDynaModel->SetElement(Output_, Input_, -K_ * T_);
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
		pDynaModel->SetElement(Output_, Output_, 1.0);
		pDynaModel->SetElement(Output_, Y2_, 0.0);
		pDynaModel->SetElement(Output_, Input_, 0.0);
		T = 0.0;
	}

	// dY2 / dY2
	//pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), 1.0 + hb0 * m_T);
	pDynaModel->SetElement(Y2_, Y2_, -T);
	// dY2 / dInput
	//pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_Input->Index()), -hb0 * m_T);
	pDynaModel->SetElement(Y2_, Input_, -T);
}

void CDerlagContinuous::BuildRightHand(CDynaModel *pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double dY2{ (Input_ - Y2_) * T_ };
		//double dOut = m_Output + m_K * m_T * (m_Y2 - m_Input);
		double dOut{ Output_ - K_ * dY2 };

		if (pDynaModel->IsInDiscontinuityMode())
		{
			ProcessDiscontinuity(pDynaModel);
			dOut = 0.0;
		}

		pDynaModel->SetFunction(Output_, dOut);
		pDynaModel->SetFunctionDiff(Y2_, dY2);
	}
	else
	{
		pDynaModel->SetFunction(Output_, 0.0);
		pDynaModel->SetFunctionDiff(Y2_, 0.0);
	}
}

bool CDerlagContinuous::Init(CDynaModel *pDynaModel)
{
	bool bRes{ true };
	if (Equal(K_, 0.0))
		T_ = 0.0;
	else
	{
		_ASSERTE(!Equal(T_, 0.0));
		T_ = 1.0 / T_;
	}
	Y2_ = Input_;
	Output_ = 0.0;
	return bRes;
}


void CDerlagContinuous::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double dY2{ (Input_ - Y2_) * T_ };
		pDynaModel->SetDerivative(Y2_, dY2);
	}
	else
	{
		pDynaModel->SetDerivative(Y2_, 0.0);
	}
}

eDEVICEFUNCTIONSTATUS CDerlagContinuous::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		if (Equal(K_, 0.0))
			Y2_ = 0.0;
		else
		{
			// Пока не ясно, надо делать на РДЗ скачок на выходе,
			// или пытаться подогнать лаг ко входу
			Y2_ = Input_ - Output_ / T_ / K_;			// подгонка лага ко входу
			//m_Output = m_K * m_T * (m_Input - m_Y2);		// выход по входу
		}
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

// Вариант функции для РДЗ с подавлением разрыва на выходе
#if defined DERLAG_CONTINUOUSMOOTH || defined DERLAG_CONTINUOUS
eDEVICEFUNCTIONSTATUS CDerlagContinuousSmooth::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		if (Equal(K_, 0.0))
			Y2_ = 0.0;
		else
		{
			// рассчитываем выход лага с условием сохранения значения на выходе РДЗ
			Y2_ = Input_  - Output_ / K_ / T_;
			double dOut = Output_ + K_ * T_ * (Y2_ - Input_);
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

#if defined DERLAG_NORDSIEK

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

void CDerlagNordsieck::BuildRightHand(CDynaModel *pDynaModel)
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
}

void CDerlagNordsieck::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		const RightVector* pRightVector{ pDynaModel->GetRightVector(m_Input.Index) };
		pDynaModel->SetDerivative(m_Output, (pRightVector->Nordsiek[1] / pDynaModel->GetH() * m_K - m_Output) * m_T);
	}
}

bool CDerlagNordsieck::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double T1(1E-4), T2(1.0);
	CDynaPrimitive::UnserializeParameters({ T1,T2 }, Parameters);
	SetTK(T1, T2);
	return true;
}

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

void CDerlagNordsieck::BuildEquations(CDynaModel *pDynaModel)
{
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
}

#endif
