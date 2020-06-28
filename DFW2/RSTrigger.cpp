#include "stdafx.h"
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
		double &R = m_Input->Value();
		double &S = m_Input1->Value();

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
			pDynaModel->DiscontinuityRequest();
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}
