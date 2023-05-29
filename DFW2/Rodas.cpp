#include "stdafx.h"
#include "DynaModel.h"
#include "Rodas.h"

using namespace DFW2;
Rosenbrock23::Rosenbrock23(CDynaModel& DynaModel) : IntegratorMultiStageBase(DynaModel) 
{
	d = 1.0 / (2.0 + sqrt(2.0));
}

void Rosenbrock23::Step()
{
	const bool Refine{ false };

	auto uprevit{ VerifyVector(uprev) };
	auto f0it{ VerifyVector(f0) };
	auto f1it{ VerifyVector(f1) };
	auto f2it{ VerifyVector(flast) };  // f2 становится f0 на следующем шаге

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

	sc.RefactorMatrix(true);	// принудительно рефакторизуем матрицу

	FromModel(uprev);
	DynaModel_.StoreStates();

	if (fsal_)
	{
		f0 = flast;
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

	f(flast);

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


double Rosenbrock23::StepStartDerivative(const RightVector* pRightVector)
{
	return k1[pRightVector - DynaModel_.GetRightVector()];
}

double Rosenbrock23::NextStepDerivative(const RightVector* pRightVector)
{
	return k2[pRightVector - DynaModel_.GetRightVector()];
}


Rodas4::Rodas4(CDynaModel& DynaModel) : IntegratorMultiStageBase(DynaModel)
{
	d = 0.25;
}

void Rodas4::Step()
{
	const bool Refine{ false };

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

	auto uprevit{ VerifyVector(uprev) };
	auto duit{ VerifyVector(flast) };

	const double dtC21{ C21 / h };
	const double dtC31{ C31 / h };
	const double dtC32{ C32 / h };
	const double dtC41{ C41 / h };
	const double dtC42{ C42 / h };
	const double dtC43{ C43 / h };
	const double dtC51{ C51 / h };
	const double dtC52{ C52 / h };
	const double dtC53{ C53 / h };
	const double dtC54{ C54 / h };
	const double dtC61{ C61 / h };
	const double dtC62{ C62 / h };
	const double dtC63{ C63 / h };
	const double dtC64{ C64 / h };
	const double dtC65{ C65 / h };

	const double dtd1{ h * d1 };
	const double dtd2{ h * d2 };
	const double dtd3{ h * d3 };
	const double dtd4{ h * d4 };

	sc.RefactorMatrix(true);	// принудительно рефакторизуем матрицу
	FromModel(uprev);
	DynaModel_.StoreStates();

	//W = calc_W(integrator, cache, dtgamma, repeat_step, true)


	if (fsal_)
		DynaModel_.BuildMatrix(false);
	else
	{
		for (auto&& it : DynaModel_.DeviceContainersPredict())
			it->Predict();
		DynaModel_.NewtonUpdateDevices();
		DynaModel_.BuildMatrix();
	}
	VerifyVector(f0);
	FromB(f0);
	//du = f(uprev, p, t)
	{
		//	linsolve_tmp = du + dtd1 * dT
		//	k1 = _reshape(W \ - _vec(linsolve_tmp), axes(uprev))
		DynaModel_.SolveLinearSystem(Refine);
		auto BRange{ DynaModel_.BRange() };
		uprevit = uprev.begin();
		auto k1it{ VerifyVector(k1) };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			*k1it = *BRange;
			//	u = uprev + a21 * k1
			//*r.pValue = *uprevit + a21 * *k1it;
			*r.pValue = std::fma(a21, *k1it, *uprevit) ;
			++uprevit;
			++k1it;
			++BRange;
		}
		//	du = f(u, p, t + c2 * dt)
		f();
	}
	{
		// linsolve_tmp = du + dtd2 * dT + mass_matrix * (dtC21 * k1)
		//	k2 = _reshape(W \ - _vec(linsolve_tmp), axes(uprev))
		auto BRange{ DynaModel_.BRange() };
		auto k1it{ k1.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			if (r.PhysicalEquationType == DET_DIFFERENTIAL)
				//*BRange += dtC21 * *k1it;
				*BRange = std::fma(dtC21, *k1it, *BRange);
			++BRange;
			++k1it;
		}
		DynaModel_.SolveLinearSystem(Refine); // k2 in B()
		VerifyVector(k2);
		FromB(k2);
	}

	{
		//	u = uprev + a31 * k1 + a32 * k2
		uprevit = uprev.begin();
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			//*r.pValue = *uprevit + a31 * *k1it + a32 * *k2it;
			*r.pValue = std::fma(a31, *k1it,  std::fma(a32, *k2it, *uprevit));
			++uprevit;
			++k1it;
			++k2it;
		}

		//	du = f(u, p, t + c3 * dt)
		f();
	}
	{
		//	linsolve_tmp = du + dtd3 * dT + mass_matrix * (dtC31 * k1 + dtC32 * k2)
		auto BRange{ DynaModel_.BRange() };
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			if (r.PhysicalEquationType == DET_DIFFERENTIAL)
				//*BRange += dtC31 * *k1it + dtC32 * *k2it;
				*BRange = std::fma(dtC31, *k1it, std::fma(dtC32, *k2it, *BRange));
			++BRange;
			++k1it;
			++k2it;
		}
		//	k3 = _reshape(W \ - _vec(linsolve_tmp), axes(uprev))
		DynaModel_.SolveLinearSystem(Refine);
		VerifyVector(k3);
		FromB(k3);
	}
	{
		//	u = uprev + a41 * k1 + a42 * k2 + a43 * k3
		uprevit = uprev.begin();
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		auto k3it{ k3.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			//*r.pValue = *uprevit + a41 * *k1it + a42 * *k2it + a43 * *k3it;
			*r.pValue = std::fma(a41, *k1it, std::fma(a42, *k2it, std::fma(a43, *k3it, *uprevit)));
			++uprevit;
			++k1it;
			++k2it;
			++k3it;
		}
		//	du = f(u, p, t + c3 * dt)
		f();
	}
	{
		//	linsolve_tmp = du + dtd4 * dT + mass_matrix * (dtC41 * k1 + dtC42 * k2 + dtC43 * k3)
		auto BRange{ DynaModel_.BRange() };
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		auto k3it{ k3.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			if (r.PhysicalEquationType == DET_DIFFERENTIAL)
				//*BRange += dtC41 * *k1it + dtC42 * *k2it + dtC43* *k3it;
				*BRange = std::fma(dtC41, *k1it, std::fma(dtC42, *k2it, std::fma(dtC43, *k3it, *BRange)));
			++BRange;
			++k1it;
			++k2it;
			++k3it;
		}
		//k4 = _reshape(W \ - _vec(linsolve_tmp), axes(uprev))
		DynaModel_.SolveLinearSystem(Refine);
		VerifyVector(k4);
		FromB(k4);
	}
	{
		//	u = uprev + a51 * k1 + a52 * k2 + a53 * k3 + a54 * k4
		uprevit = uprev.begin();
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		auto k3it{ k3.begin() };
		auto k4it{ k4.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			//*r.pValue = *uprevit + a51 * *k1it + a52 * *k2it + a53 * *k3it + a54 * *k4it;
			*r.pValue = std::fma(a51, *k1it, std::fma(a52, *k2it, std::fma(a53, *k3it, std::fma(a54, *k4it, *uprevit))));
			++uprevit;
			++k1it;
			++k2it;
			++k3it;
			++k4it;
		}
		//	du = f(u, p, t + c3 * dt)
		f();
	}
	{
		//	linsolve_tmp = du + mass_matrix * (dtC52 * k2 + dtC54 * k4 + dtC51 * k1 + dtC53 * k3)
		auto BRange{ DynaModel_.BRange() };
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		auto k3it{ k3.begin() };
		auto k4it{ k4.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			if (r.PhysicalEquationType == DET_DIFFERENTIAL)
				//*BRange += dtC52 * *k2it + dtC54 * *k4it + dtC51* *k1it + dtC53* * k3it;
				*BRange = std::fma(dtC52, *k2it, std::fma(dtC54, *k4it, std::fma(dtC51, *k1it, std::fma(dtC53, *k3it, *BRange))));
			++BRange;
			++k1it;
			++k2it;
			++k3it;
			++k4it;
		}
		//	k5 = _reshape(W \ - _vec(linsolve_tmp), axes(uprev))
		DynaModel_.SolveLinearSystem(Refine);
		VerifyVector(k5);
		FromB(k5);
	}
	{
		// u = u + k5
		auto k5it{ k5.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			*r.pValue += *k5it;
			++k5it;
		}
		//	du = f(u, p, t + c3 * dt)
		f();
	}
	{
		// linsolve_tmp = du + mass_matrix * (dtC61 * k1 + dtC62 * k2 + dtC65 * k5 + dtC64 * k4 + dtC63 * k3)
		auto BRange{ DynaModel_.BRange() };
		auto k1it{ k1.begin() };
		auto k2it{ k2.begin() };
		auto k3it{ k3.begin() };
		auto k4it{ k4.begin() };
		auto k5it{ k5.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			if (r.PhysicalEquationType == DET_DIFFERENTIAL)
				//*BRange += dtC61 * *k1it + dtC62 * *k2it + dtC65 * *k5it + dtC64 * *k4it + dtC63 * *k3it;
				*BRange = std::fma(dtC61, *k1it, std::fma(dtC62, *k2it, std::fma(dtC65, *k5it, std::fma(dtC64, *k4it, std::fma(dtC63, *k3it, *BRange)))));
			++BRange;
			++k1it;
			++k2it;
			++k3it;
			++k4it;
			++k5it;
		}
		//	k6 = _reshape(W \ - _vec(linsolve_tmp), axes(uprev))
		DynaModel_.SolveLinearSystem(Refine);
		VerifyVector(k6);
		FromB(k6);
	}
	{
		// u = u + k6
		auto k6it{ k6.begin() };
		for (auto&& r : DynaModel_.RightVectorRange())
		{
			*r.pValue += *k6it;
			++k6it;
		}
		//	du = f(u, p, t + c3 * dt)
		f(flast);
	}

