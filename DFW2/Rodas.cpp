#include "stdafx.h"
#include "DynaModel.h"
#include "Rodas.h"

using namespace DFW2;
Rosenbrock23::Rosenbrock23(CDynaModel& DynaModel) : IntegratorMultiStageBase(DynaModel)
{

}

void Rosenbrock23::Restart()
{
	fsal_ = false;
	DynaModel_.ResetStep(true);
	for (auto&& r :  DynaModel_.RightVectorRange())
		// в качестве значения принимаем то, что рассчитано в устройстве
		r.Tminus2Value = r.Nordsiek[0] = r.SavedNordsiek[0] = *r.pValue;
}


void Rosenbrock23::Step()
{
	const bool Refine{ false };

	auto uprevit{ VerifyVector(uprev) };
	auto f0it{ VerifyVector(f0) };
	auto f1it{ VerifyVector(f1) };
	auto f2it{ VerifyVector(f2) };  // f2 становится f0 на следующем шаге

	PrevNorm_ = ConvTest_[0].dErrorSum;
	if (PrevNorm_ == 0.0)
		PrevNorm_ = 0.5;

	auto& sc{ DynaModel_.StepControl() };
	const double h{ DynaModel_.H() };

	if (DynaModel_.IsInDiscontinuityMode())
	{
		DynaModel_.SolveNewton(20);
		return;
	}

	if (sc.nStepsCount == 2316)
		if (PrevNorm_ == 0.0)
			PrevNorm_ = 0.5;


	sc.RefactorMatrix(true);	// принудительно рефакторизуем матрицу

	FromModel(uprev);



	if (fsal_)
	{
		f0 = f2;
		DynaModel_.BuildMatrix(false);
	}
	else
	{
		for (auto&& it : DynaModel_.DeviceContainersPredict())
			it->Predict();
		DynaModel_.NewtonUpdateDevices();
		DynaModel_.BuildMatrix();
		FromB(f0);
	}

	DynaModel_.SolveLinearSystem(Refine);

	const double dth{ h * d };

	{
		uprevit = uprev.begin();
		auto BRange{ DynaModel_.BRange() };
		auto RVRange{ DynaModel_.RightVectorRange() };
		for (auto k1it{ VerifyVector(k1) }; k1it != k1.end(); ++k1it, ++BRange, ++RVRange, ++uprevit)
		{
			*k1it = *BRange / dth;
			*RVRange->pValue = *uprevit + 0.5 * h * *k1it;
		}
	}

	f(f1);

	{
		auto BRange{ DynaModel_.BRange() };
		auto RVRange{ DynaModel_.RightVectorRange() };
		for (const auto& k1it : k1)
		{
			if (RVRange->PhysicalEquationType == DET_DIFFERENTIAL)
				*BRange -= k1it;
			++RVRange;
			++BRange;
		}
	}

	DynaModel_.SolveLinearSystem(Refine);
	VerifyVector(k2);
	{
		auto BRange{ DynaModel_.BRange() };
		auto RVRange{ DynaModel_.RightVectorRange() };
		uprevit = uprev.begin();
		auto k1it{ k1.begin() };
		for (auto&& k2it : k2)
		{
			k2it = *BRange / dth + *k1it;
			*RVRange->pValue = *uprevit + h * k2it;
			++BRange;
			++RVRange;
			++uprevit;
			++k1it;
		}
	}

	f(f2);

	fsal_ = true;

	{
		auto BRange{ DynaModel_.BRange() };
		auto RVRange{ DynaModel_.RightVectorRange() };
		uprevit = uprev.begin();
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		auto f1it{ f1.begin() };
		for (const auto& f0it : f0)
		{
			*BRange += c32 * *f1it + 2 * f0it;
			if (RVRange->PhysicalEquationType == DET_DIFFERENTIAL)
				*BRange -= (c32 * *k2it + 2.0 * *k1it);

			++BRange;
			++RVRange;
			++f1it;
			++k1it;
			++k2it;
		}
	}


	DynaModel_.SolveLinearSystem(Refine);

	sc.Integrator.Weighted.Reset();
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);

	{
		auto BRange{ DynaModel_.BRange() };
		auto RVRange{ DynaModel_.RightVectorRange() };
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		for (const auto& uprevit : uprev)
		{
			const double ut{ h / 6 * (*k1it - 2 * *k2it + *BRange / dth) };
			const double dError{ RVRange->GetWeightedError(ut, (std::max)(std::abs(uprevit), std::abs(*RVRange->pValue))) };
			ConvTest_[0].AddError(dError);
			sc.Integrator.Weighted.Update(RVRange, dError);
			++RVRange;
			++BRange;
			++k1it;
			++k2it;
		}
	}


	if (sc.Integrator.Weighted.pVector)
		sc.Integrator.Weighted.pVector->ErrorHits++;

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetRMS);

	sc.m_bNewtonConverged = true;
	DynaModel_.ResetStep(false);
}

