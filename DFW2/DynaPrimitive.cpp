#include "stdafx.h"
#include "DynaPrimitive.h"
#include "DynaModel.h"
#include "MathUtils.h"

using namespace DFW2;

/*
ptrdiff_t CDynaPrimitive::A(ptrdiff_t nOffset)
{ 
	return m_pDevice->A(nOffset); 
}
*/

bool CDynaPrimitive::Init(CDynaModel *pDynaModel)
{
	return true;
}

bool CDynaPrimitiveLimited::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitive::Init(pDynaModel);
	if (bRes)
	{
		eCurrentState = eLIMITEDSTATES::LS_MID;
		
		if (m_dMin > m_dMax)
		{
			m_Device.Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongPrimitiveLimits,
																   GetVerbalName(), 
																   m_Device.GetVerbalName(), 
																   m_dMin, 
																   m_dMax));
			bRes = false;
		}

		SetMinMax(pDynaModel, m_dMin, m_dMax);

		if (( m_Output > m_dMaxH || m_Output < m_dMinH ) && m_Device.IsStateOn())
		{
			m_Device.Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongPrimitiveInitialConditions,
																   GetVerbalName(), 
																   m_Device.GetVerbalName(), 
																   m_Output, m_dMin, m_dMax));
			bRes = false;
		}
	}
	return bRes;
}

double CDynaPrimitive::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	return 1.0;
}

bool CDynaPrimitive::UnserializeParameters(DOUBLEREFVEC ParametersList, const DOUBLEVECTOR& Parameters)
{
	auto dest = ParametersList.begin();
	for (auto& src : Parameters)
		if (dest != ParametersList.end())
		{
			dest->get() = src;
			dest++;
		}
	return true;
}

bool CDynaPrimitive::UnserializeParameters(PRIMITIVEPARAMETERSDEFAULT ParametersList, const DOUBLEVECTOR& Parameters)
{
	auto src = Parameters.begin();
	for (auto& dest : ParametersList)
		if (src != Parameters.end())
		{
			dest.first.get() = *src;
			src++;
		}
		else
			dest.first.get() = dest.second;
	return true;
}

bool CDynaPrimitive::ChangeState(CDynaModel *pDynaModel, double Diff, double TolCheck, double Constraint, ptrdiff_t ValueIndex, double &rH)
{
	// Diff			- контроль знака - если < 0 - переходим в LS_MID
	// TolCheck		- значение для контроля относительной погрешности
	// Constraint	- константа, относительно которой определяем пересечение

	bool bChangeState = false;
	rH = 1.0;

	if (Diff < 0.0)
	{
		// обращаемся к устройству с запросом
		// на уточнение зеро-кроссинга
		if (m_Device.DetectZeroCrossingFine(this))
		{
			RightVector* pRightVector = pDynaModel->GetRightVector(ValueIndex);
			// определяем расстояние до точки zero-crossing в долях от текущего шага
			rH = FindZeroCrossingToConst(pDynaModel, pRightVector, Constraint);
			if (pDynaModel->GetZeroCrossingInRange(rH))
			{
				// проверяем погрешность зеро-кроссинга по значению со взвешиванием
				// так же как в контроле точности шага
				double derr = std::abs(pRightVector->GetWeightedError(Diff, TolCheck));
				if (derr < pDynaModel->GetZeroCrossingTolerance())
				{
					// если точность удовлетворительная, изменяем состояние
					// примитива и шаг не уменьшаем
					bChangeState = true;
					rH = 1.0;
				}
				else
				{
					// если точность зеро-кроссинга не достигнута,
					// проверяем можно ли еще снизить шаг
					if (pDynaModel->ZeroCrossingStepReached(rH))
					{
						// если нет - меняем состояние
						bChangeState = true;
						rH = 1.0;
					}
				}
			}
			else
			{
				bChangeState = true;
				_ASSERTE(0); // корня нет, но знак изменился !
				rH = 1.0;
			}
		}
		else
		{
			// если устройство не разрешило уточнение 
			// времени зеро-кроссинга - меняем состояние
			bChangeState = true;
		}
	}

	return bChangeState;
}

// определение доли шага зерокроссинга для примитива с минимальным и максимальным ограничениями

double CDynaPrimitiveLimited::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	eLIMITEDSTATES oldCurrentState = eCurrentState;
	switch (eCurrentState)
	{
	case eLIMITEDSTATES::LS_MID:
		rH = OnStateMid(pDynaModel);
		break;
	case eLIMITEDSTATES::LS_MAX:
		rH = OnStateMax(pDynaModel);
		break;
	case eLIMITEDSTATES::LS_MIN:
		rH = OnStateMin(pDynaModel);
		break;
	}

	// если состояние изменилось, запрашиваем обработку разрыва
	if (oldCurrentState != eCurrentState)
	{
		pDynaModel->Log(DFW2MessageStatus::DFW2LOG_DEBUG, 
			fmt::format("t={:15.012f} {:>3} Примитив {} из {} изменяет состояние {} {} {} с {} на {}", 
			pDynaModel->GetCurrentTime(), 
			pDynaModel->GetIntegrationStepNumber(),
			GetVerbalName(), 
			m_Device.GetVerbalName(),
			/*static_cast<const double>*/m_Output, 
			m_dMin, m_dMax, 
			oldCurrentState, eCurrentState));
		pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
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
	const double rH(GetZCStepRatio(pDynaModel, a, b, c));

	if (rH <= 0.0)
	{
		if (pRightVector->pDevice)
			pRightVector->pDevice->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(
				"Negative ZC ratio {} in device {}, variable {} at t={}. "
				"Nordsieck [{},{},{}], Constant = {}",
				rH,
				pRightVector->pDevice->GetVerbalName(),
				pRightVector->pDevice->VariableNameByPtr(pRightVector->pValue),
				pDynaModel->GetCurrentTime(),
				pRightVector->Nordsiek[0],
				pRightVector->Nordsiek[1],
				pRightVector->Nordsiek[2],
				dConst
			));
	}
	return rH;
}

