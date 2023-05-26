#include "stdafx.h"
#include "DynaModel.h"
#include "MixedAdamsBDF.h"
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
	DynaModel_.SetH(1000.0 * DynaModel_.Hmin());
}

void IntegratorMultiStageBase::RepeatZeroCrossing(double rh)
{
	ToModel(uprev);
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

void IntegratorMultiStageBase::UpdateStepSize()
{

}

void IntegratorMultiStageBase::Init()
{
	alpha = 0.7 / static_cast<double>(Order());
	beta = 0.4 / static_cast<double>(Order());
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

double IntegratorMultiStageBase::FindZeroCrossingToConst(const RightVector* pRightVector, double dConst)
{
	// find zero crossing point by fitting inverse cubic polynomial 
	// François E.Cellier, Ernesto Kofman. Continuous System Simulation. https://doi.org/10.1007/0-387-30260-3

	const auto rvBase{ DynaModel_.GetRightVector() };
	const double fs{ uprev[pRightVector - rvBase] - dConst};	// f(t0) - dConst
	const double dfs{ f0[pRightVector - rvBase] };				// df/ft(t0)
	const double fl{ *pRightVector->pValue - dConst };			// f(t0+h)  - dConst
	const double dfl{ flast[pRightVector - rvBase] };			// df/dt(t0+h)

	const double h{ DynaModel_.H() };

	Eigen::Matrix4d A;
	A << fs * fs * fs, fs * fs, fs, 1.0,
		 fl * fl * fl, fl * fl, fl, 1.0,
		 3.0 * fs * fs, 2.0 * fs, 1.0, 0.0,
		 3.0 * fl * fl, 2.0 * fl, 1.0, 0.0;

	Eigen::Vector4d b(4);
	b << 0.0, 
		 h,
		 1.0 / dfs, 
		 1.0 / dfl;

	const double d{ A.lu().solve(b)(3) };
	return d / h;
}

double IntegratorMultiStageBase::FindZeroCrossingOfDifference(const RightVector* pRightVector1, const RightVector* pRightVector2)
{
	return 0.671;
}

double IntegratorMultiStageBase::FindZeroCrossingOfModule(const RightVector* pRvre, const RightVector* pRvim, double Const, bool bCheckForLow)
{
	return 0.671;
}