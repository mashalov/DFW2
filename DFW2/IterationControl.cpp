#include "stdafx.h"
#include "IterationControl.h"
#include "DynaNode.h"
using namespace DFW2;

void LFNewtonStepRatio::Reset()
{
	m_dRatio = 1.0;
	m_eStepCause = eStepLimitCause::None;
	m_pNode = nullptr;
	m_pNodeBranch = nullptr;
}

void LFNewtonStepRatio::UpdateVoltage(double Ratio, CDynaNodeBase* pNode)
{
	if (UpdateRatio(Ratio))
	{
		m_eStepCause = eStepLimitCause::Voltage;
		m_pNode = pNode;
	}
}

void LFNewtonStepRatio::UpdateNodeAngle(double Ratio, CDynaNodeBase* pNode)
{
	if (UpdateRatio(Ratio))
	{
		m_eStepCause = eStepLimitCause::NodeAngle;
		m_pNode = pNode;
	}
}

void LFNewtonStepRatio::UpdateVoltageOutOfRange(double Ratio, CDynaNodeBase* pNode)
{
	if (UpdateRatio(Ratio))
	{
		m_eStepCause = eStepLimitCause::VoltageOutOfRange;
		m_pNode = pNode;
	}
}

void LFNewtonStepRatio::UpdateBranchAngleOutOfRange(double Ratio, CDynaNodeBase* pNode, CDynaNodeBase* pNodeBranch)
{
	if (UpdateRatio(Ratio))
	{
		m_eStepCause = eStepLimitCause::BrancheAngleOutOfRange;
		m_pNode = pNode;
		m_pNodeBranch = pNodeBranch;
	}
}

void LFNewtonStepRatio::UpdateBranchAngle(double Ratio, CDynaNodeBase* pNode, CDynaNodeBase* pNodeBranch)
{
	if (UpdateRatio(Ratio))
	{
		m_eStepCause = eStepLimitCause::BranchAngle;
		m_pNode = pNode;
		m_pNodeBranch = pNodeBranch;
	}
}

bool LFNewtonStepRatio::UpdateRatio(double Ratio)
{
	_ASSERTE(Ratio >= 0.0);
	if (Ratio < m_dRatio)
	{
		m_dRatio = Ratio * 0.95;
		return true;
	}
	else
		return false;
}

void _MaxNodeDiff::UpdateOp(CDynaNodeBase* pNode, double dValue, OperatorFunc OpFunc) noexcept
{
	if (pNode)
	{
		if (m_pNode)
		{
			if (OpFunc(dValue, m_dDiff))
			{
				m_pNode = pNode;
				m_dDiff = dValue;
			}
		}
		else
		{
			m_pNode = pNode;
			m_dDiff = dValue;
		}
	}
	else
		_ASSERTE(pNode);
}


ptrdiff_t _MaxNodeDiff::GetId()
{
	if (m_pNode)
		return m_pNode->GetId();
	return -1;
}

double _MaxNodeDiff::GetDiff()
{
	if (GetId() >= 0)
		return m_dDiff;
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
	UpdateOp(pNode, Value, [](double lhs, double rhs) noexcept -> bool { return fabs(lhs) > fabs(rhs); });
}


bool _IterationControl::Converged(double m_dToleratedImb)
{
	return fabs(m_MaxImbP.GetDiff()) < m_dToleratedImb && fabs(m_MaxImbQ.GetDiff()) < m_dToleratedImb && m_nQviolated == 0;
}

void _IterationControl::Reset()
{
	*this = _IterationControl();
}

void _IterationControl::Update(_MatrixInfo* pMatrixInfo)
{
	if (pMatrixInfo && pMatrixInfo->pNode)
	{
		CDynaNodeBase* pNode = pMatrixInfo->pNode;
		m_MaxImbP.UpdateMaxAbs(pNode, pMatrixInfo->m_dImbP);
		m_MaxImbQ.UpdateMaxAbs(pNode, pMatrixInfo->m_dImbQ);
		const double VdVnom = pNode->V / pNode->Unom;
		m_MaxV.UpdateMax(pNode, VdVnom);
		m_MinV.UpdateMin(pNode, VdVnom);
	}
	else
		_ASSERTE(pMatrixInfo && pMatrixInfo->pNode);
}