void CDynaPrimitiveLimited::SetCurrentState(CDynaModel* pDynaModel, eLIMITEDSTATES CurrentState)
{
	if (eCurrentState != CurrentState)
	{
		pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
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
	SetCurrentState(pDynaModel, GetCurrentState() == eRELAYSTATES::RS_ON ? eRELAYSTATES::RS_OFF : eRELAYSTATES::RS_ON);
}

// изменить состояние примитива
void CDynaPrimitiveBinary::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	if (eCurrentState != CurrentState)
	{
		// если текущее состояние не соответствует заданному
		// запрашиваем обработку разрыва
		pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
	}
	eCurrentState = CurrentState;
}

double CDynaPrimitiveBinaryOutput::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	if (m_Device.IsStateOn())
	{
		eRELAYSTATES oldCurrentState = eCurrentState;

		switch (eCurrentState)
		{
		case eRELAYSTATES::RS_ON:
			rH = OnStateOn(pDynaModel);
			break;
		case eRELAYSTATES::RS_OFF:
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
	pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
}

bool CDynaPrimitiveBinary::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	pDynaModel->SetElement(m_Output, m_Output, 1.0);
	return true;
}

bool CDynaPrimitiveBinary::BuildRightHand(CDynaModel *pDynaModel)
{
	pDynaModel->SetFunction(m_Output, 0.0);
	return true;
}

double CDynaPrimitiveBinaryOutput::FindZeroCrossingOfDifference(CDynaModel* pDynaModel, RightVector* pRightVector1, RightVector* pRightVector2)
{
	ptrdiff_t q = pDynaModel->GetOrder();
	double h = pDynaModel->GetH();
	double dError1 = pRightVector1->Error;
	double dError2 = pRightVector2->Error;

	const double* lm1 = pDynaModel->Methodl[pRightVector1->EquationType * 2 + q - 1];
	const double* lm2 = pDynaModel->Methodl[pRightVector2->EquationType * 2 + q - 1];

	double a = 0.0;
	double b = (pRightVector1->Nordsiek[1] + dError1 * lm1[1] - (pRightVector2->Nordsiek[1] + dError2 * lm2[1])) / h;
	double c = (pRightVector1->Nordsiek[0] + dError1 * lm1[0] - (pRightVector2->Nordsiek[0] + dError2 * lm2[0]));
	if (q == 2)
		a = (pRightVector1->Nordsiek[2] + dError1 * lm1[2] - (pRightVector2->Nordsiek[2] + dError2 * lm2[2])) / h / h;

	const double rH(GetZCStepRatio(pDynaModel, a, b, c));

	if (rH <= 0.0)
	{
		if (pRightVector1->pDevice)
			pRightVector1->pDevice->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(
				"Negative ZC ratio {} in device {}, variable {} at t={}. "
				"Nordsieck1 [{},{},{}], Nordsieck2 [{},{},{}]",
				rH,
				pRightVector1->pDevice->GetVerbalName(),
				pRightVector1->pDevice->VariableNameByPtr(pRightVector1->pValue),
				pDynaModel->GetCurrentTime(),
				pRightVector1->Nordsiek[0],
				pRightVector1->Nordsiek[1],
				pRightVector1->Nordsiek[2],
				pRightVector2->Nordsiek[0],
				pRightVector2->Nordsiek[1],
				pRightVector2->Nordsiek[2]
			));
	}
	return rH;
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
		double h1(0.0), h2(0.0);

		if (MathUtils::CSquareSolver::Roots(a, b, c, h1, h2))
		{
			_ASSERTE(!(Equal(h1, FLT_MAX) && Equal(h2, FLT_MAX)));

			if (h1 > 0.0 || h1 < -h) h1 = FLT_MAX;
			if (h2 > 0.0 || h2 < -h) h2 = FLT_MAX;

			// возвращаем наименьший из действительных корней
			rH = (h + (std::min)(h1, h2)) / h;
		}
	}

	return rH;
}

std::string CDynaPrimitive::GetVerboseName()
{
	const auto pRightVector = m_Device.GetModel()->GetRightVector(m_Output);
	const char* cszUnknown = "\"unknown\"";
	return fmt::format("{} {} {} {}", 
		GetVerbalName(), 
		m_Device.GetVerbalName(), 
		pRightVector ? m_Device.VariableNameByPtr(pRightVector->pValue) : cszUnknown,
		pRightVector ? *pRightVector->pValue : 0.0);
}
