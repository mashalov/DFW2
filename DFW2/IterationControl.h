#pragma once
#include "Header.h"
namespace DFW2
{
	class CDynaNodeBase;
	struct _MatrixInfo;

	class LFNewtonStepRatio
	{
	public:
		enum class eStepLimitCause
		{
			None,
			Voltage,
			NodeAngle,
			BranchAngle,
			VoltageOutOfRange,
			BrancheAngleOutOfRange,
			Backtrack
		};

		double Ratio_ = 1.0;
		eStepLimitCause eStepCause = eStepLimitCause::None;
		CDynaNodeBase *pNode_ = nullptr,
					  *pNodeBranch_ = nullptr;

		void Reset();
		void UpdateVoltage(double Ratio, CDynaNodeBase* pNode);
		void UpdateNodeAngle(double Ratio, CDynaNodeBase* pNode);
		void UpdateVoltageOutOfRange(double Ratio, CDynaNodeBase* pNode);
		void UpdateBranchAngleOutOfRange(double Ratio, CDynaNodeBase* pNode, CDynaNodeBase* pNodeBranch);
		void UpdateBranchAngle(double Ratio, CDynaNodeBase* pNode, CDynaNodeBase* pNodeBranch);
	protected:
		bool UpdateRatio(double Ratio);
	};

	struct _MaxNodeDiff
	{
	protected:
		CDynaNodeBase* pNode_;
		double Diff;
		typedef bool (OperatorFunc)(double lhs, double rhs);

		void UpdateOp(CDynaNodeBase* pNode, double Value, OperatorFunc OpFunc) noexcept;
	public:
		_MaxNodeDiff() noexcept : pNode_(nullptr),
			Diff(0.0)
		{}

		ptrdiff_t GetId();
		double GetDiff();
		void UpdateMin(CDynaNodeBase* pNode, double Value);
		void UpdateMax(CDynaNodeBase* pNode, double Value);
		void UpdateMaxAbs(CDynaNodeBase* pNode, double Value);
	};

	struct _IterationControl
	{
		_MaxNodeDiff MaxImbP;
		_MaxNodeDiff MaxImbQ;
		_MaxNodeDiff MaxV;
		_MaxNodeDiff MinV;
		size_t Number = 0;
		size_t QviolatedCount = 0;
		double ImbRatio = 0.0;
		bool Converged(double m_dToleratedImb);
		void Reset();
		void Update(_MatrixInfo* pMatrixInfo);
	};
}

