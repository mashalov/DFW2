#include "stdafx.h"
#include "IterationControl.h"
#include "DynaNode.h"
using namespace DFW2;

void LFNewtonStepRatio::Reset()
{
	Ratio_ = 1.0;
	eStepCause = eStepLimitCause::None;
	pNode_ = nullptr;
	pNodeBranch_ = nullptr;
}

void LFNewtonStepRatio::UpdateVoltage(double Ratio, CDynaNodeBase* pNode)
{
	if (UpdateRatio(Ratio))
	{
		eStepCause = eStepLimitCause::Voltage;
		pNode_ = pNode;
	}
}

void LFNewtonStepRatio::UpdateNodeAngle(double Ratio, CDynaNodeBase* pNode)
{
	if (UpdateRatio(Ratio))
	{
		eStepCause = eStepLimitCause::NodeAngle;
		pNode_ = pNode;
	}
}

void LFNewtonStepRatio::UpdateVoltageOutOfRange(double Ratio, CDynaNodeBase* pNode)
{
	if (UpdateRatio(Ratio))
	{
		eStepCause = eStepLimitCause::VoltageOutOfRange;
		pNode_ = pNode;
	}
}

void LFNewtonStepRatio::UpdateBranchAngleOutOfRange(double Ratio, CDynaNodeBase* pNode, CDynaNodeBase* pNodeBranch)
{
	if (UpdateRatio(Ratio))
	{
		eStepCause = eStepLimitCause::BrancheAngleOutOfRange;
		pNode_ = pNode;
		pNodeBranch_ = pNodeBranch;
	}
}

void LFNewtonStepRatio::UpdateBranchAngle(double Ratio, CDynaNodeBase* pNode, CDynaNodeBase* pNodeBranch)
{
	if (UpdateRatio(Ratio))
	{
		eStepCause = eStepLimitCause::BranchAngle;
		pNode_ = pNode;
		pNodeBranch_ = pNodeBranch;
	}
}

bool LFNewtonStepRatio::UpdateRatio(double Ratio)
{
	_ASSERTE(Ratio >= 0.0);
	if (Ratio < Ratio_)
	{
		Ratio_ = Ratio * 0.95;
		return true;
	}
	else
		return false;
}

void _MaxNodeDiff::UpdateOp(CDynaNodeBase* pNode, double dValue, OperatorFunc OpFunc) noexcept
{
	if (pNode)
	{
		if (pNode_)
		{
			if (OpFunc(dValue, Diff))
			{
				pNode_ = pNode;
				Diff = dValue;
			}
		}
		else
		{
			pNode_ = pNode;
			Diff = dValue;
		}
	}
	else
		_ASSERTE(pNode);
}


ptrdiff_t _MaxNodeDiff::GetId()
{
	if (pNode_)
		return pNode_->GetId();
	return -1;
}

double _MaxNodeDiff::GetDiff()
{
	if (GetId() >= 0)
		return Diff;
	return 0.0;
}

void _MaxNodeDiff::UpdateMin(CDynaNodeBase* pNode, double Value)
{
	UpdateOp(pNode, Value, [](double lhs, double rhs) noexcept -> bool { return lhs < rhs; });
}

void _MaxNodeDiff::UpdateMax(CDynaNodeBase* pNode, double Value)
{
	UpdateOp(pNode, Value, [](double lhs, double rhs) noexcept -> bool { return lhs > rhs; });
}

void _MaxNodeDiff::UpdateMaxAbs(CDynaNodeBase* pNode, double Value)
{
	UpdateOp(pNode, Value, [](double lhs, double rhs) noexcept -> bool { return std::abs(lhs) > std::abs(rhs); });
}


bool _IterationControl::Converged(double m_dToleratedImb)
{
	return std::abs(MaxImbP.GetDiff()) < m_dToleratedImb && std::abs(MaxImbQ.GetDiff()) < m_dToleratedImb && QviolatedCount == 0;
}

void _IterationControl::Reset()
{
	const auto backNumber{ Number };
	*this = _IterationControl();
	Number = backNumber;
}

void _IterationControl::Update(_MatrixInfo* pMatrixInfo)
{
	if (pMatrixInfo && pMatrixInfo->pNode)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		MaxImbP.UpdateMaxAbs(pNode, pMatrixInfo->ImbP);
		MaxImbQ.UpdateMaxAbs(pNode, pMatrixInfo->ImbQ);
		const double VdVnom{ pNode->V / pNode->Unom };
		MaxV.UpdateMax(pNode, VdVnom);
		MinV.UpdateMin(pNode, VdVnom);
	}
	else
		_ASSERTE(pMatrixInfo && pMatrixInfo->pNode);
}