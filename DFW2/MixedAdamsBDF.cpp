#include "stdafx.h"
#include "stdafx.h"
#include "DynaModel.h"
#include "MixedAdamsBDF.h"

using namespace DFW2;
MixedAdamsBDF::MixedAdamsBDF(CDynaModel& DynaModel) : IntegratorBase(DynaModel)
{
	std::copy(&MethodlDefault[0][0], &MethodlDefault[0][0] + sizeof(MethodlDefault) / sizeof(MethodlDefault[0][0]), &Methodl[0][0]);
}

void MixedAdamsBDF::SaveNordsiek()
{
	auto& sc{ DynaModel_.StepControl() };

	for (auto&& r : DynaModel_.RightVectorRange())
	{
		double* pN{ r.Nordsiek };
		double* pS{ r.SavedNordsiek };
#ifdef _AVX2
		_mm256_store_pd(pVectorBegin->Nordsiek, _mm256_load_pd(pVectorBegin->SavedNordsiek));
#else
		// сохраняем пред-предыдущее значение переменной состояния
		r.Tminus2Value = *pS;

		// запоминаем три элемента, не смотря на текущий порядок
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;
#endif
		r.SavedError = r.Error;
	}

	DynaModel_.StoreUsedH();
	sc.SetNordsiekScaledForHSaved(sc.NordsiekScaledForH());
	sc.m_bNordsiekSaved = true;
	DynaModel_.StoreStates();
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

	InitNordsiek();

	for (auto&& r : DynaModel_.RightVectorRange())
	{
		r.Nordsiek[0] = r.SavedNordsiek[0] = *r.pValue;
		PrepareNordsiekElement(&r);
	}
	
	DynaModel_.SetH(0.0);
	auto& sc{ DynaModel_.StepControl() };
	sc.m_bDiscontinuityMode = true;
	DynaModel_.SolveNewton(100);
	sc.m_bDiscontinuityMode = false;
		
	sc.SetNordsiekScaledForH(1.0);
	SaveNordsiek();
}


void MixedAdamsBDF::InitNordsiek()
{
	DynaModel_.InitDevicesNordsiek();

	for (auto&& r : DynaModel_.RightVectorRange())
		InitNordsiekElement(&r, DynaModel_.Atol(), DynaModel_.Rtol());

	auto& sc{ DynaModel_.StepControl() };
	sc.StepChanged();
	sc.OrderChanged();
	sc.m_bNordsiekSaved = false;
}

void MixedAdamsBDF::InitNordsiekElement(struct RightVector* pVectorBegin, double Atol, double Rtol)
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

void MixedAdamsBDF::PrepareNordsiekElement(struct RightVector* pVectorBegin)
{
	pVectorBegin->Error = 0.0;
	pVectorBegin->PhysicalEquationType = DET_ALGEBRAIC;
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
	}

	SaveNordsiek();

	// после того как Нордсик обновлен,
	// сбрасываем флаг ресета, начинаем работу предиктора
	// и контроль соответствия предиктора корректору
	DynaModel_.ResetStep(false);

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
	if (r > 0.0)
	{
		sc.SetNordsiekScaledForH(DynaModel_.H());
		sc.StepChanged();
		sc.OrderChanged();

		// рассчитываем коэффициент изменения шага
		const double dRefactorRatio{ DynaModel_.H() / sc.m_dLastRefactorH };
		// если шаг изменился более в заданное количество раз - взводим флаг рефакторизации Якоби
		// sc.m_dLastRefactorH обновляется после рефакторизации
		if (dRefactorRatio > Parameters.m_dRefactorByHRatio || 1.0 / dRefactorRatio > Parameters.m_dRefactorByHRatio)
			sc.RefactorMatrix();
	}
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


// построение Nordsieck после того, как обнаружено что текущая история ненадежна
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
		sc.SetNordsiekScaledForH(sc.NordsiekScaledForHSaved());
	}

	DynaModel_.RestoreStates();
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

