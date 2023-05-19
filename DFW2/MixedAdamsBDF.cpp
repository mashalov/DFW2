#include "stdafx.h"
#include "DynaModel.h"
#include "MixedAdamsBDF.h"

using namespace DFW2;
MixedAdamsBDF::MixedAdamsBDF(CDynaModel& DynaModel) : IntegratorBase(DynaModel)
{
	std::copy(&MethodlDefault[0][0], &MethodlDefault[0][0] + sizeof(MethodlDefault) / sizeof(MethodlDefault[0][0]), &Methodl[0][0]);
}

void MixedAdamsBDF::Step()
{
	Predict();
}


void MixedAdamsBDF::AcceptStep(bool DisableStepControl)
{
	auto& sc{ DynaModel_.StepControl() };

	sc.m_bRetryStep = false;		// отказываемся от повтора шага, все хорошо
	sc.RefactorMatrix(false);		// отказываемся от рефакторизации Якоби
	sc.nSuccessfullStepsOfNewton++;
	sc.nMinimumStepFailures = 0;
	// рассчитываем количество успешных шагов и пройденного времени для каждого порядка
	sc.OrderStatistics[sc.q - 1].nSteps++;
	// переходим к новому рассчитанному времени с обновлением суммы Кэхэна
	sc.Advance_t0();

	double StepRatio{ StepRatio_ };

	if (StepRatio > 1.2 && !DisableStepControl)
	{
		// если шаг можно хорошо увеличить
		StepRatio /= 1.2;

		switch (sc.q)
		{
		case 1:
		{
			// если были на первом порядке, пробуем шаг для второго порядка
			const double rHigher{ GetRatioForHigherOrder() / 1.4 };
			// call before step change
			UpdateNordsiek();

			// если второй порядок лучше чем первый
			if (rHigher > StepRatio)
			{
				// пытаемся перейти на второй порядок
				if (sc.FilterOrder(rHigher))
				{
					ConstructNordsiekOrder();
					ChangeOrder(2);
					DynaModel_.SetH(DynaModel_.H() * sc.dFilteredOrder);
					RescaleNordsiek();
					DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG,
						fmt::format(CDFW2Messages::m_cszStepAndOrderChanged,
							sc.q,
							DynaModel_.H()));
				}
			}
			else
				sc.OrderChanged();	// сбрасываем фильтр изменения порядка, если перейти на второй неэффективно
		}
		break;

		case 2:
		{
			// если были на втором порядке, пробуем шаг для первого порядка
			const double rLower{ GetRatioForLowerOrder() / 1.3 }; // 5.0 - чтобы уменьшить использование демпфирующего метода
			// call before step change
			UpdateNordsiek();

			// если первый порядок лучше чем второй
			if (rLower > StepRatio)
			{
				// пытаемся перейти на первый порядок
				if (sc.FilterOrder(rLower))
				{
					DynaModel_.SetH(DynaModel_.H() * sc.dFilteredOrder);
					ChangeOrder(1);
					RescaleNordsiek();
					DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG,
						fmt::format(CDFW2Messages::m_cszStepAndOrderChanged,
							sc.q,
							DynaModel_.H()));
				}
			}
			else
				sc.OrderChanged();	// сбрасываем фильтр изменения порядка, если перейти на первый неэффективно
		}
		break;
		}

		// запрашиваем возможность изменения (увеличения шага)
		// тут можно регулировать частоту увеличения шага 
		// rSame уже поделили выше для безопасности на 1.2
		if (sc.FilterStep(StepRatio) && StepRatio > 1.1)
		{

			// если фильтр дает разрешение на увеличение
			_ASSERTE(Equal(H(), UsedH()));
			// запоминаем коэффициент увеличения только для репорта
			// потому что sc.dFilteredStep изменится в последующем 
			// RescaleNordsiek
			const double k{ sc.dFilteredStep };
			// рассчитываем новый шаг
			// пересчитываем Nordsieck на новый шаг
			DynaModel_.SetH(DynaModel_.H() * sc.dFilteredStep);
			RescaleNordsiek();
			DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszStepChanged,
				DynaModel_.H(),
				k,
				sc.q));
			sc.StepChanged();
		}
	}
	else
	{
		// если шаг хорошо увеличить нельзя, обновляем Nordsieck, сбрасываем фильтр шага, если его увеличивать неэффективно
		UpdateNordsiek(true);
		// если мы находимся здесь - можно включить BDF по дифурам, на случай, если шаг не растет из-за рингинга
		// но нужно посчитать сколько раз мы не смогли увеличить шаг и перейти на BDF, скажем, на десятом шагу
		// причем делать это нужно после вызова UpdateNordsiek(), так как в нем BDF отменяется
		sc.StepChanged();
	}
}