double Rosenbrock23::NextH() const
{
	const double Norm{ ConvTest_[0].dErrorSum };
	const double alpha{ 0.7 / 2.0 };
	const double beta{ 0.4 / 2.0 };
	const double gamma{ 0.95 };
	return gamma * DynaModel_.H() * std::pow(Norm, -alpha) * pow(PrevNorm_, beta);
}

void Rosenbrock23::AcceptStep(bool DisableStepControl)
{
	auto& sc{ DynaModel_.StepControl() };
	const double Norm{ ConvTest_[0].dErrorSum };
	sc.Advance_t0();
	sc.m_bRetryStep = false;
	sc.nMinimumStepFailures = 0;
	sc.OrderStatistics[0].nSteps++;
	if (!DisableStepControl)
	{
		DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG,
			fmt::format(CDFW2Messages::m_cszMultiStageSepAccepted, 
				DynaModel_.H(), 
				Norm,
				sc.Integrator.Weighted.Info()));

		const double alpha{ 0.7 / 2.0 };
		const double beta{ 0.4 / 2.0 };
		const double gamma{ 0.95 };
		DynaModel_.SetH(NextH());
		sc.StepChanged();
	}
}

void Rosenbrock23::RejectStep()
{
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };
	double newH{ NextH() };
	double newEffectiveH{ (std::max)(newH, sc.Hmin) };

	sc.m_bEnforceOut = false;	// отказываемся от вывода данных на данном заваленном шаге

	DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszStepChangedOnError,
		newEffectiveH,
		sc.Integrator.Weighted.Info()));

	// считаем статистику по заваленным шагам для текущего порядка метода интегрирования
	sc.OrderStatistics[0].nFailures++;

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
			DynaModel_.SetH(newEffectiveH);
		}
		// шагаем на следующее время без возможности возврата, так как шаг уже не снизить
		sc.Advance_t0();
		sc.Assign_t0();
	}
	else
	{
		// если шаг еще можно снизить
		sc.nMinimumStepFailures = 0;
		ToModel(uprev);
		DynaModel_.NewtonUpdateDevices();
		DynaModel_.SetH(newEffectiveH);
		sc.CheckAdvance_t0();
	}
}

void Rosenbrock23::UpdateStepSize()
{

}

void Rosenbrock23::Init()
{

}

bool Rosenbrock23::StepConverged()
{
	return ConvTest_[0].dErrorSum <= 1.0;
}

void Rosenbrock23::NewtonUpdateIteration()
{
	const auto& Parameters{ DynaModel_.Parameters() };
	auto& sc{ DynaModel_.StepControl() };

	sc.Newton.Reset();

	const double* pB{ DynaModel_.B() };

	for (auto&& r : DynaModel_.RightVectorRange())
	{
		const double dOldValue{ *r.pValue };
		r.b = *pB;
		sc.Newton.Absolute.Update(&r, std::abs(r.b));
		const double dNewValue{ dOldValue + r.b };

#ifdef USE_FMA
		const double dNewValue{ std::fma(Methodl0[pVectorBegin->EquationType], pVectorBegin->Error, pVectorBegin->Nordsiek[0]) };
#else
		
#endif

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

	bool bConvCheckConverged{ sc.Newton.Weighted.dMaxError < Parameters.m_dNewtonMaxNorm };

	if (bConvCheckConverged)
		sc.m_bNewtonConverged = true;
	else
		sc.RefactorMatrix(false);
}

void Rosenbrock23::NewtonBacktrack(const double* pVec, double lambda)
{

}

void Rosenbrock23::AOperator(ptrdiff_t Row, double& Value)
{
	//Value = -Value;
}

void Rosenbrock23::DOperator(ptrdiff_t Row, double& Value)
{
	if (DynaModel_.IsInDiscontinuityMode())
		Value = 0.0;
}

void Rosenbrock23::WOperator(ptrdiff_t Row, ptrdiff_t  Col, double& Value)
{
	// из SetElement знаки приходят
	// Алгебра - как есть
	// Дифур диагональ - знак как есть
	// Дифур внедиагональ - отрицательный
	// из SetEquation приходит -g !!!!
	const  RightVector* const pRightVector{ DynaModel_.GetRightVector() };
	if (DynaModel_.IsInDiscontinuityMode())
	{
		if ((pRightVector + Row)->PhysicalEquationType == DET_DIFFERENTIAL)
			Value = (Row == Col) ? 1.0 : 0.0;
		else
			Value = -Value;
	}
	else
	{
		if ((pRightVector + Row)->PhysicalEquationType == DET_DIFFERENTIAL)
		{
			if (Row == Col)
				Value = 1.0 / DynaModel_.H() / d - Value; // диагональ как есть
		}
		else
			Value = -Value;
	}
}