//	atmp = calculate_residuals(k6, uprev, u, integrator.opts.abstol, integrator.opts.reltol, integrator.opts.internalnorm, t)
//	integrator.EEst = integrator.opts.internalnorm(atmp, t)

	sc.Integrator.Weighted.Reset();
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::Reset);
	auto k6it{ k6.begin() };
	uprevit = uprev.begin();
	for (auto&& r : DynaModel_.RightVectorRange())
	{
		const double dError{ r.GetWeightedError(*k6it, (std::max)(std::abs(*uprevit), std::abs(*r.pValue))) };
		ConvTest_[0].AddError(dError);
		sc.Integrator.Weighted.Update(&r, dError);
		++k6it;
		++uprevit;
	}

	if (sc.Integrator.Weighted.pVector)
		sc.Integrator.Weighted.pVector->ErrorHits++;

	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest_, ConvergenceTest::GetRMS);

	sc.m_bNewtonConverged = true;
	DynaModel_.ResetStep(false);
}


double derivative(double theta, double h, double k1, double k2, double y0, double y1)
{
	//k[1] + Θ * (-2 * k[1] + 2 * k[2] - 3 * k[2] * Θ) - y₀ + y₁) / dt
	return (k1 + theta * (-2.0 * k1 + 2 * k2 - 3 * k2 * theta) - y0 + y1) / h;
}

