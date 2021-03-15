#include "stdafx.h"
#include "DynaModel.h"


using namespace DFW2;

// рассчитывает прогноз Nordsieck для заданного шага
void CDynaModel::Predict()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

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

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

	while (pVectorBegin < pVectorEnd)
	{
		InitNordsiekElement(pVectorBegin,GetAtol(),GetRtol());
		pVectorBegin++;
	}

	sc.StepChanged();
	sc.OrderChanged();
	sc.m_bNordsiekSaved = false;
}

// сброс элементов Нордиска (уничтожение истории)
void CDynaModel::ResetNordsiek()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

	if (sc.m_bNordsiekSaved)
	{
		while (pVectorBegin < pVectorEnd)
		{
			// в качестве значения принимаем то, что рассчитано в устройстве
			pVectorBegin->Tminus2Value = pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0] = *pVectorBegin->pValue;
			// первая производная равна нулю (вторая тоже, но после Reset мы ее не используем, т.к. работаем на первом порядке
			pVectorBegin->Nordsiek[1] = pVectorBegin->SavedNordsiek[1] = 0.0;
			pVectorBegin->SavedError = 0.0;
			pVectorBegin++;
		}
		// запрашиваем расчет производных дифференциальных уравнений у устройств, которые это могут
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
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

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
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

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
		const double Methodl1[2] = { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][1],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][1] };
		struct RightVector *pVectorBegin = pRightVector;
		struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

		while (pVectorBegin < pVectorEnd)
		{
			double newValue = pVectorBegin->Nordsiek[1] + pVectorBegin->Error * Methodl1[pVectorBegin->EquationType];
			// если знак производной изменился - увеличиваем счетчик циклов
			if (std::signbit(newValue) != std::signbit(pVectorBegin->SavedNordsiek[1]) && fabs(newValue) > pVectorBegin->Atol * sc.m_dCurrentH * 5.0)
				pVectorBegin->nRingsCount++;
			else
				pVectorBegin->nRingsCount = 0;

			// если счетчик циклов изменения знака достиг порога
			if (pVectorBegin->nRingsCount > m_Parameters.m_nAdamsIndividualSuppressionCycles)
			{
				pVectorBegin->nRingsCount = 0;
				sc.bRingingDetected = true;
				switch (m_Parameters.m_eAdamsRingingSuppressionMode)
				{
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
						if (pVectorBegin->EquationType == DET_DIFFERENTIAL)
						{
							// в RightVector устанавливаемколичество шагов, на протяжении которых производная Адамса будет заменяться 
							// на производную  BDF
							pVectorBegin->nRingsSuppress = m_Parameters.m_nAdamsIndividualSuppressStepsRange;
						}
						break;
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA:
						pVectorBegin->nRingsSuppress = m_Parameters.m_nAdamsIndividualSuppressStepsRange;
						break;
				}
				
				if (pVectorBegin->nRingsSuppress)
				{
					Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Ringing {} {} last values {} {} {}",
						pVectorBegin->pDevice->GetVerbalName(),
						pVectorBegin->pDevice->VariableNameByPtr(pVectorBegin->pValue),
						newValue,
						pVectorBegin->SavedNordsiek[0],
						pVectorBegin->nRingsSuppress));
				}
			}

			pVectorBegin++;
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
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

	double alpha = sc.m_dCurrentH / sc.m_dOldH > 0.0 ? sc.m_dOldH : 1.0;
	double alphasq = alpha * alpha;
	double alpha1 = (1.0 + alpha);
	double alpha2 = (1.0 + 2.0 * alpha);
	bool bSuprressRinging = false;

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
	double LocalMethodl[2][3];
	std::copy(&Methodl[sc.q - 1][0], &Methodl[sc.q - 1][3], &LocalMethodl[0][0]);
	std::copy(&Methodl[sc.q + 1][0], &Methodl[sc.q + 1][3], &LocalMethodl[1][0]);

	while (pVectorBegin < pVectorEnd)
	{
		// выбираем коэффициент метода по типу уравнения EquationType
		const double *lm = LocalMethodl[pVectorBegin->EquationType];
		double dError = pVectorBegin->Error;
#ifdef USE_FMA
		pVectorBegin->Nordsiek[0] = FMA(dError, *lm, pVectorBegin->Nordsiek[0]);	lm++;
		pVectorBegin->Nordsiek[1] = FMA(dError, *lm, pVectorBegin->Nordsiek[1]);	lm++;
		pVectorBegin->Nordsiek[2] = FMA(dError, *lm, pVectorBegin->Nordsiek[2]); 
#else
		pVectorBegin->Nordsiek[0] += dError * *lm;	lm++;
		pVectorBegin->Nordsiek[1] += dError * *lm;	lm++;
		pVectorBegin->Nordsiek[2] += dError * *lm;	
#endif

		// подавление рингинга
		if (bSuprressRinging)
		{
			if (pVectorBegin->EquationType == DET_DIFFERENTIAL)
			{
				// рингинг подавляем только для дифуров (если дифуры решаются BDF надо сбрасывать подавление в ARSM_NONE)
				switch (m_Parameters.m_eAdamsRingingSuppressionMode)
				{
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
						if (pVectorBegin->nRingsSuppress > 0)
						{
							// для переменных, у которых количество шагов для замены Адамса на BDF не исчерпано
							// делаем эту замену и уменьшаем счетчик
							pVectorBegin->Nordsiek[1] = (alphasq * pVectorBegin->Tminus2Value - alpha1 * alpha1 * pVectorBegin->SavedNordsiek[0] + alpha2 * pVectorBegin->Nordsiek[0]) / alpha1;
							pVectorBegin->nRingsSuppress--;
							pVectorBegin->nRingsCount = 0;
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
		pVectorBegin->SavedNordsiek[0] = pVectorBegin->Nordsiek[0];
		pVectorBegin->SavedNordsiek[1] = pVectorBegin->Nordsiek[1];
		pVectorBegin->SavedNordsiek[2] = pVectorBegin->Nordsiek[2];

		pVectorBegin->SavedError = dError;

		pVectorBegin++;
	}

	sc.m_dOldH = sc.m_dCurrentH;
	sc.m_bNordsiekSaved = true;

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
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

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
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();
	
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
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

	while (pVectorBegin < pVectorEnd)
	{
		pVectorBegin->Nordsiek[2] = pVectorBegin->Error / 2.0;
		pVectorBegin++;
	}
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
	pVectorBegin->nErrorHits = 0;
	pVectorBegin->nRingsCount = 0;
	pVectorBegin->nRingsSuppress = 0;
}

void CDynaModel::PrepareNordsiekElement(struct RightVector *pVectorBegin)
{
	pVectorBegin->Error = 0.0;
	pVectorBegin->PhysicalEquationType = DET_ALGEBRAIC;
}