﻿#include "stdafx.h"
#include "DynaPrimitive.h"
#include "DynaModel.h"

using namespace DFW2;

ptrdiff_t CDynaPrimitive::A(ptrdiff_t nOffset)
{ 
	return m_pDevice->A(nOffset); 
}

bool CDynaPrimitive::Init(CDynaModel *pDynaModel)
{
	bool bRes = false;
	if (m_pDevice/* && m_Input->Index() >= 0 && m_OutputEquationIndex >= 0*/)
		bRes = true;
	return bRes;
}

CDynaPrimitiveState::CDynaPrimitiveState(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitive(pDevice, pOutput, nOutputIndex, Input)
{
	pDevice->RegisterStatePrimitive(this);
}

CDynaPrimitiveState::CDynaPrimitiveState(CDevice *pDevice, VariableIndex* pInput) : CDynaPrimitive(pDevice, pInput)
{
	pDevice->RegisterStatePrimitive(this);
}


bool CDynaPrimitiveLimited::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitive::Init(pDynaModel);
	if (bRes)
	{
		eCurrentState = LS_MID;
		
		if (m_dMin > m_dMax)
		{
			m_pDevice->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongPrimitiveLimits, GetVerbalName(), m_pDevice->GetVerbalName(), m_dMin, m_dMax));
			bRes = false;
		}

		SetMinMax(pDynaModel, m_dMin, m_dMax);

		if (m_Output)
		{
			if (*m_Output > m_dMaxH || *m_Output < m_dMinH)
			{
				m_pDevice->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongPrimitiveInitialConditions, GetVerbalName(), m_pDevice->GetVerbalName(), *m_Output, m_dMin, m_dMax));
				bRes = false;
			}
		}
		else
		{
			if (m_viOutput > m_dMaxH || m_viOutput < m_dMinH)
			{
				m_pDevice->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongPrimitiveInitialConditions, GetVerbalName(), m_pDevice->GetVerbalName(), (double)m_viOutput, m_dMin, m_dMax));
				bRes = false;
			}
		}
	}
	return bRes;
}

double CDynaPrimitive::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	return 1.0;
}

bool CDynaPrimitive::ChangeState(CDynaModel *pDynaModel, double Diff, double TolCheck, double Constraint, ptrdiff_t ValueIndex, double &rH)
{
	// Diff			- контроль знака - если < 0 - переходим в LS_MID
	// TolCheck		- значение для контроля относительной погрешности
	// Constraint	- константа, относительно которой определяем пересечение

	bool bChangeState = false;

	RightVector *pRightVector = pDynaModel->GetRightVector(A(ValueIndex));
	rH = FindZeroCrossingToConst(pDynaModel, pRightVector, Constraint);

	if (pDynaModel->GetZeroCrossingInRange(rH))
	{
		if (Diff < 0.0)
		{
			double derr = fabs(pRightVector->GetWeightedError(Diff, TolCheck));
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				bChangeState = true;
				rH = 1.0;
			}
			else
			{
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					bChangeState = true;
					rH = 1.0;
				}
			}
		}
	}
	else if (Diff < 0.0)
	{
		bChangeState = true;
		_ASSERTE(0); // корня нет, но знак изменился !
		rH = 1.0;
	}
	else
		rH = 1.0;

	return bChangeState;
}

// определение доли шага зерокроссинга для примитива с минимальным и максимальным ограничениями

double CDynaPrimitiveLimited::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	eLIMITEDSTATES oldCurrentState = eCurrentState;
	switch (eCurrentState)
	{
	case LS_MID:
		rH = OnStateMid(pDynaModel);
		break;
	case LS_MAX:
		rH = OnStateMax(pDynaModel);
		break;
	case LS_MIN:
		rH = OnStateMin(pDynaModel);
		break;
	}

	// если состояние изменилось, запрашиваем обработку разрыва
	if (oldCurrentState != eCurrentState)
	{
		pDynaModel->Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("t=%.12g (%d) Примитив %s из %s изменяет состояние %g %g %g с %d на %d"), 
			pDynaModel->GetCurrentTime(), 
			pDynaModel->GetIntegrationStepNumber(),
			GetVerbalName(), 
			m_pDevice->GetVerbalName(),
			*m_Output, 
			m_dMin, m_dMax, 
			oldCurrentState, eCurrentState);
		pDynaModel->DiscontinuityRequest();
	}

	return rH;
}

double CDynaPrimitive::FindZeroCrossingToConst(CDynaModel *pDynaModel, RightVector* pRightVector, double dConst)
{
	ptrdiff_t q = pDynaModel->GetOrder();
	double h = pDynaModel->GetH();

	double dError = pRightVector->Error;

	// получаем константу метода интегрирования
	const double *lm = pDynaModel->Methodl[pRightVector->EquationType * 2 + q - 1];
	// рассчитываем коэффициенты полинома, описывающего изменение переменной
	double a = 0.0;		// если порядок метода 1 - квадратичный член равен нулю
	// линейный член
	double b = (pRightVector->Nordsiek[1] + dError * lm[1]) / h;
	// постоянный член
	double c = (pRightVector->Nordsiek[0] + dError * lm[0]) - dConst;
	// если порядок метода 2 - то вводим квадратичный коэффициент
	if (q == 2)
		a = (pRightVector->Nordsiek[2] + dError * lm[2]) / h / h;
	// возвращаем отношение зеро-кроссинга для полинома
	return GetZCStepRatio(pDynaModel, a, b, c);
}

