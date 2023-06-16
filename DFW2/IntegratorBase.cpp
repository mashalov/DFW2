#include "stdafx.h"
#include "DynaModel.h"
#include "MixedAdamsBDF.h"
// eigen does not compile on x86 with AVX2 enabled
#if defined(_MSC_VER) && !defined(_WIN64)
#define EIGEN_DONT_VECTORIZE
#endif
#include "Eigen/Dense"

using namespace DFW2;

void IntegratorBase::RepeatZeroCrossing(double rh)
{
	DynaModel_.SetH( (std::max)(DynaModel_.Hmin(), rh * DynaModel_.H()));
	auto& sc{ DynaModel_.StepControl() };
	sc.CheckAdvance_t0();
	sc.OrderStatistics[DynaModel_.Order() - 1].nZeroCrossingsSteps++;
	DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszZeroCrossingStep,
		DynaModel_.H(),
		DynaModel_.ClosestZeroCrossingContainer()->GetZeroCrossingDevice()->GetVerbalName(),
		rh));
}

IntegratorBase::vecType::iterator IntegratorBase::VerifyVector(IntegratorBase::vecType& vec)
{
	if (vec.size() != DynaModel_.MatrixSize())
		vec.resize(DynaModel_.MatrixSize());
	return vec.begin();
}

IntegratorBase::vecType::iterator IntegratorBase::FromModel(IntegratorBase::vecType& vec)
{
	auto range{ DynaModel_.RightVectorRange() };
	if (range.size() != vec.size())
		throw dfw2error("IntegratorBase::FromModel vector size mismatch");

	for (auto vit{ vec.begin() }; vit!= vec.end(); vit++, ++range)
		*vit = *range->pValue;
	return vec.begin();
}

IntegratorBase::vecType::iterator IntegratorBase::ToModel(IntegratorBase::vecType& vec)
{
	auto range{ DynaModel_.RightVectorRange() };
	if (range.size() != vec.size())
		throw dfw2error("IntegratorBase::ToModel vector size mismatch");

	for (auto vit{ vec.begin() }; vit != vec.end(); vit++, ++range)
		*range->pValue  = *vit;

	return vec.begin();
}


IntegratorBase::vecType::iterator IntegratorBase::FromB(IntegratorBase::vecType& vec)
{
	auto range{ DynaModel_.BRange() };
	if (range.size() != vec.size())
		throw dfw2error("IntegratorBase::FromB vector size mismatch");

	for (auto vit{ vec.begin() }; vit != vec.end(); ++vit, ++range)
		*vit = *range;

	return vec.begin();
}

IntegratorBase::vecType::iterator IntegratorBase::ToB(IntegratorBase::vecType& vec)
{
	auto range{ DynaModel_.BRange() };
	if (range.size() != vec.size())
		throw dfw2error("IntegratorBase::ToB vector size mismatch");

	for (auto vit{ vec.begin() }; vit != vec.end(); ++vit, ++range)
		*range = *vit;

	return vec.begin();
}

IntegratorBase::vecType::iterator IntegratorMultiStageBase::f(IntegratorBase::vecType& vec)
{
	f();
	return FromB(vec);
}

void IntegratorMultiStageBase::f()
{
	for (auto&& it : DynaModel_.DeviceContainersPredict())
		it->Predict();
	DynaModel_.NewtonUpdateDevices();
	DynaModel_.BuildRightHand();
}

IntegratorMultiStageBase::IntegratorMultiStageBase(CDynaModel& DynaModel) :  IntegratorBase(DynaModel) {}

void IntegratorMultiStageBase::Restart()
{
	fsal_ = false;
}

void IntegratorMultiStageBase::LeaveDiscontinuityMode()
{
	DynaModel_.SetH(1e3 * DynaModel_.Hmin());
}

void IntegratorMultiStageBase::RepeatZeroCrossing(double rh)
{
	ToModel(uprev);
	DynaModel_.RestoreStates();
	DynaModel_.NewtonUpdateDevices();
	DynaModel_.ResetStep(true);
	Restart();
	IntegratorBase::RepeatZeroCrossing(rh);
}

