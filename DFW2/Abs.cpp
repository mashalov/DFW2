#include "stdafx.h"
#include "Abs.h"
#include "DynaModel.h"

using namespace DFW2;

CAbs::CAbs(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : 
									CDynaPrimitiveState(pDevice,pOutput,nOutputIndex,Input) 
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

// проверяем разрыв модуля

double CAbs::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	// если устройство с этим примитивом включено
	if (m_pDevice->IsStateOn())
	{
		// получаем Нордиск входной переменной
		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
		double dInput = m_Input->Value();

		bool bReadyToChangeState = false;

		if (m_bPositive)
		{
			// если было положительное значение переменной
			// и стало отрицательное - отмечаем готовность к изменению состояния
			if (dInput < 0)
				bReadyToChangeState = true;
		}
		else
		{
			// если было отрицательное значение переменной
			// и стало положительное- отмечаем готовность к изменению состояния
			if (dInput > 0)
				bReadyToChangeState = true;
		}

		if (bReadyToChangeState)
		{
			// если есть готовность к изменению состояния (знак входной переменной изменился)
			// рассчитываем погрешность входной переменной
			// оцениванием расстояние от нуля и взвешиваем по самой же переменной,
			// поэтому параметры на входе в GetWeightedError одинаковые
			double derr = fabs(pRightVector->GetWeightedError(dInput, dInput));
			// если погрешность меньше чем заданный порог для зерокроссинга
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				// изменяем состояние и запрашиваем обработку разрыва
				m_bPositive = !m_bPositive;
				pDynaModel->DiscontinuityRequest();
			}
			else
			{
				// если погрешность выше, чем заданная для зерокросиинга
				// ищем долю шага для достижения нуля
				rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector, 0.0);
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					// здесь и выше: состояние изменяем только если нашли время зерокроссинга 
					// с заданной погрешностью
					m_bPositive = !m_bPositive;
					pDynaModel->DiscontinuityRequest();
				}
			}
		}
	}
	
	return rH;
}