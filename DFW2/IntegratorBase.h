#pragma once
#include "MathUtils.h"
namespace DFW2
{
	class CDynaModel;
	class Parameters;

	using summatorT = MathUtils::StraightSummation;

	struct ConvergenceTest
	{
		ptrdiff_t nVarsCount;
		volatile double dErrorSum;
		double dOldErrorSum;
		double dCm;
		double dCms;
		double dOldCm;
		double dErrorSums;

		summatorT summator;

		void Reset()
		{
			summator.Reset();
			nVarsCount = 0;
		}

		void GetConvergenceRatio()
		{
			GetRMS();
			dCms = 0.7;
			if (!Consts::Equal(dOldErrorSum, 0.0))
			{
				dCm = dErrorSum / dOldErrorSum;
				dCms = (std::max)(0.2 * dOldCm, dCm);
			}
			dErrorSums = dErrorSum * (std::min)(1.0, 1.5 * dCms);
		}

		void GetRMS()
		{
			if (nVarsCount)
			{
				dErrorSum /= static_cast<double>(nVarsCount);
				dErrorSum = sqrt(dErrorSum);
			}
			else
				dErrorSum = dErrorSums = 0.0;
		}

		void ResetIterations()
		{
			dOldErrorSum = 0.0;
			dOldCm = 0.7;
		}

		void NextIteration()
		{
			dOldErrorSum = dErrorSum;
			dOldCm = dCm;
		}

		void AddError(double dError)
		{
			nVarsCount++;
			summator.Add(dError * dError);
		}

		void FinalizeSum()
		{
			dErrorSum = summator.Finalize();
		}

		// обработка диапазона тестов сходимости в массиве

		typedef ConvergenceTest ConvergenceTestVec[2];

		static void ProcessRange(ConvergenceTestVec& Range, void(*ProcFunc)(ConvergenceTest& ct))
		{
			for (auto&& it : Range) ProcFunc(it);
		}

		static void Reset(ConvergenceTest& ct) { ct.Reset(); }
		static void GetConvergenceRatio(ConvergenceTest& ct) { ct.GetConvergenceRatio(); }
		static void NextIteration(ConvergenceTest& ct) { ct.NextIteration(); }
		static void FinalizeSum(ConvergenceTest& ct) { ct.FinalizeSum(); }
		static void GetRMS(ConvergenceTest& ct) { ct.GetRMS(); }
		static void ResetIterations(ConvergenceTest& ct) { ct.ResetIterations(); }
	};

	class IntegratorBase
	{
	protected:
		bool fsal_ = false;
		using vecType = std::vector<double>;
		CDynaModel& DynaModel_;
		ConvergenceTest::ConvergenceTestVec ConvTest_;
		double StepRatio_ = 0.0;
		vecType::iterator VerifyVector(vecType& vec);
		vecType::iterator FromModel(vecType& vec);
		vecType::iterator ToModel(vecType& vec);
		vecType::iterator FromB(vecType& vec);
		vecType::iterator ToB(vecType& vec);
	public:
		IntegratorBase(CDynaModel& DynaModel) : DynaModel_(DynaModel) {}
		virtual ~IntegratorBase() = default;
		virtual void Step() = 0;
		virtual bool StepConverged() = 0;
		virtual void AcceptStep(bool DisableStepControl = false) = 0;
		virtual void RejectStep() = 0;
		virtual void UpdateStepSize() = 0;
		virtual void Init() = 0;
		virtual void NewtonUpdateIteration() = 0;
		virtual void NewtonBacktrack(const double* pVec, double lambda) = 0;
		//! Обработка отказа расчета Ньютоном
		virtual void NewtonFailed() = 0;
		virtual void WOperator(ptrdiff_t Row, ptrdiff_t Col, double& Value) = 0;
		virtual void BOperator() = 0;
		virtual void Restart() = 0 ;
		virtual void LeaveDiscontinuityMode() = 0;
		//! Функция подготовки к повтору шага для поиска зерокроссинга
		virtual void RepeatZeroCrossing(double rh);
		virtual bool ReportJacobiRefactor() const { return true; }
		virtual double NextStepValue(const RightVector* pRightVector) = 0;
		virtual double StepStartValue(const RightVector* pRightVector) = 0;
		virtual double StepStartDerivative(const RightVector* pRightVector) = 0;
		virtual double NextStepDerivative(const RightVector* pRightVector) = 0;
		virtual double FindZeroCrossingToConst(const RightVector* pRightVector, double dConst) = 0;
		virtual	double FindZeroCrossingOfDifference(const RightVector* pRightVector1, const RightVector* pRightVector2) = 0;
		virtual	double FindZeroCrossingOfModule(const RightVector* pRvre, const RightVector* pRvim, double Const, bool bCheckForLow) = 0;
		inline ConvergenceTest::ConvergenceTestVec& ConvTest()  { return ConvTest_; }
	};

	class IntegratorMultiStageBase : public IntegratorBase
	{
	protected:
		vecType::iterator f(vecType& vec);
		void f();
		IntegratorMultiStageBase(CDynaModel& DynaModel);
		virtual double NextH() const;
		double PrevNorm_ = 0.5;
		double alpha, beta, gamma;
		double d;
		IntegratorBase::vecType uprev, f0, flast;
		double ZeroCrossingInverseCubic(double fstart, double dfstart, double fend, double dfend) const;
	public:
		virtual int Order() const = 0;
		void Restart() override;
		void LeaveDiscontinuityMode() override;
		void AcceptStep(bool DisableStepControl = false) override;
		void RejectStep() override;
		void UpdateStepSize() override;
		void Init() override;
		bool StepConverged() override;
		void NewtonUpdateIteration() override;
		void NewtonBacktrack(const double* pVec, double lambda) override;
		void NewtonFailed() override;
		void WOperator(ptrdiff_t Row, ptrdiff_t  Col, double& Value) override;
		void BOperator() override;
		void RepeatZeroCrossing(double rh) override;
		bool ReportJacobiRefactor() const override { return false; } ;
		double NextStepValue(const RightVector* pRightVector) override;
		double StepStartValue(const RightVector* pRightVector) override;
		double FindZeroCrossingToConst(const RightVector* pRightVector, double dConst) override;
		double FindZeroCrossingOfDifference(const RightVector* pRightVector1, const RightVector* pRightVector2) override;
		double FindZeroCrossingOfModule(const RightVector* pRvre, const RightVector* pRvim, double Const, bool bCheckForLow) override;
	};
}