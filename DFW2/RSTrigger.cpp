﻿#include "stdafx.h"
#include "RSTrigger.h"
#include "DynaModel.h"

using namespace DFW2;

bool CRSTrigger::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	m_Output = 0.0;
	ProcessDiscontinuity(pDynaModel);
	return bRes;
}

eDEVICEFUNCTIONSTATUS CRSTrigger::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dOldOut = m_Output;
		const double &R = m_Input;
		const double &S = m_Input1;

		if (S > 0.0)
		{
			if (R > 0.0)
			{
				if (m_bResetPriority)
					m_Output = 0.0;
				else
					m_Output = 1.0;
			}
			else
				m_Output = 1.0;
		}
		else
		{
			if (R > 0.0)
				m_Output = 0.0;
		}

		if (dOldOut != m_Output)
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CRSTrigger::UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double dResetPriority(1.0);
	// по умолчанию приоритет имеет вход Reset
	CDynaPrimitive::UnserializeParameters({ { dResetPriority, 1.0 } }, Parameters);
	m_bResetPriority = dResetPriority > 0;
	return true;
}

bool CRSTrigger::GetResetPriority() const
{
	return m_bResetPriority;
}

void CRSTrigger::SetResetPriority(bool bResetPriority)
{
	m_bResetPriority = bResetPriority;
}