void MixedAdamsBDF::NewtonFailed()
{
	const auto& Parameters{ DynaModel_.Parameters() };
	auto& sc{ DynaModel_.StepControl() };

	const double h{ DynaModel_.H() };
	const double usedh{ DynaModel_.H() };

	// обновляем подсчет ошибок Ньютона
	if (!sc.m_bDiscontinuityMode)
	{
		// если вне разрыва - считаем для порядка шага
		sc.OrderStatistics[sc.q - 1].nNewtonFailures++;
	}
	else // если в разрыве - считаем количество завалов на разрывах
		sc.nDiscontinuityNewtonFailures++;

	if (sc.nSuccessfullStepsOfNewton > 10)
	{
		if (usedh / h >= 0.8)
		{
			DynaModel_.SetH(h * 0.87);
			sc.SetRateGrowLimit(1.0);
		}
		else
		{
			DynaModel_.SetH(0.8 * usedh + 0.2 * h);
			sc.SetRateGrowLimit(1.18);
		}
	}
	else
		if (sc.nSuccessfullStepsOfNewton >= 1)
		{
			DynaModel_.SetH(h * 0.87);
			sc.SetRateGrowLimit(1.0);
		}
		else
			if (sc.nSuccessfullStepsOfNewton == 0)
			{
				DynaModel_.SetH(h * 0.25);
				sc.SetRateGrowLimit(10.0);
			}

	sc.nSuccessfullStepsOfNewton = 0;

	ChangeOrder(1);

	if (DynaModel_.H() < DynaModel_.Hmin())
	{
		DynaModel_.SetH(DynaModel_.Hmin());
		if (++sc.nMinimumStepFailures > Parameters.m_nMinimumStepFailures)
			throw dfw2error(fmt::format(CDFW2Messages::m_cszFailureAtMinimalStep, 
				DynaModel_.GetCurrentTime(), 
				DynaModel_.GetIntegrationStepNumber(), 
				DynaModel_.Order(), 
				DynaModel_.H()));
		sc.Advance_t0();
		sc.Assign_t0();
	}
	else
	{
		sc.nMinimumStepFailures = 0;
		sc.CheckAdvance_t0();
	}

	if (DynaModel_.Hmin() / DynaModel_.H() > 0.99)
		Restart();
	else
		ReInitializeNordsiek();

	sc.RefactorMatrix();
	DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG,
		fmt::format(CDFW2Messages::m_cszStepAndOrderChangedOnNewton,
			DynaModel_.Order(),
			DynaModel_.H()));
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

void MixedAdamsBDF::LeaveDiscontinuityMode()
{
	DynaModel_.SetH(DynaModel_.Hmin());
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
		sc.SetNordsiekScaledForH(DynaModel_.H());
		DynaModel_.BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
	// ставим флаг ресета Нордсика, чтобы не
	// контролировать соответствие предиктора-корректору
	// на стартап-шаге
	DynaModel_.ResetStep(true);
}

void MixedAdamsBDF::RepeatZeroCrossing(double rh)
{
	const auto& sc{ DynaModel_.StepControl() };
	// восстанавливаем Nordsieck с предыдущего шага
	RestoreNordsiek();
	// ограничиваем шаг до минимального
	if (DynaModel_.H() * rh < sc.Hmin)
	{
		// переходим на первый порядок, так
		// как снижение шага может быть очень
		// значительным
		ChangeOrder(1);
	}
	IntegratorBase::RepeatZeroCrossing(rh);
}

double MixedAdamsBDF::NextStepValue(const RightVector* pRightVector)
{
	const double lm{ Methodl[static_cast<ptrdiff_t>(pRightVector->PhysicalEquationType) * 2 + (DynaModel_.Order() - 1)][0] };
	return pRightVector->Nordsiek[0] + pRightVector->Error * lm;
}

double MixedAdamsBDF::StepStartValue(const RightVector* pRightVector)
{
	return pRightVector->Nordsiek[0];
}