void IntegratorMultiStageBase::AcceptStep(bool DisableStepControl)
{
	auto& sc{ DynaModel_.StepControl() };
	const double Norm{ ConvTest_[0].dErrorSum };
	sc.Advance_t0();
	sc.m_bRetryStep = false;
	sc.nMinimumStepFailures = 0;
	sc.OrderStatistics[0].nSteps++;
	DynaModel_.LogTime(DFW2MessageStatus::DFW2LOG_DEBUG,
		fmt::format(CDFW2Messages::m_cszMultiStageSepAccepted,
			DynaModel_.H(),
			Norm,
			sc.Integrator.Weighted.Info()));
	if (!DisableStepControl)
	{
		DynaModel_.SetH(NextH());
		sc.StepChanged();
	}
}

double IntegratorMultiStageBase::NextH() const
{
	const double Norm{ ConvTest_[0].dErrorSum };
	return gamma * DynaModel_.H() * std::pow(Norm, -alpha) * pow(PrevNorm_, beta);
}

void IntegratorMultiStageBase::RejectStep()
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
	if (newH <= sc.Hmin && Consts::Equal(DynaModel_.H(), sc.Hmin))
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
		DynaModel_.RestoreStates();
		DynaModel_.NewtonUpdateDevices();
		DynaModel_.SetH(newEffectiveH);
		sc.CheckAdvance_t0();
	}
}

void IntegratorMultiStageBase::UpdateStepSize()
{

}

void IntegratorMultiStageBase::Init()
{
	alpha = 1.0 / static_cast<double>(Order());
	beta = 0.0 / static_cast<double>(Order());
	gamma = 0.95;

	DynaModel_.InitDevicesNordsiek();

	for (auto&& r : DynaModel_.RightVectorRange())
		MixedAdamsBDF::InitNordsiekElement(&r, DynaModel_.Atol(), DynaModel_.Rtol());

	DynaModel_.SetH(0.0);
	auto& sc{ DynaModel_.StepControl() };
	sc.m_bDiscontinuityMode = true;
	DynaModel_.SolveNewton(100);
	sc.m_bDiscontinuityMode = false;

}

bool IntegratorMultiStageBase::StepConverged()
{
	return ConvTest_[0].dErrorSum <= 1.0;
}

void IntegratorMultiStageBase::NewtonUpdateIteration()
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

void IntegratorMultiStageBase::NewtonBacktrack(const double* pVec, double lambda)
{
	// константы метода выделяем в локальный массив, определяя порядок метода для всех переменных один раз
	const double* pB{ pVec };
	lambda -= 1.0;
	for (auto&& r : DynaModel_.RightVectorRange())
	{
		*r.pValue = *pB * lambda;
		++pB;
	}
}

void IntegratorMultiStageBase::NewtonFailed()
{

}


double IntegratorMultiStageBase::BOperatorAlgebraic(ptrdiff_t Row, const double Value)
{
	return Value;
}

double IntegratorMultiStageBase::BOperatorDifferential(ptrdiff_t Row, const double Value)
{
	if (DynaModel_.IsInDiscontinuityMode())
		return 0.0;
	else
		return Value;
}


void IntegratorMultiStageBase::BOperator()
{
	if (DynaModel_.IsInDiscontinuityMode())
	{
		auto BRange{ DynaModel_.BRange() };
		for (const auto& r : DynaModel_.RightVectorRange())
		{
			if (r.PhysicalEquationType == DET_DIFFERENTIAL)
				*BRange = 0.0;
			++BRange;
		}
	}
}

void IntegratorMultiStageBase::WOperator(ptrdiff_t Row, ptrdiff_t  Col, double& Value)
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

double IntegratorMultiStageBase::NextStepValue(const RightVector* pRightVector)
{
	return *pRightVector->pValue;
}

double IntegratorMultiStageBase::StepStartValue(const RightVector* pRightVector)
{
	return uprev[pRightVector - DynaModel_.GetRightVector()];
}

