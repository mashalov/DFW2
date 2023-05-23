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
	auto& sc{ DynaModel_.StepControl() };
	Predict();
	DynaModel_.SolveNewton(sc.m_bDiscontinuityMode ? 20 : 10);	// делаем корректор
	if(sc.m_bNewtonConverged && !sc.m_bDiscontinuityMode)
		DetectAdamsRinging();
}


void MixedAdamsBDF::AcceptStep(bool DisableStepControl)
{
	auto& sc{ DynaModel_.StepControl() };

	sc.m_bRetryStep = false;		// отказываемся от повтора шага, все хорошо
	sc.RefactorMatrix(false);		// отказываемся от рефакторизации Якоби
	sc.nSuccessfullStepsOfNewton++;
	sc.nMinimumStepFailures = 0;
	// рассчитываем количество успешных шагов и пройденного времени для каждого порядка
	sc.OrderStatistics[DynaModel_.Order() - 1].nSteps++;
	// переходим к новому рассчитанному времени с обновлением суммы Кэхэна
	sc.Advance_t0();

	double StepRatio{ StepRatio_ };

	if (StepRatio > 1.2 && !DisableStepControl)
	{
		// если шаг можно хорошо увеличить
		StepRatio /= 1.2;

		switch (DynaModel_.Order())
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
					DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG,
						fmt::format(CDFW2Messages::m_cszStepAndOrderChanged,
							DynaModel_.Order(),
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
					ChangeOrder(1);
					DynaModel_.SetH(DynaModel_.H() * sc.dFilteredOrder);
					DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG,
						fmt::format(CDFW2Messages::m_cszStepAndOrderChanged,
							DynaModel_.Order(),
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
			_ASSERTE(Equal(DynaModel_.H(), DynaModel_.UsedH()));
			// запоминаем коэффициент увеличения только для репорта
			// потому что sc.dFilteredStep изменится в последующем 
			// RescaleNordsiek
			const double k{ sc.dFilteredStep };
			// рассчитываем новый шаг
			// пересчитываем Nordsieck на новый шаг
			DynaModel_.SetH(DynaModel_.H() * sc.dFilteredStep);
			DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszStepChanged,
				DynaModel_.H(),
				k,
				DynaModel_.Order()));
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
	RescaleNordsiek();
}

void MixedAdamsBDF::RejectStep()
{
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
	sc.OrderStatistics[DynaModel_.Order()].nFailures++;


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
	/*if (Parameters.m_eDiffEquationType == DET_ALGEBRAIC)
		Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_NONE;
	*/
}

void MixedAdamsBDF::Predict()
{
	//#define DBG_CHECK_PREDICTION

#ifdef DBG_CHECK_PREDICTION
	SnapshotRightVector();
#endif

	if (!DynaModel_.StepFromReset())
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
			for (auto&& r : DynaModel_.RightVectorRange())
			{
				r.Nordsiek[0] = *r.pValue;
				r.Nordsiek[1] += r.Nordsiek[2];
				r.Nordsiek[0] += r.Nordsiek[1];
				r.Nordsiek[1] += r.Nordsiek[2];
				r.Error = 0.0;
			}
		}
		else
		{
			for (auto&& r : DynaModel_.RightVectorRange())
			{
				r.Nordsiek[0] = *r.pValue;
				r.Nordsiek[0] += r.Nordsiek[1];
				r.Error = 0.0;
			}
		}

		for (auto&& r : DynaModel_.RightVectorRange())
			*r.pValue = *r.Nordsiek;
	}
	else
	{
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			r.Nordsiek[0] = *r.pValue;
			r.Error = 0.0;   // обнуляем ошибку шага
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
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };
		
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);
	sc.Integrator.Reset();

	const double Methodl0[2]{ Methodl[DynaModel_.Order() - 1 + DET_ALGEBRAIC * 2][0],  Methodl[DynaModel_.Order() - 1 + DET_DIFFERENTIAL * 2][0] };

	for (auto&& r : DynaModel_.RightVectorRange())
	{
		if (r.Atol > 0)
		{
			// compute again to not asking device via pointer
#ifdef USE_FMA
			const double dNewValue{ std::fma(r.Error, Methodl0[r.EquationType], r.Nordsiek[0]) };
#else
			const double dNewValue{ r.Nordsiek[0] + r.Error * Methodl0[r.EquationType] };
#endif
			const double dError{ r.GetWeightedError(dNewValue) };
			sc.Integrator.Weighted.Update(&r, dError);
			struct ConvergenceTest* const pCt{ ConvTest_ + r.EquationType };
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

	const double DqSame0{ ConvTest_[DET_ALGEBRAIC].dErrorSum / Methodl[DynaModel_.Order() - 1][3] };
	const double DqSame1{ ConvTest_[DET_DIFFERENTIAL].dErrorSum / Methodl[DynaModel_.Order() + 1][3] };
	const double rSame0{ pow(DqSame0, -1.0 / (DynaModel_.Order() + 1)) };
	const double rSame1{ pow(DqSame1, -1.0 / (DynaModel_.Order() + 1)) };

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
	_ASSERTE(DynaModel_.Order() == 1);

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);

	const double Methodl1[2] = { Methodl[DynaModel_.Order() - 1 + DET_ALGEBRAIC * 2][1],  Methodl[DynaModel_.Order() - 1 + DET_DIFFERENTIAL * 2][1] };

	for (auto&& r : DynaModel_.RightVectorRange())
	{
		if (r.Atol > 0)
		{
			struct ConvergenceTest* pCt{ ConvTest_ + r.EquationType };
			double dNewValue{ *r.pValue };
			// method consts lq can be 1 only
			const double dError{ r.GetWeightedError(r.Error - r.SavedError, dNewValue) * Methodl1[r.EquationType] };
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
	auto& sc{ DynaModel_.StepControl() };

	double rDown{ 0.0 };
	_ASSERTE(DynaModel_.Order() == 2);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);

	for (auto&& r : DynaModel_.RightVectorRange())
	{
		if (r.Atol > 0)
		{
			struct ConvergenceTest* const pCt{ ConvTest_ + r.EquationType };
			const double dNewValue{ *r.pValue };
			// method consts lq can be 1 only
			const double dError{ r.GetWeightedError(r.Nordsiek[2], dNewValue) };
			pCt->AddError(dError);
		}
	}

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetRMS);

	const double DqDown0{ ConvTest_[DET_ALGEBRAIC].dErrorSum };
	const double DqDown1{ ConvTest_[DET_DIFFERENTIAL].dErrorSum };

	const double rDown0{ pow(DqDown0, -1.0 / DynaModel_.Order()) };
	const double rDown1{ pow(DqDown1, -1.0 / DynaModel_.Order()) };

	rDown = (std::min)(rDown0, rDown1);
	return rDown;
}


