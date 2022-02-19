#include "stdafx.h"
#include "DynaModel.h"
#include <immintrin.h>

//#define _AVX2

using namespace DFW2;

// рассчитывает прогноз Nordsieck для заданного шага
void CDynaModel::Predict()
{
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

//#define DBG_CHECK_PREDICTION

#ifdef DBG_CHECK_PREDICTION
	SnapshotRightVector();
#endif
	
	for (pVectorBegin = pRightVector; pVectorBegin < pVectorEnd ; pVectorBegin++)
	{
		pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;
		pVectorBegin->Error = 0.0;	// обнуляем ошибку шага
	}

	if (!sc.m_bNordsiekReset)
	{
		// Алгоритм расчета [Lsode 2.61]
		for (pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{
			/*
			for (ptrdiff_t k = 0; k < sc.q; k++)
			{
				for (ptrdiff_t j = sc.q; j >= k + 1; j--)
				{
					pVectorBegin->Nordsiek[j - 1] += pVectorBegin->Nordsiek[j];
				}
			}
			*/

			//
			// |x0|x1|x2| * |1|0|0|
			//				|1|1|0|
			//				|1|2|1|

			if (sc.q == 2)
			{
				pVectorBegin->Nordsiek[1] += pVectorBegin->Nordsiek[2];
				pVectorBegin->Nordsiek[0] += pVectorBegin->Nordsiek[1];
				pVectorBegin->Nordsiek[1] += pVectorBegin->Nordsiek[2];
			}
			else
				pVectorBegin->Nordsiek[0] += pVectorBegin->Nordsiek[1];

			// прогнозное значение переменной состояния обновляем по Nordsieck
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
		}
	}

#ifdef DBG_CHECK_PREDICTION
	// после выполнения прогноза рассчитываем разность
	// между спрогнозированным значением и исходным
	CompareRightVector();
#endif

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::ResetIterations);

	// для устройств, которые требует внутренней обработки прогноза
	// (например для узлов, которым нужно перевести прогноз полярного напряжения в прямоугольное)
	// делаем цикл с вызовом функции прогноза устройства
	for (auto&& it : m_DeviceContainersPredict)
		it->Predict();
}

void CDynaModel::InitDevicesNordsiek()
{
	ChangeOrder(1);
	for (auto&& it : m_DeviceContainers)
		it->InitNordsieck(this);
}

void CDynaModel::InitNordsiek()
{
	InitDevicesNordsiek();

	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		InitNordsiekElement(pVectorBegin,GetAtol(),GetRtol());

	sc.StepChanged();
	sc.OrderChanged();
	sc.m_bNordsiekSaved = false;
}

// сброс элементов Нордиска (уничтожение истории)
void CDynaModel::ResetNordsiek()
{
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	if (sc.m_bNordsiekSaved)
	{
		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{
			// в качестве значения принимаем то, что рассчитано в устройстве
			pVectorBegin->Tminus2Value = pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0] = *pVectorBegin->pValue;
			// первая производная равна нулю (вторая тоже, но после Reset мы ее не используем, т.к. работаем на первом порядке
			pVectorBegin->Nordsiek[1] = pVectorBegin->SavedNordsiek[1] = 0.0;
			pVectorBegin->SavedError = 0.0;
		}
		// запрашиваем расчет производных дифференциальных уравнений у устройств, которые это могут
		BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
	// ставим флаг ресета Нордсика, чтобы не
	// контролировать соответствие предиктора-корректору
	// на стартап-шаге
	sc.m_bNordsiekReset = true;
}

