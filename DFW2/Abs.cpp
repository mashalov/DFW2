#include "stdafx.h"
#include "Abs.h"
#include "DynaModel.h"

using namespace DFW2;



bool CAbs::BuildEquations(CDynaModel *pDynaModel)
{
	pDynaModel->SetElement(m_Output, m_Input, m_bPositive ? 1.0 : -1.0);
	pDynaModel->SetElement(m_Output, m_Output, 1.0);
	return true;
}

bool CAbs::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		pDynaModel->SetFunction(m_Output, m_Output - (m_bPositive ? m_Input : -m_Input));
	}
	else
		pDynaModel->SetFunction(m_Output, 0.0);

	return true;
}



bool CAbs::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitive::Init(pDynaModel);
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

eDEVICEFUNCTIONSTATUS CAbs::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		if (m_Input >= 0)
			m_bPositive = true;
		else
			m_bPositive = false;

		m_Output = std::abs(m_Input);
	}
	else
		m_Output = 0.0;

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

// проверяем разрыв модуля

double CAbs::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (!m_Device.IsStateOn())
		return rH;

	double dHyst = pDynaModel->GetHysteresis(m_Input);
	if (m_bPositive)
	{
		if (CDynaPrimitive::ChangeState(pDynaModel, m_Input + dHyst, m_Input + dHyst, -dHyst, m_Input.Index, rH))
		{
			m_bPositive = false;
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
		}
	}
	else
	{
		if (CDynaPrimitive::ChangeState(pDynaModel, dHyst - m_Input, dHyst - m_Input, dHyst, m_Input.Index, rH))
		{
			m_bPositive = true;
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
		}
	}

	return rH;
}