void MixedAdamsBDF::UpdateNordsiek(bool bAllowSuppression)
{
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
	if (DynaModel_.Order() == 2 && bAllowSuppression && DynaModel_.H() > 0.01 && DynaModel_.UsedH() > 0.0)
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
	std::copy(&Methodl[DynaModel_.Order() - 1][0], &Methodl[DynaModel_.Order() - 1][3], &LocalMethodl[0][0]);
	std::copy(&Methodl[DynaModel_.Order() + 1][0], &Methodl[DynaModel_.Order() + 1][3], &LocalMethodl[1][0]);

	for (auto&& r: DynaModel_.RightVectorRange())
	{
		// выбираем коэффициент метода по типу уравнения EquationType
		const double& Error{ r.Error };
		const double* lm = LocalMethodl[r.EquationType];
#ifdef _AVX2
		const __m256d err = _mm256_set1_pd(pVectorBegin->Error);
		__m256d lmms = _mm256_load_pd(lm);
		__m256d nord = _mm256_load_pd(pVectorBegin->Nordsiek);
		nord = _mm256_fmadd_pd(err, lmms, nord);
		_mm256_store_pd(pVectorBegin->Nordsiek, nord);
#else
		r.Nordsiek[0] += Error * *lm;	lm++;
		r.Nordsiek[1] += Error * *lm;	lm++;
		r.Nordsiek[2] += Error * *lm;
#endif
	}

	for (auto&& r : DynaModel_.RightVectorRange())
	{
		r.SavedError = r.Error;
		// подавление рингинга
		if (bSuprressRinging)
		{
			if (r.EquationType == DET_DIFFERENTIAL)
			{
				// рингинг подавляем только для дифуров (если дифуры решаются BDF надо сбрасывать подавление в ARSM_NONE)
				switch (Parameters.m_eAdamsRingingSuppressionMode)
				{
				case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
					if (r.RingsSuppress > 0)
					{
						// для переменных, у которых количество шагов для замены Адамса на BDF не исчерпано
						// делаем эту замену и уменьшаем счетчик
						r.Nordsiek[1] = (alphasq * r.Tminus2Value - alpha1 * alpha1 * r.SavedNordsiek[0] + alpha2 * r.Nordsiek[0]) / alpha1;
						r.RingsSuppress--;
						r.RingsCount = 0;
					}
					break;
				case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL:
					// в глобальном режиме просто заменяем производную Адамса на производную BDF
					r.Nordsiek[1] = (alphasq * r.Tminus2Value - alpha1 * alpha1 * r.SavedNordsiek[0] + alpha2 * r.Nordsiek[0]) / alpha1;
					break;
				}
			}
		}

		// сохраняем пред-предыдущее значение переменной состояния
		r.Tminus2Value = r.SavedNordsiek[0];

		// и сохраняем текущее значение переменных состояния перед новым шагом
#ifdef _AVX2
		_mm256_store_pd(pVectorBegin->SavedNordsiek, _mm256_load_pd(pVectorBegin->Nordsiek));
#else
		r.SavedNordsiek[0] = r.Nordsiek[0];
		r.SavedNordsiek[1] = r.Nordsiek[1];
		r.SavedNordsiek[2] = r.Nordsiek[2];
#endif
	}

	DynaModel_.StoreUsedH();
	sc.m_bNordsiekSaved = true;
	sc.SetNordsiekScaledForHSaved(sc.NordsiekScaledForH());
	// после того как Нордсик обновлен,
	// сбрасываем флаг ресета, начинаем работу предиктора
	// и контроль соответствия предиктора корректору
	DynaModel_.ResetStep(false);

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

	switch (DynaModel_.Order())
	{
	case 1:
		break;
	case 2:
		break;
	}
}

