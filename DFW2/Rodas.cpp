#include "stdafx.h"
#include "DynaModel.h"
#include "Rodas.h"

using namespace DFW2;
Rodas4::Rodas4(CDynaModel& DynaModel) : IntegratorMultiStageBase(DynaModel)
{

}

void Rodas4::Restart()
{
	fsal_ = false;
	DynaModel_.ResetStep(true);
	for (auto RVRange{ DynaModel_.RightVectorRange() }; RVRange.begin < RVRange.end; RVRange.begin++)
		// в качестве значения принимаем то, что рассчитано в устройстве
		RVRange.begin->Tminus2Value = RVRange.begin->Nordsiek[0] = RVRange.begin->SavedNordsiek[0] = *RVRange.begin->pValue;
}


void Rodas4::Step()
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
		for (auto k1it{ VerifyVector(k1) }; k1it != k1.end(); k1it++, BRange.begin++, RVRange.begin++, uprevit++)
		{
			*k1it = *BRange.begin / dth;
			*RVRange.begin->pValue = *uprevit + 0.5 * h * *k1it;
		}
	}

	f(f1);

	{
		auto BRange{ DynaModel_.BRange() };
		auto RVRange{ DynaModel_.RightVectorRange() };
		for (const auto& k1it : k1)
		{
			if (RVRange.begin->PhysicalEquationType == DET_DIFFERENTIAL)
				*BRange.begin -= k1it;
			RVRange.begin++;
			BRange.begin++;
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
			k2it = *BRange.begin / dth + *k1it;
			*RVRange.begin->pValue = *uprevit + h * k2it;
			BRange.begin++;
			RVRange.begin++;
			uprevit++;
			k1it++;
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
			*BRange.begin += c32 * *f1it + 2 * f0it;
			if (RVRange.begin->PhysicalEquationType == DET_DIFFERENTIAL)
				*BRange.begin -= (c32 * *k2it + 2.0 * *k1it);

			BRange.begin++;
			RVRange.begin++;
			f1it++;
			k1it++;
			k2it++;
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
			const double ut{ h / 6 * (*k1it - 2 * *k2it + *BRange.begin / dth) };
			const double dError{ RVRange.begin->GetWeightedError(ut, (std::max)(std::abs(uprevit), std::abs(*RVRange.begin->pValue))) };
			ConvTest_[0].AddError(dError);
			sc.Integrator.Weighted.Update(RVRange.begin, dError);
			RVRange.begin++;
			BRange.begin++;
			k1it++;
			k2it++;
		}
	}


	if (sc.Integrator.Weighted.pVector)
		sc.Integrator.Weighted.pVector->ErrorHits++;

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetRMS);

	sc.m_bNewtonConverged = true;
	DynaModel_.ResetStep(false);
}

double Rodas4::NextH() const
{
	const double Norm{ ConvTest_[0].dErrorSum };
	const double alpha{ 0.7 / 2.0 };
	const double beta{ 0.4 / 2.0 };
	const double gamma{ 0.95 };
	return gamma * DynaModel_.H() * std::pow(Norm, -alpha) * pow(PrevNorm_, beta);
}

void Rodas4::AcceptStep(bool DisableStepControl)
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector() };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MatrixSize() };
	auto& sc{ DynaModel_.StepControl() };
	const auto& Parameters{ DynaModel_.Parameters() };
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

void Rodas4::RejectStep()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector() };
	RightVector* pVectorBegin{ pRightVector };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MatrixSize() };
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
		auto uprevit{ uprev.begin() };
		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, uprevit++)
			*pVectorBegin->pValue = *uprevit;

		DynaModel_.NewtonUpdateDevices();
		DynaModel_.SetH(newEffectiveH);
		sc.CheckAdvance_t0();
	}
}

void Rodas4::UpdateStepSize()
{

}

void Rodas4::Init()
{

}

bool Rodas4::StepConverged()
{
	return ConvTest_[0].dErrorSum <= 1.0;
}

void Rodas4::NewtonUpdateIteration()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector() };
	RightVector* pVectorBegin{ pRightVector };
	const auto& Parameters{ DynaModel_.Parameters() };
	auto& sc{ DynaModel_.StepControl() };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MatrixSize() };

	sc.Newton.Reset();

	const double* pB{ DynaModel_.B() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++)
	{
		const double dOldValue{ *pVectorBegin->pValue };
		pVectorBegin->b = *pB;
		sc.Newton.Absolute.Update(pVectorBegin, std::abs(pVectorBegin->b));
		const double dNewValue{ dOldValue + pVectorBegin->b };

#ifdef USE_FMA
		const double dNewValue{ std::fma(Methodl0[pVectorBegin->EquationType], pVectorBegin->Error, pVectorBegin->Nordsiek[0]) };
#else
		
#endif

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

	bool bConvCheckConverged{ sc.Newton.Weighted.dMaxError < Parameters.m_dNewtonMaxNorm };

	if (bConvCheckConverged)
		sc.m_bNewtonConverged = true;
	else
		sc.RefactorMatrix(false);
}

void Rodas4::NewtonBacktrack(const double* pVec, double lambda)
{

}

void Rodas4::AOperator(ptrdiff_t Row, double& Value)
{
	//Value = -Value;
}

void Rodas4::DOperator(ptrdiff_t Row, double& Value)
{
	const  RightVector* const pRightVector{ DynaModel_.GetRightVector() };
	if (DynaModel_.IsInDiscontinuityMode())
		Value = 0.0;
}

void Rodas4::WOperator(ptrdiff_t Row, ptrdiff_t  Col, double& Value)
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