double MixedAdamsBDF::FindZeroCrossingToConst(const RightVector* pRightVector, double dConst)
{
	const ptrdiff_t q{ DynaModel_.Order() };
	const double h{ DynaModel_.H() };

	const double dError{ pRightVector->Error };

	// получаем константу метода интегрирования
	const double* lm{ Methodl[pRightVector->EquationType * 2 + q - 1] };
	// рассчитываем коэффициенты полинома, описывающего изменение переменной
	double a{ 0.0 };		// если порядок метода 1 - квадратичный член равен нулю
	// линейный член
	const double b{ (pRightVector->Nordsiek[1] + dError * lm[1]) / h };
	// постоянный член
	double c{ (pRightVector->Nordsiek[0] + dError * lm[0]) };

	double GuardDiff{ DynaModel_.GetZeroCrossingTolerance() * (pRightVector->Rtol * std::abs(dConst) + pRightVector->Atol) };

	c -= dConst;
	if (c > 0)
		GuardDiff = std::min(c, GuardDiff);
	else
		GuardDiff = std::max(c, -GuardDiff);

	c -= 0.755 * GuardDiff;

	// если порядок метода 2 - то вводим квадратичный коэффициент
	if (q == 2)
		a = (pRightVector->Nordsiek[2] + dError * lm[2]) / h / h;
	// возвращаем отношение зеро-кроссинга для полинома
	const double rH{ GetZCStepRatio(a, b, c) };

	if (rH <= 0.0)
	{
		if (pRightVector->pDevice)
			pRightVector->pDevice->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(
				"Negative ZC ratio {} in device {}, variable {} at t={}. "
				"Nordsieck [{},{},{}], Constant = {}",
				rH,
				pRightVector->pDevice->GetVerbalName(),
				pRightVector->pDevice->VariableNameByPtr(pRightVector->pValue),
				GetCurrentTime(),
				pRightVector->Nordsiek[0],
				pRightVector->Nordsiek[1],
				pRightVector->Nordsiek[2],
				dConst
			));
	}
	return rH;
}

// возвращает отношение текущего шага к шагу до пересечения заданного a*t*t + b*t + c полинома
double MixedAdamsBDF::GetZCStepRatio(double a, double b, double c)
{
	// по умолчанию зеро-кроссинга нет - отношение 1.0
	double rH{ 1.0 };
	const double h{ DynaModel_.H() };

	if (Equal(a, 0.0))
	{
		// если квадратичный член равен нулю - просто решаем линейное уравнение
		//if (!Equal(b, 0.0))
		{
			const double h1{ -c / b };
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
			_ASSERTE(!(Equal(h1, (std::numeric_limits<double>::max)()) &&
				Equal(h2, (std::numeric_limits<double>::max)())));

			if (h1 > 0.0 || h1 < -h) h1 = (std::numeric_limits<double>::max)();
			if (h2 > 0.0 || h2 < -h) h2 = (std::numeric_limits<double>::max)();

			// возвращаем наименьший из действительных корней
			rH = (h + (std::min)(h1, h2)) / h;
		}
	}

	return rH;
}