void MixedAdamsBDF::ConstructNordsiekOrder()
{
	for (auto&& r :  DynaModel_.RightVectorRange() )
		r.Nordsiek[2] = r.Error / 2.0;
}

// масштабирование Nordsieck на заданный коэффициент изменения шага
void MixedAdamsBDF::RescaleNordsiek()
{
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };

	// расчет выполняется путем умножения текущего Nordsieck на диагональную матрицу C[q+1;q+1]
	// с элементами C[i,i] = r^(i-1) [Lsode 2.64]

	if (sc.NordsiekScaledForH() <= 0.0)
		return;
	const double r{ DynaModel_.H() / sc.NordsiekScaledForH() };
	if (r == 1.0)
		return;

	DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Nordsiek rescale {}->{}",
		sc.NordsiekScaledForH(),
		DynaModel_.H()));

	const double crs[4] = { 1.0, r , (DynaModel_.Order() == 2) ? r * r : 1.0 , 1.0 };
#ifdef _AVX2
	const __m256d rs = _mm256_load_pd(crs);
#endif
	for (auto&& r:  DynaModel_.RightVectorRange())
	{
#ifdef _AVX2
		__m256d nord = _mm256_load_pd(pVectorBegin->Nordsiek);
		nord = _mm256_mul_pd(nord, rs);
		_mm256_store_pd(pVectorBegin->Nordsiek, nord);
#else
		for (ptrdiff_t j = 1; j < DynaModel_.Order() + 1; j++)
			r.Nordsiek[j] *= crs[j];
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
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };

	if (sc.m_bNordsiekSaved)
	{
		// если есть сохраненный шаг
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			// значение восстанавливаем
			r.Nordsiek[0] = r.SavedNordsiek[0];
			*r.pValue = r.Nordsiek[0];
			// а элемент первого порядка конструируем по разности между предыдущим и пред-предыдущим значениями
			// т.к. первый элемент это hy', а y' = (y(-1) - y(-2)) / h. Note - мы берем производную с пред-предыдущего шага

			//											y(-1)						y(-2)
			r.Nordsiek[1] = r.SavedNordsiek[0] - r.Tminus2Value;
			r.SavedError = 0.0;
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
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };

	if (sc.m_bNordsiekSaved)
	{
		// если есть данные для восстановления - просто копируем предыдущий шаг
		// в текущий
		for (auto&& r : DynaModel_.RightVectorRange())
		{

#ifdef _AVX2
			_mm256_store_pd(pVectorBegin->Nordsiek, _mm256_load_pd(pVectorBegin->SavedNordsiek));
#else
			double* pN{ r.Nordsiek };
			double* pS{ r.SavedNordsiek };
			*pN = *pS; pS++; pN++;
			*pN = *pS; pS++; pN++;
			*pN = *pS; pS++; pN++;
#endif
			*r.pValue = r.Nordsiek[0];
			r.Error = r.SavedError;
		}
		sc.SetNordsiekScaledForH(sc.NordsiekScaledForHSaved());
	}
	else
	{
		// если данных для восстановления нет
		// считаем что прозводные нулевые
		// это слабая надежда, но лучше чем ничего
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			*r.pValue = r.Nordsiek[0];
			r.Nordsiek[1] = r.Nordsiek[2] = 0.0;
			r.Error = 0.0;
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
	// константы метода выделяем в локальный массив, определяя порядок метода для всех переменных один раз
	const double Methodl0[2]{ Methodl[DynaModel_.Order() - 1 + DET_ALGEBRAIC * 2][0],  Methodl[DynaModel_.Order() - 1 + DET_DIFFERENTIAL * 2][0] };
	const double* pB{ pVec };
	lambda -= 1.0;
	for (auto&& r: DynaModel_.RightVectorRange())
	{
		r.Error += *pB * lambda;
		*r.pValue = r.Nordsiek[0] + Methodl0[r.EquationType] * r.Error;
		++pB;
	}
}