// построение Nordsieck после того, как обнаружено что текущая история
// ненадежна
void CDynaModel::ReInitializeNordsiek()
{
	
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	if (sc.m_bNordsiekSaved)
	{
		// если есть сохраненный шаг
		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{
			// значение восстанавливаем
			pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0];
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			// а элемент первого порядка конструируем по разности между предыдущим и пред-предыдущим значениями
			// т.к. первый элемент это hy', а y' = (y(-1) - y(-2)) / h. Note - мы берем производную с пред-предыдущего шага

			//											y(-1)						y(-2)
			pVectorBegin->Nordsiek[1] = pVectorBegin->SavedNordsiek[0] - pVectorBegin->Tminus2Value;
			pVectorBegin->SavedError = 0.0;
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
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	if (sc.m_bNordsiekSaved)
	{
		// если есть данные для восстановления - просто копируем предыдущий шаг
		// в текущий
		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{

#ifdef _AVX2
			_mm256_store_pd(pVectorBegin->Nordsiek, _mm256_load_pd(pVectorBegin->SavedNordsiek));
#else
			double* pN = pVectorBegin->Nordsiek;
			double* pS = pVectorBegin->SavedNordsiek;
			*pN = *pS; pS++; pN++;
			*pN = *pS; pS++; pN++;
			*pN = *pS; pS++; pN++;
#endif
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			pVectorBegin->Error = pVectorBegin->SavedError;
		}
	}
	else
	{
		// если данных для восстановления нет
		// считаем что прозводные нулевые
		// это слабая надежда, но лучше чем ничего
		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			pVectorBegin->Nordsiek[1] = pVectorBegin->Nordsiek[2] = 0.0;
			pVectorBegin->Error = 0.0;
		}
	}

	for (auto&& it : m_DeviceContainers)
		for (auto&& dit : *it)
			dit->RestoreStates();
}

bool CDynaModel::DetectAdamsRinging()
{
	sc.bRingingDetected = false;

	if ((m_Parameters.m_eAdamsRingingSuppressionMode == ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA ||
		m_Parameters.m_eAdamsRingingSuppressionMode == ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL) &&
		sc.q == 2 && sc.m_dCurrentH > 0.01 && sc.m_dOldH > 0.0)
	{
		const double Methodl1[2] { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][1],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][1] };
		const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{
#ifdef USE_FMA
			double newValue = std::fma(pVectorBegin->Error, Methodl1[pVectorBegin->EquationType], pVectorBegin->Nordsiek[1]) ;
#else 
			double newValue = pVectorBegin->Error *  Methodl1[pVectorBegin->EquationType] +  pVectorBegin->Nordsiek[1];
#endif
			// если знак производной изменился - увеличиваем счетчик циклов
			if (std::signbit(newValue) != std::signbit(pVectorBegin->SavedNordsiek[1]) && std::abs(newValue) > pVectorBegin->Atol * sc.m_dCurrentH * 5.0)
				pVectorBegin->RingsCount++;
			else
				pVectorBegin->RingsCount = 0;

			// если счетчик циклов изменения знака достиг порога
			if (pVectorBegin->RingsCount > m_Parameters.m_nAdamsIndividualSuppressionCycles)
			{
				pVectorBegin->RingsCount = 0;
				sc.bRingingDetected = true;
				switch (m_Parameters.m_eAdamsRingingSuppressionMode)
				{
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
						if (pVectorBegin->EquationType == DET_DIFFERENTIAL)
						{
							// в RightVector устанавливаем количество шагов, на протяжении которых производная Адамса будет заменяться 
							// на производную  BDF
							pVectorBegin->RingsSuppress = m_Parameters.m_nAdamsIndividualSuppressStepsRange;
						}
						break;
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA:
						pVectorBegin->RingsSuppress = m_Parameters.m_nAdamsIndividualSuppressStepsRange;
						break;
				}
				
				if (pVectorBegin->RingsSuppress)
				{
					Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Ringing {} {} last values {} {} {}",
						pVectorBegin->pDevice->GetVerbalName(),
						pVectorBegin->pDevice->VariableNameByPtr(pVectorBegin->pValue),
						newValue,
						pVectorBegin->SavedNordsiek[0],
						pVectorBegin->RingsSuppress));
				}
			}
		}

		if (sc.bRingingDetected)
			sc.nNoRingingSteps = 0;
		else
			sc.nNoRingingSteps++;

	}
	return sc.bRingingDetected;
}

