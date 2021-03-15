#include "stdafx.h"
#include "ZCDetector.h"
#include "DynaModel.h"

using namespace DFW2;

double CZCDetector::OnStateOff(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	double dHyst = pDynaModel->GetHysteresis(m_Input);
	if (CDynaPrimitive::ChangeState(pDynaModel, dHyst - m_Input, dHyst - m_Input, dHyst, m_Input.Index, rH))
		SetCurrentState(pDynaModel, RS_ON);
	return rH;
}

double CZCDetector::OnStateOn(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	double dHyst = pDynaModel->GetHysteresis(m_Input);
	if (CDynaPrimitive::ChangeState(pDynaModel, m_Input + dHyst, m_Input + dHyst, -dHyst, m_Input.Index, rH))
		SetCurrentState(pDynaModel, RS_OFF);
	return rH;
}

bool CZCDetector::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	eCurrentState = RS_OFF;
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));

	return bRes;
}

eDEVICEFUNCTIONSTATUS CZCDetector::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		if (m_Input > 0)
			SetCurrentState(pDynaModel, RS_ON);
		else
			SetCurrentState(pDynaModel, RS_OFF);

		m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}
