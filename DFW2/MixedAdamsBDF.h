#pragma once
#include "IntegratorBase.h"

namespace DFW2
{
	class MixedAdamsBDF final : public IntegratorBase
	{
	public:
		MixedAdamsBDF(CDynaModel& DynaModel);
		void Step() override;
		void AcceptStep(bool DisableStepControl = false) override;
		void RejectStep() override;
		void LeaveDiscontinuityMode() override;
		void UpdateStepSize() override;
		void Init() override;
		bool StepConverged() override;
		void FinishStep() override;
		void NewtonUpdateIteration() override;
		void NewtonBacktrack(const double* pVec, double lambda) override;
		void NewtonFailed() override;
		void WOperator(ptrdiff_t Row, ptrdiff_t  Col, double& Value) override;
		void BOperator() override;
		double BOperatorAlgebraic(ptrdiff_t Row, const double Value) override;
		double BOperatorDifferential(ptrdiff_t Row, const double Value) override;
		void Restart() override;
		void RepeatZeroCrossing(double rh) override;
		double NextStepValue(const RightVector* pRightVector) override;
		double StepStartValue(const RightVector* pRightVector) override;
		double StepStartDerivative(const RightVector* pRightVector) override { return 0.0; }
		double NextStepDerivative(const RightVector* pRightVector) override { return 0.0; }
		double FindZeroCrossingToConst(const RightVector* pRightVector, double dConst) override;
		double FindZeroCrossingOfDifference(const RightVector* pRightVector1, const RightVector* pRightVector2) override;
		double FindZeroCrossingOfModule(const RightVector* pRvre, const RightVector* pRvim, double Const, bool bCheckForLow) override;
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
		void SaveNordsiek();
		void InitNordsiek();
		double GetZCStepRatio(double a, double b, double c);
	public:
		static const double MethodlDefault[4][4]; // фиксированные коэффициенты метода интегрирования
		double Methodl[4][4];	// текущие коэффициенты метода интегрирования
		double Methodlh[4];		// коэффициенты метода интегрирования l0, умноженные на шаг
		static void InitNordsiekElement(struct RightVector* pVectorBegin, double Atol, double Rtol);
		static void PrepareNordsiekElement(struct RightVector* pVectorBegin);
		static inline constexpr const char* cszNoNordsiekHistory = "MixedAdamsBDF - No Nordsiek History";
	};
}