double MixedAdamsBDF::FindZeroCrossingOfDifference(const RightVector* pRightVector1, const RightVector* pRightVector2)
{
	const ptrdiff_t q{ DynaModel_.Order() };
	const double h{ DynaModel_.H() };
	const double dError1{ pRightVector1->Error };
	const double dError2{ pRightVector2->Error };

	const double* lm1{ Methodl[pRightVector1->EquationType * 2 + q - 1] };
	const double* lm2{ Methodl[pRightVector2->EquationType * 2 + q - 1] };

	double a{ 0.0 };
	double b{ (pRightVector1->Nordsiek[1] + dError1 * lm1[1] - (pRightVector2->Nordsiek[1] + dError2 * lm2[1])) / h };
	double c{ (pRightVector1->Nordsiek[0] + dError1 * lm1[0] - (pRightVector2->Nordsiek[0] + dError2 * lm2[0])) };

	if (q == 2)
		a = (pRightVector1->Nordsiek[2] + dError1 * lm1[2] - (pRightVector2->Nordsiek[2] + dError2 * lm2[2])) / h / h;

	const double rH{ GetZCStepRatio(a, b, c) };

	if (rH <= 0.0)
	{
		if (pRightVector1->pDevice)
			pRightVector1->pDevice->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(
				"Negative ZC ratio {} in device {}, variable {} at t={}. "
				"Nordsieck1 [{},{},{}], Nordsieck2 [{},{},{}]",
				rH,
				pRightVector1->pDevice->GetVerbalName(),
				pRightVector1->pDevice->VariableNameByPtr(pRightVector1->pValue),
				GetCurrentTime(),
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

double MixedAdamsBDF::FindZeroCrossingOfModule(const RightVector* pRvre, const RightVector* pRvim, double Const, bool bCheckForLow)
{
	double rH{ 1.0 };
	const double  h{ DynaModel_.H() };
	const ptrdiff_t q{ DynaModel_.Order() };
	const double* lm{ Methodl[DET_ALGEBRAIC * 2 + q - 1] };
	
	// рассчитываем текущие напряжения по Нордсику (итоговые еще не рассчитаны в итерации)
	const double Vre1{ NextStepValue(pRvre) };
	const double Vim1{ NextStepValue(pRvim) };
	// коэффициенты первого порядка
	const double dVre1{ (pRvre->Nordsiek[1] + pRvre->Error * lm[1]) / h };
	const double dVim1{ (pRvim->Nordsiek[1] + pRvim->Error * lm[1]) / h };
	// коэффициенты второго порядка
	const double dVre2{ (q == 2) ? (pRvre->Nordsiek[2] + pRvre->Error * lm[2]) / h / h : 0.0 };
	const double dVim2{ (q == 2) ? (pRvim->Nordsiek[2] + pRvim->Error * lm[2]) / h / h : 0.0 };

	// функция значения переменной от шага
	// Vre(h) = Vre1 + h * dVre1 + h^2 * dVre2
	// Vim(h) = Vim1 + h * dVim1 + h^2 * dVim2

	// (Vre1 + h * dVre1 + h^2 * dVre2)^2 + (Vim1 + h * dVim1 + h^2 * dVim2)^2 - Boder^2 = 0


	if (q == 1)
	{
		// для первого порядка решаем квадратное уравнение
		// (Vre1 + h * dVre1)^2 + (Vim1 + h * dVim1)^2 - Boder^2 = 0
		// Vre1^2 + 2 * Vre1 * h * dVre1 + dVre1^2 * h^2 + Vim1^2 + 2 * Vim1 * h * dVim1 + dVim1^2 * h^2 - Border^2 = 0

		const double a{ dVre1 * dVre1 + dVim1 * dVim1 };
		const double b{ 2.0 * (Vre1 * dVre1 + Vim1 * dVim1) };
		const double c{ Vre1 * Vre1 + Vim1 * Vim1 - Const * Const };

		rH = GetZCStepRatio(a, b, c);
	}
	else
	{
		const double a{ dVre2 * dVre2 + dVim2 * dVim2 };												// (xs2^2 + ys2^2)*t^4 
		const double b{ 2.0 * (dVre1 * dVre2 + dVim1 * dVim1) };										// (2*xs1*xs2 + 2*ys1*ys2)*t^3 
		const double c{ (dVre1 * dVre1 + dVim1 * dVim1 + 2.0 * Vre1 * dVre2 + 2.0 * Vim1 * dVim2) };	// (xs1^2 + ys1^2 + 2*x1*xs2 + 2*y1*ys2)*t^2 
		const double d{ (2.0 * Vre1 * dVre1 + 2.0 * Vim1 * dVim1) };									// (2*x1*xs1 + 2*y1*ys1)*t 
		const double e{ Vre1 * Vre1 + Vim1 * Vim1 - Const * Const };									// x1^2 + y1^2 - e
		double t{ -0.5 * h };

		for (int i = 0; i < 5; i++)
		{
			double dt = (a * t * t * t * t + b * t * t * t + c * t * t + d * t + e) / (4.0 * a * t * t * t + 3.0 * b * t * t + 2.0 * c * t + d);
			t = t - dt;

			// здесь была проверка диапазона t, но при ее использовании возможно неправильное
			// определение доли шага, так как на первых итерациях значение может значительно выходить
			// за диапазон. Но можно попробовать контролировать диапазон не на первой, а на последующих итерациях

			if (std::abs(dt) < DFW2_EPSILON * 10.0)
				break;

			/*
			if (t > 0.0 || t < -h)
			{
				t = FLT_MAX;
				break;
			}
			*/
		}
		rH = (h + t) / h;
	}
	return rH;
}


const double MixedAdamsBDF::MethodlDefault[4][4] =
	//									   l0			l1			l2			Tauq
										{ { 1.0,		1.0,		0.0,		2.0  },				//  BDF-1
										  { 2.0 / 3.0,	1.0,		1.0 / 3.0,  4.5 },				//  BDF-2
										  { 1.0,		1.0,		0.0,		2.0  },				//  ADAMS-1
										  { 0.5,		1.0,		0.5,		12.0 } };			//  ADAMS-2
