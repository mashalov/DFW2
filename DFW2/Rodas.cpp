#include "stdafx.h"
#include "DynaModel.h"
#include "Rodas.h"

using namespace DFW2;
Rodas4::Rodas4(CDynaModel& DynaModel) : IntegratorBase(DynaModel)
{

}

void Rodas4::Step()
{

}

void Rodas4::AcceptStep(bool DisableStepControl)
{

}

void Rodas4::RejectStep()
{

}

void Rodas4::UpdateStepSize()
{

}

void Rodas4::Init()
{

}

bool Rodas4::StepConverged()
{
	return true;
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

}

void Rodas4::DOperator(ptrdiff_t Row, double& Value)
{

}

void Rodas4::WOperator(ptrdiff_t Row, ptrdiff_t  Col, double& Value)
{

}