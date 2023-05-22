#pragma once
#include "IntegratorBase.h"
namespace DFW2
{
	class Rodas4 : public IntegratorMultiStageBase
	{
	public:
		Rodas4(CDynaModel& DynaModel);
		void Step() override;
		void AcceptStep(bool DisableStepControl = false) override;
		void RejectStep() override;
		void UpdateStepSize() override;
		void Init() override;
		bool StepConverged() override;
		void NewtonUpdateIteration() override;
		void NewtonBacktrack(const double* pVec, double lambda) override;
		void WOperator(ptrdiff_t Row, ptrdiff_t  Col, double& Value) override;
		void AOperator(ptrdiff_t Row, double& Value) override;
		void DOperator(ptrdiff_t Row, double& Value) override;
	protected:
		static inline const double c32 = 6.0 + std::sqrt(2.0);
		static inline const double d = 1.0 / (2.0 + sqrt(2.0));
		IntegratorBase::vecType uprev, f0, f1, f2, k1, k2;
		double PrevNorm_ = 0.5;
		double NextH() const;
	};
}