void MixedAdamsBDF::NewtonUpdateIteration()
{
	const auto& Parameters{ DynaModel_.Parameters() };
	auto& sc{ DynaModel_.StepControl() };

	// original Hindmarsh (2.99) suggests ConvCheck = 0.5 / (sc.q + 2), but i'm using tolerance 5 times lower
	const double ConvCheck{ 0.1 / (DynaModel_.Order() + 2.0) };

	// first check Newton convergence
	sc.Newton.Reset();

	// константы метода выделяем в локальный массив, определяя порядок метода для всех переменных один раз
	const double Methodl0[2]{ Methodl[DynaModel_.Order() - 1 + DET_ALGEBRAIC * 2][0],  Methodl[DynaModel_.Order() - 1 + DET_DIFFERENTIAL * 2][0] };

	const double* pB{ DynaModel_.B() };

	for (auto&& r : DynaModel_.RightVectorRange())
	{
		r.b = *pB;
		r.Error += r.b;
		sc.Newton.Absolute.Update(&r, std::abs(r.b));

#ifdef USE_FMA
		const double dNewValue{ std::fma(Methodl0[r.EquationType], r.Error, r.Nordsiek[0]) };
#else
		const double dNewValue{ r.Nordsiek[0] + r.Error * Methodl0[r.EquationType] };
#endif
		const double dOldValue{ *r.pValue };
		*r.pValue = dNewValue;
		if (r.Atol > 0)
		{
			const double dError{ r.GetWeightedError(r.b, dOldValue) };
			sc.Newton.Weighted.Update(&r, dError);
			_CheckNumber(dError);
			ConvergenceTest* pCt{ ConvTest_ + r.EquationType };
#ifdef _DEBUG
			// breakpoint place for nans
			_ASSERTE(!std::isnan(dError));
#endif
			pCt->AddError(dError);
		}

		++pB;
	}

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetConvergenceRatio);

	bool bConvCheckConverged{ ConvTest_[DET_ALGEBRAIC].dErrorSums < Methodl[DynaModel_.Order() - 1][3] * ConvCheck &&
							  ConvTest_[DET_DIFFERENTIAL].dErrorSums < Methodl[DynaModel_.Order() + 1][3] * ConvCheck &&
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


void MixedAdamsBDF::DetectAdamsRinging()
{
	const auto& Parameters{ DynaModel_.Parameters() };
	auto& sc{ DynaModel_.StepControl() };
	sc.bRingingDetected = false;

	if ((Parameters.m_eAdamsRingingSuppressionMode == ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA ||
		Parameters.m_eAdamsRingingSuppressionMode == ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL) &&
		DynaModel_.Order() == 2 && DynaModel_.H() > 0.01 && DynaModel_.UsedH() > 0.0)
	{
		const double Methodl1[2]{ Methodl[DynaModel_.Order() - 1 + DET_ALGEBRAIC * 2][1],  Methodl[DynaModel_.Order() - 1 + DET_DIFFERENTIAL * 2][1] };

		for(auto&& r : DynaModel_.RightVectorRange())
		{
#ifdef USE_FMA
			const double newValue{ std::fma(r.Error, Methodl1[r.EquationType], r.Nordsiek[1]) };
#else 
			const double newValue{ r.Error * Methodl1[r.EquationType] + r.Nordsiek[1] };
#endif
			// если знак производной изменился - увеличиваем счетчик циклов
			if (std::signbit(newValue) != std::signbit(r.SavedNordsiek[1]) && std::abs(newValue) > r.Atol * DynaModel_.H() * 5.0)
				r.RingsCount++;
			else
				r.RingsCount = 0;

			// если счетчик циклов изменения знака достиг порога
			if (r.RingsCount > Parameters.m_nAdamsIndividualSuppressionCycles)
			{
				r.RingsCount = 0;
				sc.bRingingDetected = true;
				switch (Parameters.m_eAdamsRingingSuppressionMode)
				{
				case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
					if (r.EquationType == DET_DIFFERENTIAL)
					{
						// в RightVector устанавливаем количество шагов, на протяжении которых производная Адамса будет заменяться 
						// на производную  BDF
						r.RingsSuppress = Parameters.m_nAdamsIndividualSuppressStepsRange;
					}
					break;
				case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA:
					r.RingsSuppress = Parameters.m_nAdamsIndividualSuppressStepsRange;
					break;
				}

				if (r.RingsSuppress)
				{
					DynaModel_.Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Ringing {} {} last values {} {} {}",
									r.pDevice->GetVerbalName(),
									r.pDevice->VariableNameByPtr(r.pValue),
									newValue,
									r.SavedNordsiek[0],
									r.RingsSuppress));
				}
			}
		}

		if (sc.bRingingDetected)
			sc.nNoRingingSteps = 0;
		else
			sc.nNoRingingSteps++;

	}
}

