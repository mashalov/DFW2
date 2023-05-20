#pragma once
#include "IntegratorBase.h"
namespace DFW2
{
	class Rodas4 : public IntegratorBase
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
	};
}
