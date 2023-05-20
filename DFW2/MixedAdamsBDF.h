#pragma once
#include "IntegratorBase.h"

namespace DFW2
{
	class MixedAdamsBDF : public IntegratorBase
	{
	public:
		MixedAdamsBDF(CDynaModel& DynaModel);
		void Step() override;
		void AcceptStep(bool DisableStepControl = false) override;
		void RejectStep() override;
		void UpdateStepSize() override;
		void Init() override;
		bool StepConverged() override;
		void NewtonUpdateIteration() override;
		void NewtonBacktrack(const double* pVec, double lambda) override;
	protected:
		double GetRatioForHigherOrder();
		double GetRatioForCurrentOrder();
		double GetRatioForLowerOrder();
		void Predict();
		void UpdateNordsiek(bool bAllowSuppression = false);
		void ConstructNordsiekOrder();
		void ChangeOrder(ptrdiff_t newOrder);
		void RescaleNordsiek();
		void EnableAdamsCoefficientDamping(bool bEnable);
		void Computehl0();
		void ReInitializeNordsiek();
		void RestoreNordsiek();
		void DetectAdamsRinging();
		void WOperator(ptrdiff_t Row, ptrdiff_t  Col, double& Value) override;
		virtual void AOperator(ptrdiff_t Row, double& Value) override;
		virtual void DOperator(ptrdiff_t Row, double& Value) override;
	public:
		static const double MethodlDefault[4][4]; // фиксированные коэффициенты метода интегрирования
		double Methodl[4][4];	// текущие коэффициенты метода интегрирования
		double Methodlh[4];		// коэффициенты метода интегрирования l0, умноженные на шаг
	};
}
