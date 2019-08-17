#include "stdafx.h"
#include "ZCDetector.h"
#include "DynaModel.h"

using namespace DFW2;


double CZCDetector::ChangeState(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (!m_pDevice->IsStateOn())
		return rH;

	double dInput = m_Input->Value();
	// получаем Нордиск входной переменной
	RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
	double dHyst = pDynaModel->GetHysteresis(dInput);
	bool bReadyToChangeState = false;
	if (GetCurrentState() == RS_ON)
	{
		// если было положительное значение переменной
		// и стало отрицательное - отмечаем готовность к изменению состояния
		dHyst -= dHyst;
		if (dInput <= dHyst)
			bReadyToChangeState = true;
	}
	else
	{
		// если было отрицательное значение переменной
		// и стало положительное- отмечаем готовность к изменению состояния
		if (dInput >= dHyst)
			bReadyToChangeState = true;
	}

	// проверяем, могло ли быть изменение знака входной переменной на шаге интегрирования
	rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector, dHyst);

	if (pDynaModel->GetZeroCrossingInRange(rH))
	{
		if (bReadyToChangeState)
		{
			// если есть готовность к изменению состояния (знак входной переменной изменился)
			// рассчитываем погрешность входной переменной
			// оцениванием расстояние от нуля и взвешиваем по самой же переменной,
			// поэтому параметры на входе в GetWeightedError одинаковые
			double derr = fabs(pRightVector->GetWeightedError(dInput + dHyst, dInput + dHyst));
			// если погрешность меньше чем заданный порог для зерокроссинга
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				// изменяем состояние
				InvertState(pDynaModel);
				rH = 1.0;
			}
			else
			{
				if (pDynaModel->ZeroCrossingStepReached(rH))
					InvertState(pDynaModel);
			}
		}
		// если изменения знака не было - не изменяем состояние, возвращаем шаг до зерокроссинга
	}
	else if (bReadyToChangeState)
	{
		// если пересечения не было найдено, но знак изменился, меняем состояние
		InvertState(pDynaModel);
		rH = 1.0;
		_ASSERTE(0); // корня нет, но знак изменился !
	}
	else
		rH = 1.0;

	return rH;
}

double CZCDetector::OnStateOff(CDynaModel *pDynaModel)
{
	return ChangeState(pDynaModel);
	/*
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
	*/
}

double CZCDetector::OnStateOn(CDynaModel *pDynaModel)
{
	return ChangeState(pDynaModel);
	/*
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
	*/
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