void MixedAdamsBDF::UpdateStepSize()
{
	Computehl0();
}

void MixedAdamsBDF::RejectStep()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };

	double newH{ 0.5 * DynaModel_.H() };
	double newEffectiveH{ (std::max)(newH, sc.Hmin) };

	sc.RefactorMatrix(true);	// принудительно рефакторизуем матрицу
	sc.m_bEnforceOut = false;	// отказываемся от вывода данных на данном заваленном шаге

	DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszStepChangedOnError, 
		newEffectiveH,
		sc.Integrator.Weighted.Info()));

	// считаем статистику по заваленным шагам для текущего порядка метода интегрирования
	sc.OrderStatistics[sc.q - 1].nFailures++;


	if (newH < sc.Hmin * 10.0)
		ChangeOrder(1);

	// если шаг снизился до минимума
	if (newH <= sc.Hmin && Equal(DynaModel_.H(), sc.Hmin))
	{
		if (++sc.nMinimumStepFailures > Parameters.m_nMinimumStepFailures)
			throw dfw2error(fmt::format(CDFW2Messages::m_cszFailureAtMinimalStep, 
				DynaModel_.GetCurrentTime(), 
				DynaModel_.GetIntegrationStepNumber(), 
				DynaModel_.Order(),
				DynaModel_.H()));

		// проверяем количество последовательно
		// заваленных шагов
		if (sc.StepFailLimit())
		{
			// если можно еще делать шаги,
			// обновляем Nordsieck и пересчитываем 
			// производные
			UpdateNordsiek();
			DynaModel_.SetH(newEffectiveH);
			RescaleNordsiek();
		}
		else
			ReInitializeNordsiek(); // если заваленных шагов слишком много, делаем новый Nordsieck

		// шагаем на следующее время без возможности возврата, так как шаг уже не снизить
		sc.Advance_t0();
		sc.Assign_t0();
	}
	else
	{
		// если шаг еще можно снизить
		sc.nMinimumStepFailures = 0;
		RestoreNordsiek();					// восстанавливаем Nordsieck c предыдущего шага
		DynaModel_.SetH(newEffectiveH);
		RescaleNordsiek();					// масштабируем Nordsieck на новый (половинный см. выше) шаг
		sc.CheckAdvance_t0();
	}
}

bool MixedAdamsBDF::StepConverged()
{
	StepRatio_ = GetRatioForCurrentOrder();
	return StepRatio_ > 1.0;
}

void MixedAdamsBDF::Init() 
{
	auto& Parameters{ DynaModel_.Parameters() };
	if (Parameters.m_eDiffEquationType == DET_ALGEBRAIC)
		Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_NONE;
}

void MixedAdamsBDF::Predict()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };

	//#define DBG_CHECK_PREDICTION

#ifdef DBG_CHECK_PREDICTION
	SnapshotRightVector();
