#include "stdafx.h"
#include "DynaModel.h"


using namespace DFW2;

// рассчитывает прогноз Nordsieck для заданного шага
void CDynaModel::Predict()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	// Алгоритм расчета [Lsode 2.61]
	while (pVectorBegin < pVectorEnd)
	{
		pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;

		for (ptrdiff_t k = 0; k < sc.q; k++)
		{
			for (ptrdiff_t j = sc.q; j >= k + 1; j--)
			{
				pVectorBegin->Nordsiek[j - 1] += pVectorBegin->Nordsiek[j];
			}
		}

		// прогнозное значение переменной состояния обновляем по Nordsieck
		*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
		pVectorBegin->Error = 0.0;	// обнуляем ошибку шага
		pVectorBegin++;
	}

	ConvTest[0].ResetIterations();
	ConvTest[1].ResetIterations();

	// для устройств, которые требует внутренней обработки прогноза
	// (например для узлов, которым нужно перевести прогноз полярного напряжения в прямоугольное)
	// делаем цикл с вызовом функции прогноза устройства
	for (DEVICECONTAINERITR it = m_DeviceContainersPredict.begin(); it != m_DeviceContainersPredict.end(); it++)
	{
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
		{
			(*dit)->Predict();
		}
	}

}

void CDynaModel::InitDevicesNordsiek()
{
	ChangeOrder(1);
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
		{
			(*dit)->InitNordsiek(this);
		}
	}
}

void CDynaModel::InitNordsiek()
{
	InitDevicesNordsiek();

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	while (pVectorBegin < pVectorEnd)
	{
		InitNordsiekElement(pVectorBegin,GetAtol(),GetRtol());
		pVectorBegin++;
	}

	sc.StepChanged();
	sc.OrderChanged();
	sc.m_bNordsiekSaved = false;
}

void CDynaModel::ResetNordsiek()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	if (sc.m_bNordsiekSaved)
	{
		while (pVectorBegin < pVectorEnd)
		{
			pVectorBegin->Tminus2Value = pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0] = *pVectorBegin->pValue;
			pVectorBegin->Nordsiek[1] = pVectorBegin->SavedNordsiek[1] = 0.0;
			pVectorBegin->SavedError = 0.0;
			pVectorBegin++;
		}
		BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
}

// построение Nordsieck после того, как обнаружено что текущая история
// ненадежна
void CDynaModel::ReInitializeNordsiek()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	if (sc.m_bNordsiekSaved)
	{
		// если есть сохраненный шаг
		while (pVectorBegin < pVectorEnd)
		{
			// значение восстанавливаем
			pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0];
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			// а элемент первого порядка конструируем по разности между предыдущим и пред-предыдущим значениями
			// т.к. первый элемент это hy', а y' = (y(-1) - y(-2)) / h. Note - мы берем производную с пред-предыдущего шага

			//											y(-1)						y(-2)
			pVectorBegin->Nordsiek[1] = pVectorBegin->SavedNordsiek[0] - pVectorBegin->Tminus2Value;
			pVectorBegin->SavedError = 0.0;
			pVectorBegin++;
		}
		// масшатабируем Nordsieck на заданный шаг
		RescaleNordsiek(sc.m_dCurrentH / sc.m_dOldH);
		BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
	sc.ResetStepsToFail();
}

// восстанавление Nordsieck с предыдущего шага
void CDynaModel::RestoreNordsiek()
{

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	if (sc.m_bNordsiekSaved)
	{
		// если есть данные для восстановления - просто копируем предыдущий шаг
		// в текущий
		while (pVectorBegin < pVectorEnd)
		{
			double *pN = pVectorBegin->Nordsiek;
			double *pS = pVectorBegin->SavedNordsiek;
			*pN = *pS; pS++; pN++;
			*pN = *pS; pS++; pN++;
			*pN = *pS; pS++; pN++;

			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			pVectorBegin->Error = pVectorBegin->SavedError;

			pVectorBegin++;
		}
	}
	else
	{
		// если данных для восстановления нет
		// считаем что прозводные нулевые
		// это слабая надежда, но лучше чем ничего
		while (pVectorBegin < pVectorEnd)
		{
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			pVectorBegin->Nordsiek[1] = pVectorBegin->Nordsiek[2] = 0.0;
			pVectorBegin->Error = 0.0;

			pVectorBegin++;
		}
	}

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
			(*dit)->RestoreStates();
}

