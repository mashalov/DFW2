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

		double m_dRatio = 1.0;
		eStepLimitCause m_eStepCause = eStepLimitCause::None;
		CDynaNodeBase *m_pNode = nullptr,
					  *m_pNodeBranch = nullptr;

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
		CDynaNodeBase* m_pNode;
		double m_dDiff;
		typedef bool (OperatorFunc)(double lhs, double rhs);

		void UpdateOp(CDynaNodeBase* pNode, double dValue, OperatorFunc OpFunc) noexcept;
	public:
		_MaxNodeDiff() noexcept : m_pNode(nullptr),
			m_dDiff(0.0)
		{}

		ptrdiff_t GetId();
		double GetDiff();
		void UpdateMin(CDynaNodeBase* pNode, double Value);
		void UpdateMax(CDynaNodeBase* pNode, double Value);
		void UpdateMaxAbs(CDynaNodeBase* pNode, double Value);
	};

	struct _IterationControl
	{
		_MaxNodeDiff m_MaxImbP;
		_MaxNodeDiff m_MaxImbQ;
		_MaxNodeDiff m_MaxV;
		_MaxNodeDiff m_MinV;
		ptrdiff_t Number = 0;
		ptrdiff_t m_nQviolated = 0;
		double m_ImbRatio = 0.0;
		bool Converged(double m_dToleratedImb);
		void Reset();
		void Update(_MatrixInfo* pMatrixInfo);
	};
}