double IntegratorMultiStageBase::ZeroCrossingInverseCubic(double fstart, double dfstart, double fend, double dfend) const
{
	const double fssq{ fstart * fstart };
	const double fesq{ fend * fend };

	// сначала пытаемся найти точку пересечения zс-функции
	// с помощью инверсного кубического полинома
	Eigen::Matrix4d A;
	A << fstart * fssq,		fssq,			fstart,			1.0,
		 fend * fesq,		fesq,			fend,			1.0,
		 3.0 * fssq,		2.0 * fstart,	1.0,			0.0,
		 3.0 * fesq,		2.0 * fend,		1.0,			0.0;

	const double h{ DynaModel_.H() };

	Eigen::Vector4d b(4);
	b << 0.0, 	h,	 1.0 / dfstart,		1.0 / dfend;

	double d{ A.lu().solve(b)(3) };

	// если корень недопустим - стром 
	// прямой кубический полином и решаем
	// кубическое уравнение
	if (d <  0 || d > h)
	{
		Eigen::Matrix4d A1;
		A << 0.0,			0.0,		 0.0,		1.0,
			 h*h*h,			h*h,		 h,			1.0,	
			 0.0,			0.0,		 1.0,		0.0,
			 3.0*h*h,		2.0*h,		 1.0,		0.0;

		Eigen::Vector4d b1(4);
		b << fstart, fend, dfstart, dfend;
		auto sol{ A.lu().solve(b) };

		double cs[4] = { sol[3], sol[2], sol[1], sol[0] };
		double roots[4];
		int nroots{ MathUtils::CCubicSolver::Roots(cs, roots) };

		// выбираем минимальный допустимый корень
		d = 2 * h;
		for (int i = 0; i < nroots; i++)
			if (roots[i] >= 0 && roots[i] <= h)
				d = (std::min)(d, roots[i]);
	}

	// Может быть точное попадание, и тогда критерий
	// неравенства не сработает. Поэтому шаг делаем меньше
	// на долю минимального шага
	return (1.0 - 0.1 * DynaModel_.Hmin()) * d / h;
}

double IntegratorMultiStageBase::FindZeroCrossingToConst(const RightVector* pRightVector, double dConst)
{
	// find zero crossing point by fitting inverse cubic polynomial 
	// François E.Cellier, Ernesto Kofman. Continuous System Simulation. https://doi.org/10.1007/0-387-30260-3

	const double fstart{ StepStartValue(pRightVector) - dConst };	// f(t0) - dConst
	const double dfstart{ StepStartDerivative(pRightVector) };		// df/ft(t0)
	const double fend{ NextStepValue(pRightVector)- dConst };		// f(t0+h)  - dConst
	const double dfend{ NextStepDerivative(pRightVector) };			// df/dt(t0+h)
	return ZeroCrossingInverseCubic(fstart, dfstart, fend, dfend);
}
	

double IntegratorMultiStageBase::FindZeroCrossingOfDifference(const RightVector* pRightVector1, const RightVector* pRightVector2)
{
	const double fstart{ StepStartValue(pRightVector1) - StepStartValue(pRightVector2) };
	const double dfstart{ StepStartDerivative(pRightVector1) - StepStartDerivative(pRightVector2) };
	const double fend{ NextStepValue(pRightVector1) - NextStepValue(pRightVector2) };
	const double dfend{ NextStepDerivative(pRightVector1) - NextStepDerivative(pRightVector2) };
	return ZeroCrossingInverseCubic(fstart, dfstart, fend, dfend);
}

double IntegratorMultiStageBase::FindZeroCrossingOfModule(const RightVector* pRvre, const RightVector* pRvim, double Const, bool bCheckForLow)
{
	
	// f(t) = vre(t)^2 + vim(t)^2 - Const^2
	// df(t)/dt = 2*vre(t)*dvre(t)/dt + 2*vim(t)*dvim(t)/dt

	const auto rvBase{ DynaModel_.GetRightVector() };
	double VreStart{ StepStartValue(pRvre) };
	double VimStart{ StepStartValue(pRvim) };
	double dVreStart{ StepStartDerivative(pRvre) };
	double dVimStart{ StepStartDerivative(pRvim) };
	double VreEnd{ NextStepValue(pRvre) };
	double VimEnd{ NextStepValue(pRvim) };
	double dVreEnd{ NextStepDerivative(pRvre) };
	double dVimEnd{ NextStepDerivative(pRvim) };

	const double dfstart{ 2.0 * (VreStart * dVreStart + VimStart * dVimStart) };
	const double dfend{ 2.0 * (VreEnd * dVreEnd + VimEnd * dVimEnd) };

	VreStart *= VreStart;
	VimStart *= VimStart;
	VreEnd *= VreEnd;
	VimEnd *= VimEnd;
	Const *= Const;

	const double fstart{ VreStart + VimStart - Const };
	const double fend{ VreEnd + VimEnd - Const };

	return ZeroCrossingInverseCubic(fstart, dfstart, fend, dfend);
}