// обновляение Nordsieck после выполнения шага
void CDynaModel::UpdateNordsiek(bool bAllowSuppression)
{
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	const double alpha{ sc.m_dCurrentH / sc.m_dOldH > 0.0 ? sc.m_dOldH : 1.0 };
	const double alphasq{ alpha * alpha };
	const double alpha1{ (1.0 + alpha) };
	double alpha2{ 1.0 + 2.0 * alpha };
	bool bSuprressRinging{ false };

	// режим подавления рингинга активируем если порядок метода 2
	// шаг превышает 0.01 и UpdateNordsieck вызван для перехода к следующему
	// шагу после успешного завершения предыдущего
	if (sc.q == 2 && bAllowSuppression && sc.m_dCurrentH > 0.01 && sc.m_dOldH > 0.0)
	{
		switch (m_Parameters.m_eAdamsRingingSuppressionMode)
		{
		case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL:
			// в глобальном режиме подавления разрешаем подавления на каждом кратном m_nAdamsGlobalSuppressionStep шаге
			if (sc.nStepsCount % m_Parameters.m_nAdamsGlobalSuppressionStep == 0 && sc.nStepsCount > m_Parameters.m_nAdamsGlobalSuppressionStep)
				bSuprressRinging = true;
			break;
		case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
			// в индивидуальном режиме проверяем каждую переменную на рингинг вне зависимости от номера шага
			bSuprressRinging = true;
			break;
		}
	}

	// обновление по [Lsode 2.76]

	// делаем локальную копию коэффициентов метода для текущего порядка
	alignas(32) double LocalMethodl[2][3];
	std::copy(&Methodl[sc.q - 1][0], &Methodl[sc.q - 1][3], &LocalMethodl[0][0]);
	std::copy(&Methodl[sc.q + 1][0], &Methodl[sc.q + 1][3], &LocalMethodl[1][0]);

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		// выбираем коэффициент метода по типу уравнения EquationType
		const double& Error{ pVectorBegin->Error };
		const double* lm = LocalMethodl[pVectorBegin->EquationType];
#ifdef _AVX2
		const __m256d err = _mm256_set1_pd(pVectorBegin->Error);
		__m256d lmms = _mm256_load_pd(lm);
		__m256d nord = _mm256_load_pd(pVectorBegin->Nordsiek);
		nord = _mm256_fmadd_pd(err, lmms, nord);
		_mm256_store_pd(pVectorBegin->Nordsiek, nord);
#else
		pVectorBegin->Nordsiek[0] += Error * *lm;	lm++;
		pVectorBegin->Nordsiek[1] += Error * *lm;	lm++;
		pVectorBegin->Nordsiek[2] += Error * *lm;
#endif
	}

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		pVectorBegin->SavedError = pVectorBegin->Error;
		// подавление рингинга
		if (bSuprressRinging)
		{
			if (pVectorBegin->EquationType == DET_DIFFERENTIAL)
			{
				// рингинг подавляем только для дифуров (если дифуры решаются BDF надо сбрасывать подавление в ARSM_NONE)
				switch (m_Parameters.m_eAdamsRingingSuppressionMode)
				{
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
						if (pVectorBegin->RingsSuppress > 0)
						{
							// для переменных, у которых количество шагов для замены Адамса на BDF не исчерпано
							// делаем эту замену и уменьшаем счетчик
							pVectorBegin->Nordsiek[1] = (alphasq * pVectorBegin->Tminus2Value - alpha1 * alpha1 * pVectorBegin->SavedNordsiek[0] + alpha2 * pVectorBegin->Nordsiek[0]) / alpha1;
							pVectorBegin->RingsSuppress--;
							pVectorBegin->RingsCount = 0;
						}
					break;
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL:
						// в глобальном режиме просто заменяем производную Адамса на производную BDF
						pVectorBegin->Nordsiek[1] = (alphasq * pVectorBegin->Tminus2Value - alpha1 * alpha1 * pVectorBegin->SavedNordsiek[0] + alpha2 * pVectorBegin->Nordsiek[0]) / alpha1;
					break;
				}
			}
		}

		// сохраняем пред-предыдущее значение переменной состояния
		pVectorBegin->Tminus2Value = pVectorBegin->SavedNordsiek[0];

		// и сохраняем текущее значение переменных состояния перед новым шагом
#ifdef _AVX2
		_mm256_store_pd(pVectorBegin->SavedNordsiek, _mm256_load_pd(pVectorBegin->Nordsiek));
#else
		pVectorBegin->SavedNordsiek[0] = pVectorBegin->Nordsiek[0];
		pVectorBegin->SavedNordsiek[1] = pVectorBegin->Nordsiek[1];
		pVectorBegin->SavedNordsiek[2] = pVectorBegin->Nordsiek[2];
#endif
	}

	sc.m_dOldH = sc.m_dCurrentH;
	sc.m_bNordsiekSaved = true;
	// после того как Нордсик обновлен,
	// сбрасываем флаг ресета, начинаем работу предиктора
	// и контроль соответствия предиктора корректору
	sc.m_bNordsiekReset = false;

	for (auto&& it : m_DeviceContainers)
		for (auto&& dit : *it)
			dit->StoreStates();

	if (m_Parameters.m_eAdamsRingingSuppressionMode == ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA)
	{
		if(sc.bRingingDetected)
			EnableAdamsCoefficientDamping(true);
		else if(sc.nNoRingingSteps > m_Parameters.m_nAdamsDampingSteps)
			EnableAdamsCoefficientDamping(false);
	}


	// даем информацию для обработки разрывов о том, что данный момент
	// времени пройден
	m_Discontinuities.PassTime(GetCurrentTime());
	sc.ResetStepsToFail();
}