// обновляение Nordsieck после выполнения шага
void CDynaModel::UpdateNordsiek(bool bAllowSuppression)
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	double alpha = sc.m_dCurrentH / sc.m_dOldH > 0.0 ? sc.m_dOldH : 1.0;
	double alphasq = alpha * alpha;
	double alpha1 = (1.0 + alpha);
	double alpha2 = (1.0 + 2.0 * alpha);
	bool bSuprressRinging = false;

	if (m_Parameters.m_bAllowRingingSuppression && bAllowSuppression)
	{
		if (sc.q == 2 && sc.nStepsCount % 10 == 0 && sc.m_dCurrentH > 0.01 && sc.m_dOldH > 0.0)
			bSuprressRinging = true;
	}

	// обновление по [Lsode 2.76]
	while (pVectorBegin < pVectorEnd)
	{
		// выбираем коэффициент метода по типу уравнения EquationType
		const double *lm = l[pVectorBegin->EquationType * 2 + sc.q - 1];

		double dError = pVectorBegin->Error;
		pVectorBegin->Nordsiek[0] += dError * lm[0];
		pVectorBegin->Nordsiek[1] += dError * lm[1];
		pVectorBegin->Nordsiek[2] += dError * lm[2];

		// подавление рингинга
		if(bSuprressRinging && pVectorBegin->EquationType == DET_DIFFERENTIAL)
		{
			pVectorBegin->Nordsiek[1] = (alphasq * pVectorBegin->Tminus2Value - alpha1 * alpha1 * pVectorBegin->SavedNordsiek[0] + alpha2 * pVectorBegin->Nordsiek[0]) / alpha1;
		}

		// сохраняем пред-предыдущее значение переменной состояния
		pVectorBegin->Tminus2Value = pVectorBegin->SavedNordsiek[0];

		// и сохраняем текущее значение переменных состояния перед новым шагом
		pVectorBegin->SavedNordsiek[0] = pVectorBegin->Nordsiek[0];
		pVectorBegin->SavedNordsiek[1] = pVectorBegin->Nordsiek[1];
		pVectorBegin->SavedNordsiek[2] = pVectorBegin->Nordsiek[2];

		pVectorBegin->SavedError = dError;

		pVectorBegin++;
	}
	sc.m_dOldH = sc.m_dCurrentH;
	sc.m_bNordsiekSaved = true;

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
			(*dit)->StoreStates();


	// даем информацию для обработки разрывов о том, что данный момент
	// времени пройден
	m_Discontinuities.PassTime(GetCurrentTime());
	sc.ResetStepsToFail();
}

// сохранение копии Nordsieck перед выполнением шага
void CDynaModel::SaveNordsiek()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	while (pVectorBegin < pVectorEnd)
	{
		double *pN = pVectorBegin->Nordsiek;
		double *pS = pVectorBegin->SavedNordsiek;

		// сохраняем пред-предыдущее значение переменной состояния
		pVectorBegin->Tminus2Value = *pS;

		// запоминаем три элемента, не смотря на текущий порядок
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;

		pVectorBegin->SavedError = pVectorBegin->Error;
		pVectorBegin++;
	}
	sc.m_dOldH = sc.m_dCurrentH;
	sc.m_bNordsiekSaved = true;
}

// масштабирование Nordsieck на заданный коэффициент изменения шага
void CDynaModel::RescaleNordsiek(double r)
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;
	
	// расчет выполняется путем умножения текущего Nordsieck на диагональную матрицу C[q+1;q+1]
	// с элементами C[i,i] = r^(i-1) [Lsode 2.64]
	while (pVectorBegin < pVectorEnd)
	{
		double R = 1.0;
		for (ptrdiff_t j = 1; j < sc.q + 1; j++)
		{
			R *= r;
			pVectorBegin->Nordsiek[j] *= R;
		}
		pVectorBegin++;
	}

	// вызываем функции обработки изменения шага и порядка
	// пока они блокируют дальнейшее увеличение шага на протяжении
	// заданного количества шагов
	sc.StepChanged();
	sc.OrderChanged();

	// рассчитываем коэффициент изменения шага
	double dRefactorRatio = sc.m_dCurrentH / sc.m_dLastRefactorH;
	// если шаг изменился более в заданное количество раз - взводим флаг рефакторизации Якоби
	// sc.m_dLastRefactorH обновляется после рефакторизации
	if (dRefactorRatio > m_Parameters.m_dRefactorByHRatio || dRefactorRatio < 1.0 / m_Parameters.m_dRefactorByHRatio)
		sc.RefactorMatrix(true);
}

void CDynaModel::ConstructNordsiekOrder()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	while (pVectorBegin < pVectorEnd)
	{
		pVectorBegin->Nordsiek[2] = pVectorBegin->Error / 2.0;
		pVectorBegin++;
	}
}


void CDynaModel::InitNordsiekElement(struct RightVector *pVectorBegin, double Atol, double Rtol)
{
	ZeroMemory(pVectorBegin->Nordsiek + 1, sizeof(double) * 2);
	ZeroMemory(pVectorBegin->SavedNordsiek, sizeof(double) * 3);
	pVectorBegin->EquationType = DET_ALGEBRAIC;
	pVectorBegin->Atol = Atol;
	pVectorBegin->Rtol = Rtol;
	pVectorBegin->SavedError = pVectorBegin->Tminus2Value = 0.0;
	pVectorBegin->nErrorHits = 0;
}

void CDynaModel::PrepareNordsiekElement(struct RightVector *pVectorBegin)
{
	pVectorBegin->Error = 0.0;
	pVectorBegin->PhysicalEquationType = DET_ALGEBRAIC;
}