void  MixedAdamsBDF::BOperator()
{
	auto BRange{ DynaModel_.BRange() };
	for (const auto& r : DynaModel_.RightVectorRange())
	{
		if (r.PhysicalEquationType == DET_ALGEBRAIC)
			*BRange = -*BRange;
		else
		{

//			*BRange = *BRange * DynaModel_.H() - r.Nordsiek[1] - r.Error;
			*BRange = std::fma(DynaModel_.H(), *BRange, -r.Nordsiek[1] - r.Error);
		}
		++BRange;
	}
}

void MixedAdamsBDF::WOperator(ptrdiff_t Row, ptrdiff_t Col, double& Value)
{
	const RightVector* const pRightVector{ DynaModel_.GetRightVector()};
	const ptrdiff_t nMethodIndx{ static_cast<ptrdiff_t>((pRightVector + Col)->EquationType) * 2 + (DynaModel_.Order() - 1)};
	// в качестве типа уравнения используем __физический__ тип
	// потому что у алгебраических и дифференциальных уравнений
	// разная структура в матрице Якоби, а EquationType указывает лишь набор коэффициентов метода

	if ((pRightVector + Row)->PhysicalEquationType == DET_ALGEBRAIC)
		Value *= Methodl[nMethodIndx][0];		// если уравнение алгебраическое, ставим коэффициент метода интегрирования
	else
	{
		// если уравнение дифференциальное, ставим коэффициент метода умноженный на шаг
		Value *= Methodlh[nMethodIndx];
		// если элемент диагональный, учитываем диагональную единичную матрицу
		if (Row == Col)
			Value = 1.0 - Value;
	}
}

void MixedAdamsBDF::Restart()
{
	auto& sc{ DynaModel_.StepControl() };
	if (sc.m_bNordsiekSaved)
	{
		for (auto&& r :  DynaModel_.RightVectorRange())
		{
			// в качестве значения принимаем то, что рассчитано в устройстве
			r.Tminus2Value = r.Nordsiek[0] = r.SavedNordsiek[0] = *r.pValue;
			// первая производная равна нулю (вторая тоже, но после Reset мы ее не используем, т.к. работаем на первом порядке
			r.Nordsiek[1] = r.SavedNordsiek[1] = 0.0;
			r.SavedError = 0.0;
		}
		// запрашиваем расчет производных дифференциальных уравнений у устройств, которые это могут
		DynaModel_.BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
	// ставим флаг ресета Нордсика, чтобы не
	// контролировать соответствие предиктора-корректору
	// на стартап-шаге
	DynaModel_.ResetStep(true);
	sc.SetNordsiekScaledForH(DynaModel_.H());
	sc.SetNordsiekScaledForHSaved(DynaModel_.H());
}


const double MixedAdamsBDF::MethodlDefault[4][4] =
	//									   l0			l1			l2			Tauq
										{ { 1.0,		1.0,		0.0,		2.0  },				//  BDF-1
										  { 2.0 / 3.0,	1.0,		1.0 / 3.0,  4.5 },				//  BDF-2
										  { 1.0,		1.0,		0.0,		2.0  },				//  ADAMS-1
										  { 0.5,		1.0,		0.5,		12.0 } };			//  ADAMS-2