void CDynaPrimitiveLimited::SetCurrentState(CDynaModel* pDynaModel, eLIMITEDSTATES CurrentState)
{
	if (eCurrentState != CurrentState)
	{
		pDynaModel->DiscontinuityRequest();
	}
	
	eCurrentState = CurrentState;
}

void CDynaPrimitiveLimited::SetMinMax(CDynaModel *pDynaModel, double dMin, double dMax)
{
	m_dMin = dMin;
	m_dMax = dMax;
	m_dMinH = m_dMin - pDynaModel->GetHysteresis(m_dMin);
	m_dMaxH = m_dMax + pDynaModel->GetHysteresis(m_dMax);
}

void CDynaPrimitiveBinary::InvertState(CDynaModel *pDynaModel)
{
	SetCurrentState(pDynaModel, GetCurrentState() == RS_ON ? RS_OFF : RS_ON);
}

void CDynaPrimitiveBinary::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	if (eCurrentState != CurrentState)
	{
		pDynaModel->DiscontinuityRequest();
	}
	eCurrentState = CurrentState;
}

double CDynaPrimitiveBinaryOutput::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	if (m_pDevice->IsStateOn())
	{
		eRELAYSTATES oldCurrentState = eCurrentState;

		switch (eCurrentState)
		{
		case RS_ON:
			rH = OnStateOn(pDynaModel);
			break;
		case RS_OFF:
			rH = OnStateOff(pDynaModel);
			break;
		}

		if (oldCurrentState != eCurrentState)
			RequestZCDiscontinuity(pDynaModel);
	}
	return rH;
}

void CDynaPrimitiveBinary::RequestZCDiscontinuity(CDynaModel* pDynaModel)
{
	pDynaModel->DiscontinuityRequest();
}

bool CDynaPrimitiveBinary::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
	return true;
}

bool CDynaPrimitiveBinary::BuildRightHand(CDynaModel *pDynaModel)
{
	pDynaModel->SetFunction(A(m_OutputEquationIndex), 0.0);
	return true;
}

double CDynaPrimitiveBinaryOutput::FindZeroCrossingOfDifference(CDynaModel *pDynaModel, RightVector* pRightVector1, RightVector* pRightVector2)
{
	ptrdiff_t q = pDynaModel->GetOrder();
	double h = pDynaModel->GetH();
	double dError1 = pRightVector1->Error;
	double dError2 = pRightVector2->Error;

	const double *lm1 = pDynaModel->Methodl[pRightVector1->EquationType * 2 + q - 1];
	const double *lm2 = pDynaModel->Methodl[pRightVector2->EquationType * 2 + q - 1];

	double a = 0.0;
	double b = (pRightVector1->Nordsiek[1] + dError1 * lm1[1] - (pRightVector2->Nordsiek[1] + dError2 * lm2[1])) / h;
	double c = (pRightVector1->Nordsiek[0] + dError1 * lm1[0] - (pRightVector2->Nordsiek[0] + dError2 * lm2[0]));
	if (q == 2)
		a = (pRightVector1->Nordsiek[2] + dError1 * lm1[2] - (pRightVector2->Nordsiek[2] + dError2 * lm2[2])) / h / h;

	return GetZCStepRatio(pDynaModel, a, b, c);
}

// возвращает отношение текущего шага к шагу до пересечения заданного a*t*t + b*t + c полинома
double CDynaPrimitive::GetZCStepRatio(CDynaModel *pDynaModel, double a, double b, double c)
{
	// по умолчанию зеро-кроссинга нет - отношение 1.0
	double rH = 1.0;
	double h = pDynaModel->GetH();

	if (Equal(a, 0.0))
	{
		// если квадратичный член равен нулю - просто решаем линейное уравнение
		//if (!Equal(b, 0.0))
		{
			double h1 = -c / b;
			rH = (h + h1) / h;
			//_ASSERTE(rH >= 0);
		}
	}
	else
	{
		// если квадратичный член ненулевой - решаем квадратичное уравнение
		double d = b * b - 4.0 * a * c;

		if (d >= 0)
		{
			d = sqrt(d);

			double h1 = (-b + d) / 2.0 / a;
			double h2 = (-b - d) / 2.0 / a;

			// use stable formulas to avoid
			// precision loss by numerical cancellation 
			// "What Every Computer Scientist Should Know About Floating-Point Arithmetic" by DAVID GOLDBERG p.10

			if ((b*b - a*c) > 1E3 && b > 0)
				h1 = 2.0 * c / (-b - d);
			else
				h2 = 2.0 * c / (-b + d);

			_ASSERTE(!(Equal(h1, FLT_MAX) && Equal(h2, FLT_MAX)));

			if (h1 > 0.0 || h1 < -h) h1 = FLT_MAX;
			if (h2 > 0.0 || h2 < -h) h2 = FLT_MAX;

			// возвращаем наименьший из действительных корней
			rH = (h + min(h1, h2)) / h;

			//_ASSERTE(rH >= 0);
		}
	}

	return rH;
}