// сохранение копии Nordsieck перед выполнением шага
void CDynaModel::SaveNordsiek()
{
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
#ifdef _AVX2
		_mm256_store_pd(pVectorBegin->Nordsiek, _mm256_load_pd(pVectorBegin->SavedNordsiek));
#else
		double *pN = pVectorBegin->Nordsiek;
		double *pS = pVectorBegin->SavedNordsiek;

		// сохраняем пред-предыдущее значение переменной состояния
		pVectorBegin->Tminus2Value = *pS;

		// запоминаем три элемента, не смотря на текущий порядок
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;
#endif

		pVectorBegin->SavedError = pVectorBegin->Error;
	}
	sc.m_dOldH = sc.m_dCurrentH;
	sc.m_bNordsiekSaved = true;
}

// масштабирование Nordsieck на заданный коэффициент изменения шага
void CDynaModel::RescaleNordsiek(const double r)
{
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };
	
	// расчет выполняется путем умножения текущего Nordsieck на диагональную матрицу C[q+1;q+1]
	// с элементами C[i,i] = r^(i-1) [Lsode 2.64]

	const double crs[4] = { 1.0, r, (sc.q == 2) ? r * r : 1.0 , 1.0 };
#ifdef _AVX2
	const __m256d rs = _mm256_load_pd(crs);
#endif
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
#ifdef _AVX2
		__m256d nord = _mm256_load_pd(pVectorBegin->Nordsiek);
		nord = _mm256_mul_pd(nord, rs);
		_mm256_store_pd(pVectorBegin->Nordsiek,nord);
#else
		for (ptrdiff_t j = 1; j < sc.q + 1; j++)
			pVectorBegin->Nordsiek[j] *= crs[j];
#endif
	}

	// вызываем функции обработки изменения шага и порядка
	// пока они блокируют дальнейшее увеличение шага на протяжении
	// заданного количества шагов
	sc.StepChanged();
	sc.OrderChanged();

	// рассчитываем коэффициент изменения шага
	const double dRefactorRatio{ sc.m_dCurrentH / sc.m_dLastRefactorH };
	// если шаг изменился более в заданное количество раз - взводим флаг рефакторизации Якоби
	// sc.m_dLastRefactorH обновляется после рефакторизации
	if (dRefactorRatio > m_Parameters.m_dRefactorByHRatio || 1.0 / dRefactorRatio > m_Parameters.m_dRefactorByHRatio)
		sc.RefactorMatrix(true);
}

void CDynaModel::ConstructNordsiekOrder()
{
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		pVectorBegin->Nordsiek[2] = pVectorBegin->Error / 2.0;
}


void CDynaModel::InitNordsiekElement(struct RightVector *pVectorBegin, double Atol, double Rtol)
{
	std::fill(pVectorBegin->Nordsiek + 1, pVectorBegin->Nordsiek + 3, 0.0);
	std::fill(pVectorBegin->SavedNordsiek, pVectorBegin->SavedNordsiek + 3, 0.0);
	pVectorBegin->EquationType = DET_ALGEBRAIC;
	pVectorBegin->PrimitiveBlock = PBT_UNKNOWN;
	pVectorBegin->Atol = Atol;
	pVectorBegin->Rtol = Rtol;
	pVectorBegin->SavedError = pVectorBegin->Tminus2Value = 0.0;
	pVectorBegin->ErrorHits = 0;
	pVectorBegin->RingsCount = 0;
	pVectorBegin->RingsSuppress = 0;
}

void CDynaModel::PrepareNordsiekElement(struct RightVector *pVectorBegin)
{
	pVectorBegin->Error = 0.0;
	pVectorBegin->PhysicalEquationType = DET_ALGEBRAIC;
}


void CDynaModel::FinishStep()
{
	// для устройств с независимыми переменными после успешного выполнения шага
	// рассчитываем актуальные значения независимых переменных
	for (auto&& it : m_DeviceContainersFinishStep)
		it->FinishStep();
}