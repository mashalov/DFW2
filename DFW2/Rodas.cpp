#include "stdafx.h"
#include "DynaModel.h"
#include "Rodas.h"

using namespace DFW2;
Rodas4::Rodas4(CDynaModel& DynaModel) : IntegratorBase(DynaModel)
{

}

std::vector<double> Rodas4::f()
{
	RightVector* const pRightVector{ DynaModel_.GetRightVector() };
	RightVector* pVectorBegin{ pRightVector };
	const auto& Parameters{ DynaModel_.Parameters() };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };

	double* pB{ DynaModel_.B() };
	std::vector<double> vec(DynaModel_.MaxtrixSize());
	auto  vecit{ vec.begin() };
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++, vecit++)
	{
		*vecit = *pB;
		/*if (pVectorBegin->PhysicalEquationType == DET_ALGEBRAIC)
			*vecit += pVectorBegin->Nordsiek[0];*/
	}

	return vec;
}

void Rodas4::Step()
{
	const bool Refine{ false };
	std::vector<double> uprev(DynaModel_.MaxtrixSize());
	auto uprevit{ uprev.begin() };


	PrevNorm_ = ConvTest_[0].dErrorSum;

	if (PrevNorm_ == 0.0)
		PrevNorm_ = 0.5;

	RightVector* const pRightVector{ DynaModel_.GetRightVector() };
	RightVector* pVectorBegin{ pRightVector };
	auto& Parameters{ DynaModel_.Parameters() };
	auto& sc{ DynaModel_.StepControl() };
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
	const double h{ DynaModel_.H() };

	if (DynaModel_.IsInDiscontinuityMode())
	{
		DynaModel_.SolveNewton(20);
		return;
	}

	if(sc.nStepsCount == 2316)
		if (PrevNorm_ == 0.0)
			PrevNorm_ = 0.5;


	sc.RefactorMatrix(true);	// принудительно рефакторизуем матрицу

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, uprevit++)
		*uprevit = pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;


	for (auto&& it : DynaModel_.DeviceContainersPredict())
		it->Predict();
	DynaModel_.NewtonUpdateDevices();
	DynaModel_.BuildMatrix();

	auto f0{ f() };
		
	double* pB{ DynaModel_.B() };
	auto f0it{ f0.begin() };
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++, f0it++)
	{
		*pB = *f0it;
	}

	DynaModel_.SolveLinearSystem(Refine);

	pB = DynaModel_.B();
		
	std::vector<double> k1(DynaModel_.MaxtrixSize());
	auto  k1it{ k1.begin() };
	uprevit = uprev.begin();
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++, k1it++, uprevit++)
	{
		*k1it = *pB / h / d;
		*pVectorBegin->pValue = *uprevit + 0.5 * h * *k1it;
	}


	for (auto&& it : DynaModel_.DeviceContainersPredict())
		it->Predict();
	DynaModel_.NewtonUpdateDevices();
	DynaModel_.BuildRightHand();

	auto f1{ f() };

	auto f1it{ f1.begin() };
	pB = DynaModel_.B();
	k1it = k1.begin();

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++, k1it++, f1it++)
	{
		*pB = *f1it;
		if(pVectorBegin->PhysicalEquationType == DET_DIFFERENTIAL)
			*pB -= *k1it;
	}

	DynaModel_.SolveLinearSystem(Refine);


	std::vector<double> k2(DynaModel_.MaxtrixSize());
	auto  k2it{ k2.begin() };
	pB = DynaModel_.B();
	k1it = k1.begin();
	uprevit = uprev.begin();
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++, k1it++, k2it++, uprevit++)
	{
		*k2it = *pB / h / d + *k1it;
		*pVectorBegin->pValue = *uprevit + h * *k2it;
	}

	for (auto&& it : DynaModel_.DeviceContainersPredict())
		it->Predict();
	DynaModel_.NewtonUpdateDevices();
	DynaModel_.BuildRightHand();

	auto f2{ f() };
	auto f2it{ f2.begin() };

	pB = DynaModel_.B();
	f1it = f1.begin();
	f0it = f0.begin();
	k1it = k1.begin();
	k2it = k2.begin();

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++, k1it++, k2it++, f2it++, f1it++, f0it++)
		//*pB = *f2it - c32 * (*k2it - *f1it) - 2 * (*k1it - *f0it);
	{
		const double M{ (pVectorBegin->PhysicalEquationType == DET_ALGEBRAIC) ? 0.0 : c32 * *k2it + 2.0 * *k1it };
		*pB = *f2it - M + c32 * *f1it + 2 ** f0it;
			//linsolve_tmp = integrator.fsallast - mass_matrix * (c₃₂ * k₂ + 2 * k₁) + c₃₂ * f₁ + 2 * integrator.fsalfirst + dt * dT
	}

	DynaModel_.SolveLinearSystem(Refine);

	std::vector<double> ut(DynaModel_.MaxtrixSize());

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);

	k1it = k1.begin();
	k2it = k2.begin();
	pB = DynaModel_.B();

	sc.Integrator.Weighted.Reset();
	uprevit = uprev.begin();
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++, pB++, k1it++, k2it++, uprevit++)
	{
		const double ut{ h / 6 * (*k1it - 2 * *k2it + *pB / h / d ) };
		double dError = pVectorBegin->GetWeightedError(ut, (std::max)(std::abs(*uprevit),std::abs(*pVectorBegin->pValue)));
		ConvTest_[0].AddError(dError);
		sc.Integrator.Weighted.Update(pVectorBegin, dError);
	}


	if (sc.Integrator.Weighted.pVector)
		sc.Integrator.Weighted.pVector->ErrorHits++;

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetRMS);
	
	sc.m_bNewtonConverged = true;
	sc.m_bNordsiekReset = false;
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
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
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
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };
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
		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
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
	const RightVector* const pVectorEnd{ pVectorBegin + DynaModel_.MaxtrixSize() };

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