#endif

	if (!DynaModel_.StepControl().m_bNordsiekReset)
	{
		/* 	Алгоритм расчета [Lsode 2.61]
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

		if (DynaModel_.Order() == 2)
		{
			for (pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
			{
				pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;
				pVectorBegin->Nordsiek[1] += pVectorBegin->Nordsiek[2];
				pVectorBegin->Nordsiek[0] += pVectorBegin->Nordsiek[1];
				pVectorBegin->Nordsiek[1] += pVectorBegin->Nordsiek[2];
				pVectorBegin->Error = 0.0;
			}
		}
		else
		{
			for (pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
			{
				pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;
				pVectorBegin->Nordsiek[0] += pVectorBegin->Nordsiek[1];
				pVectorBegin->Error = 0.0;
			}
		}

		for (pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
			*pVectorBegin->pValue = *pVectorBegin->Nordsiek;
	}
	else
	{
		for (pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{
			pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;
			pVectorBegin->Error = 0.0;	// обнуляем ошибку шага
		}
	}

#ifdef DBG_CHECK_PREDICTION
	// после выполнения прогноза рассчитываем разность
	// между спрогнозированным значением и исходным
	CompareRightVector();
#endif

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::ResetIterations);

	// для устройств, которые требует внутренней обработки прогноза
	// (например для узлов, которым нужно перевести прогноз полярного напряжения в прямоугольное)
	// делаем цикл с вызовом функции прогноза устройства
	for (auto&& it : DynaModel_.DeviceContainersPredict())
		it->Predict();
}

double MixedAdamsBDF::GetRatioForCurrentOrder()
{
	double r{ 0.0 };

	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };
		
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);
	sc.Integrator.Reset();

	const double Methodl0[2]{ Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][0],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][0] };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		if (pVectorBegin->Atol > 0)
		{
			// compute again to not asking device via pointer
#ifdef USE_FMA
			double dNewValue = std::fma(pVectorBegin->Error, Methodl0[pVectorBegin->EquationType], pVectorBegin->Nordsiek[0]);
#else
			const double dNewValue{ pVectorBegin->Nordsiek[0] + pVectorBegin->Error * Methodl0[pVectorBegin->EquationType] };
#endif
			const double dError{ pVectorBegin->GetWeightedError(dNewValue) };
			sc.Integrator.Weighted.Update(pVectorBegin, dError);
			struct ConvergenceTest* const pCt{ ConvTest_ + pVectorBegin->EquationType };
			pCt->AddError(dError);
		}
	}

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetRMS);

	const double maxnorm{ 0.2 };

	// гибридная норма
	//ConvTest[DET_ALGEBRAIC].dErrorSum *= (1.0 - maxnorm);
	//ConvTest[DET_ALGEBRAIC].dErrorSum += maxnorm * sc.Integrator.Weighted.dMaxError;
	//ConvTest[DET_DIFFERENTIAL].dErrorSum *= (1.0 - maxnorm);
	//ConvTest[DET_DIFFERENTIAL].dErrorSum += maxnorm * sc.Integrator.Weighted.dMaxError;

	const double DqSame0{ ConvTest_[DET_ALGEBRAIC].dErrorSum / Methodl[sc.q - 1][3] };
	const double DqSame1{ ConvTest_[DET_DIFFERENTIAL].dErrorSum / Methodl[sc.q + 1][3] };
	const double rSame0{ pow(DqSame0, -1.0 / (sc.q + 1)) };
	const double rSame1{ pow(DqSame1, -1.0 / (sc.q + 1)) };

	r = (std::min)(rSame0, rSame1);

	if (Equal(DynaModel_.H() / sc.Hmin, 1.0) && Parameters.m_bDontCheckTolOnMinStep)
		r = (std::max)(1.01, r);

	DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("{} rSame {} RateLimit {} for {} steps",
		sc.Integrator.Weighted.Info(),
		r,
		sc.dRateGrowLimit < (std::numeric_limits<double>::max)() ? sc.dRateGrowLimit : 0.0,
		sc.nStepsToEndRateGrow - sc.nStepsCount));

	// считаем ошибку в уравнении если шаг придется уменьшить
	if (r <= 1.0 && sc.Integrator.Weighted.pVector)
		sc.Integrator.Weighted.pVector->ErrorHits++;

	return r;
}

double MixedAdamsBDF::GetRatioForHigherOrder()
{
	double rUp{ 0.0 };
	_ASSERTE(sc.q == 1);

	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);

	const double Methodl1[2] = { Methodl[DynaModel_.Order() - 1 + DET_ALGEBRAIC * 2][1],  Methodl[DynaModel_.Order() - 1 + DET_DIFFERENTIAL * 2][1] };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		if (pVectorBegin->Atol > 0)
		{
			struct ConvergenceTest* pCt{ ConvTest_ + pVectorBegin->EquationType };
			double dNewValue = *pVectorBegin->pValue;
			// method consts lq can be 1 only
			double dError = pVectorBegin->GetWeightedError(pVectorBegin->Error - pVectorBegin->SavedError, dNewValue) * Methodl1[pVectorBegin->EquationType];
			pCt->AddError(dError);
		}
	}

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetRMS);

	const double DqUp0{ ConvTest_[DET_ALGEBRAIC].dErrorSum / Methodl[1][3] };		// 4.5 gives better result than 3.0, calculated by formulas in Hindmarsh
	const double DqUp1{ ConvTest_[DET_DIFFERENTIAL].dErrorSum / Methodl[3][3] };	// also 4.5 is LTE of BDF-2. 12 is LTE of ADAMS-2, so 4.5 seems correct

	const double rUp0{ pow(DqUp0, -1.0 / (DynaModel_.Order() + 2)) };
	const double rUp1{ pow(DqUp1, -1.0 / (DynaModel_.Order() + 2)) };

	rUp = (std::min)(rUp0, rUp1);

	return rUp;
}

double MixedAdamsBDF::GetRatioForLowerOrder()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	auto& sc{ DynaModel_.StepControl() };

	double rDown{ 0.0 };
	_ASSERTE(sc.q == 2);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		if (pVectorBegin->Atol > 0)
		{
			struct ConvergenceTest* const pCt{ ConvTest_ + pVectorBegin->EquationType };
			const double dNewValue{ *pVectorBegin->pValue };
			// method consts lq can be 1 only
			const double dError{ pVectorBegin->GetWeightedError(pVectorBegin->Nordsiek[2], dNewValue) };
			pCt->AddError(dError);
		}
	}

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetRMS);

	const double DqDown0{ ConvTest_[DET_ALGEBRAIC].dErrorSum };
	const double DqDown1{ ConvTest_[DET_DIFFERENTIAL].dErrorSum };

	const double rDown0{ pow(DqDown0, -1.0 / sc.q) };
	const double rDown1{ pow(DqDown1, -1.0 / sc.q) };

	rDown = (std::min)(rDown0, rDown1);
	return rDown;
}


void MixedAdamsBDF::UpdateNordsiek(bool bAllowSuppression)
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };

	const double alpha{ DynaModel_.H() / DynaModel_.UsedH() > 0.0 ? DynaModel_.UsedH() : 1.0 };
	const double alphasq{ alpha * alpha };
	const double alpha1{ (1.0 + alpha) };
	double alpha2{ 1.0 + 2.0 * alpha };
	bool bSuprressRinging{ false };

	// режим подавления рингинга активируем если порядок метода 2
	// шаг превышает 0.01 и UpdateNordsieck вызван для перехода к следующему
	// шагу после успешного завершения предыдущего
	if (sc.q == 2 && bAllowSuppression && DynaModel_.H() > 0.01 && DynaModel_.UsedH() > 0.0)
	{
		switch (Parameters.m_eAdamsRingingSuppressionMode)
		{
		case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL:
			// в глобальном режиме подавления разрешаем подавления на каждом кратном m_nAdamsGlobalSuppressionStep шаге
			if (sc.nStepsCount % Parameters.m_nAdamsGlobalSuppressionStep == 0 && sc.nStepsCount > Parameters.m_nAdamsGlobalSuppressionStep)
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
				switch (Parameters.m_eAdamsRingingSuppressionMode)
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

	DynaModel_.StoreUsedH();
	sc.m_bNordsiekSaved = true;
	sc.SetNordsiekScaledForHSaved(sc.NordsiekScaledForH());
	// после того как Нордсик обновлен,
	// сбрасываем флаг ресета, начинаем работу предиктора
	// и контроль соответствия предиктора корректору
	sc.m_bNordsiekReset = false;

	for (auto&& it : DynaModel_.DeviceContainersStoreStates())
		for (auto&& dit : *it)
			dit->StoreStates();

	if (Parameters.m_eAdamsRingingSuppressionMode == ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA)
	{
		if (sc.bRingingDetected)
			EnableAdamsCoefficientDamping(true);
		else if (sc.nNoRingingSteps > Parameters.m_nAdamsDampingSteps)
			EnableAdamsCoefficientDamping(false);
	}


	// даем информацию для обработки разрывов о том, что данный момент
	// времени пройден
	DynaModel_.PassTime();
	sc.ResetStepsToFail();
}


void MixedAdamsBDF::ChangeOrder(ptrdiff_t newOrder)
{
	auto& sc{ DynaModel_.StepControl() };
	if (DynaModel_.Order() != newOrder)
	{
		sc.RefactorMatrix();
		// если меняется порядок - то нужно обновлять элементы матрицы считавшиеся постоянными, так как 
		// изменяются коэффициенты метода.
		sc.UpdateConstElements();
	}

	sc.q = newOrder;

	switch (sc.q)
	{
	case 1:
		break;
	case 2:
		break;
	}
}

void MixedAdamsBDF::ConstructNordsiekOrder()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		pVectorBegin->Nordsiek[2] = pVectorBegin->Error / 2.0;
}

// масштабирование Nordsieck на заданный коэффициент изменения шага
void MixedAdamsBDF::RescaleNordsiek()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };

	// расчет выполняется путем умножения текущего Nordsieck на диагональную матрицу C[q+1;q+1]
	// с элементами C[i,i] = r^(i-1) [Lsode 2.64]

	_ASSERTE(sc.NordsiekScaledForH() > 0.0);
	const double r{ DynaModel_.H() / sc.NordsiekScaledForH() };
	if (r == 1.0)
		return;

	DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Nordsiek rescale {}->{}",
		sc.NordsiekScaledForH(),
		DynaModel_.H()));

	const double crs[4] = { 1.0, r , (sc.q == 2) ? r * r : 1.0 , 1.0 };
#ifdef _AVX2
	const __m256d rs = _mm256_load_pd(crs);
#endif
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
#ifdef _AVX2
		__m256d nord = _mm256_load_pd(pVectorBegin->Nordsiek);
		nord = _mm256_mul_pd(nord, rs);
		_mm256_store_pd(pVectorBegin->Nordsiek, nord);
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
	sc.SetNordsiekScaledForH(DynaModel_.H());

	// рассчитываем коэффициент изменения шага
	const double dRefactorRatio{ DynaModel_.H() / sc.m_dLastRefactorH };
	// если шаг изменился более в заданное количество раз - взводим флаг рефакторизации Якоби
	// sc.m_dLastRefactorH обновляется после рефакторизации
	if (dRefactorRatio > Parameters.m_dRefactorByHRatio || 1.0 / dRefactorRatio > Parameters.m_dRefactorByHRatio)
		sc.RefactorMatrix();
}

void MixedAdamsBDF::EnableAdamsCoefficientDamping(bool bEnable)
{
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };
	if (bEnable == sc.bAdamsDampingEnabled) return;

	sc.bAdamsDampingEnabled = bEnable;
	double Alpha = bEnable ? Parameters.m_dAdamsDampingAlpha : 0.0;
	Methodl[3][0] = MixedAdamsBDF::MethodlDefault[3][0] * (1.0 + Alpha);
	// Вместо MethodDefault[3][3] можно использовать честную формулу для LTE (см. Docs)
	Methodl[3][3] = 1.0 / std::abs(-1.0 / MixedAdamsBDF::MethodlDefault[3][3] - 0.5 * Alpha) / (1.0 + Alpha);
	// требуется обновление матрицы и постоянных коэффициентов 
	// из-за замены коэффициентов метода
	sc.RefactorMatrix();
	// Для подавления рингинга требуется обновление элементов так как в алгебраические
	// уравнения могут входит дифференциальные переменные с коэффициентами Адамса
	sc.UpdateConstElements();
	Computehl0();
	DynaModel_.Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(DFW2::CDFW2Messages::m_cszAdamsDamping,
		bEnable ? DFW2::CDFW2Messages::m_cszOn : DFW2::CDFW2Messages::m_cszOff));
}

void MixedAdamsBDF::Computehl0()
{
	// кэшированное значение l0 * GetH для элементов матрицы
	// пересчитывается при изменении шага и при изменении коэффициентов метода
	// для демпфирования
	// lh[i] = l[i][0] * GetH()
	for (auto&& lh : Methodlh)
		lh = Methodl[&lh - Methodlh][0] * DynaModel_.H();
}


// построение Nordsieck после того, как обнаружено что текущая история
// ненадежна
void MixedAdamsBDF::ReInitializeNordsiek()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };

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
		sc.SetNordsiekScaledForH(sc.NordsiekScaledForHSaved());
		RescaleNordsiek();
		DynaModel_.BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
	sc.ResetStepsToFail();
}

// восстанавление Nordsieck с предыдущего шага
void MixedAdamsBDF::RestoreNordsiek()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };

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
			* pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			pVectorBegin->Error = pVectorBegin->SavedError;
		}
		sc.SetNordsiekScaledForH(sc.NordsiekScaledForHSaved());
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
		sc.SetNordsiekScaledForH(0.0);
		sc.SetNordsiekScaledForHSaved(0.0);
	}

	for (auto&& it : DynaModel_.DeviceContainersStoreStates())
		for (auto&& dit : *it)
			dit->RestoreStates();
}

void MixedAdamsBDF::NewtonBacktrack(const double* pVec, double lambda)
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	// константы метода выделяем в локальный массив, определяя порядок метода для всех переменных один раз
	const double Methodl0[2]{ Methodl[DynaModel_.Order() - 1 + DET_ALGEBRAIC * 2][0],  Methodl[DynaModel_.Order() - 1 + DET_DIFFERENTIAL * 2][0] };
	const double* pB{ pVec };
	lambda -= 1.0;
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++)
	{
		pVectorBegin->Error += *pB * lambda;
		*pVectorBegin->pValue = pVectorBegin->Nordsiek[0] + Methodl0[pVectorBegin->EquationType] * pVectorBegin->Error;
	}
}

void MixedAdamsBDF::NewtonUpdateIteration()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector(0) };
	RightVector* pVectorBegin{ pRightVector };
	const auto& Parameters{ DynaModel_.Parameters() };
	auto& sc{ DynaModel_.StepControl() };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };

	// original Hindmarsh (2.99) suggests ConvCheck = 0.5 / (sc.q + 2), but i'm using tolerance 5 times lower
	const double ConvCheck{ 0.1 / (DynaModel_.Order() + 2.0) };

	// first check Newton convergence
	sc.Newton.Reset();

	// константы метода выделяем в локальный массив, определяя порядок метода для всех переменных один раз
	const double Methodl0[2]{ Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][0],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][0] };

	const double* pB{ DynaModel_.B() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++)
	{
		pVectorBegin->b = *pB;
		pVectorBegin->Error += pVectorBegin->b;
		sc.Newton.Absolute.Update(pVectorBegin, std::abs(pVectorBegin->b));

#ifdef USE_FMA
		const double dNewValue{ std::fma(Methodl0[pVectorBegin->EquationType], pVectorBegin->Error, pVectorBegin->Nordsiek[0]) };
#else
		const double dNewValue{ pVectorBegin->Nordsiek[0] + pVectorBegin->Error * Methodl0[pVectorBegin->EquationType] };
#endif
		const double dOldValue{ *pVectorBegin->pValue };
		*pVectorBegin->pValue = dNewValue;
		if (pVectorBegin->Atol > 0)
		{
			const double dError{ pVectorBegin->GetWeightedError(pVectorBegin->b, dOldValue) };
			sc.Newton.Weighted.Update(pVectorBegin, dError);
			_CheckNumber(dError);
			ConvergenceTest* pCt{ ConvTest_ + pVectorBegin->EquationType };
#ifdef _DEBUG
			// breakpoint place for nans
			_ASSERTE(!std::isnan(dError));
#endif
			pCt->AddError(dError);
		}
	}

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetConvergenceRatio);

	bool bConvCheckConverged{ ConvTest_[DET_ALGEBRAIC].dErrorSums < Methodl[sc.q - 1][3] * ConvCheck &&
							  ConvTest_[DET_DIFFERENTIAL].dErrorSums < Methodl[sc.q + 1][3] * ConvCheck &&
							  sc.Newton.Weighted.dMaxError < Parameters.m_dNewtonMaxNorm };

	if (bConvCheckConverged)
		sc.m_bNewtonConverged = true;
	else
	{
		if (ConvTest_[DET_ALGEBRAIC].dCms > 1.0 || ConvTest_[DET_DIFFERENTIAL].dCms > 1.0)
			sc.m_bNewtonDisconverging = true;
		else
			sc.RefactorMatrix(false);
	}
}


const double MixedAdamsBDF::MethodlDefault[4][4] =
	//									   l0			l1			l2			Tauq
										{ { 1.0,		1.0,		0.0,		2.0  },				//  BDF-1
										  { 2.0 / 3.0,	1.0,		1.0 / 3.0,  4.5 },				//  BDF-2
										  { 1.0,		1.0,		0.0,		2.0  },				//  ADAMS-1
										  { 0.5,		1.0,		0.5,		12.0 } };			//  ADAMS-2