double Rodas4::StepStartDerivative(const RightVector* pRightVector)
{
	const ptrdiff_t ix{ pRightVector - DynaModel_.GetRightVector() };
	//const double df1{ (a51 * k1[ix] + a52 * k2[ix] + a53 * k3[ix] + a54 * k4[ix]) / DynaModel_.H() };
	const double rk1{ h21 * k1[ix] + h22 * k2[ix] + h23 * k3[ix] + h24 * k4[ix] + h25 * k5[ix] };
	const double rk2{ h31 * k1[ix] + h32 * k2[ix] + h33 * k3[ix] + h34 * k4[ix] + h35 * k5[ix] };
	const double df1{ derivative(0.0, DynaModel_.H(), rk1, rk2, uprev[ix], *pRightVector->pValue) };
	return df1;
}

double Rodas4::NextStepDerivative(const RightVector* pRightVector)
{
	const ptrdiff_t ix{ pRightVector - DynaModel_.GetRightVector() };
	//const double df1{ StepStartDerivative(pRightVector) + (k5[ix] + k6[ix]) / DynaModel_.H() };
	const double rk1{ h21 * k1[ix] + h22 * k2[ix] + h23 * k3[ix] + h24 * k4[ix] + h25 * k5[ix] };
	const double rk2{ h31 * k1[ix] + h32 * k2[ix] + h33 * k3[ix] + h34 * k4[ix] + h35 * k5[ix] };
	const double df1{ derivative(1.0, DynaModel_.H(), rk1, rk2, uprev[ix], *pRightVector->pValue) };
	return df1;
}