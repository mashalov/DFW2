#include "stdafx.h"
#include "AndOrNot.h"
#include "DynaModel.h"

using namespace DFW2;

bool CAnd::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	m_Output = 0.0;
	ProcessDiscontinuity(pDynaModel);
	return bRes;
}

bool COr::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	m_Output = 0.0;
	ProcessDiscontinuity(pDynaModel);
	return bRes;
}

bool CNot::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	m_Output = 0.0;
	ProcessDiscontinuity(pDynaModel);
	return bRes;
}


eDEVICEFUNCTIONSTATUS CAnd::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dOldOut = m_Output;

		if (m_Input > 0 && m_Input1 > 0)
			m_Output = 1.0;
		else
			m_Output = 0.0;

		if (dOldOut != m_Output)
			pDynaModel->DiscontinuityRequest();
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS COr::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dOldOut = m_Output;

		if (m_Input  > 0 || m_Input1 > 0)
			m_Output = 1.0;
		else
			m_Output = 0.0;

		if (dOldOut != m_Output)
			pDynaModel->DiscontinuityRequest();
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CNot::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		double dOldOut = m_Output;

		if (m_Input > 0 )
			m_Output = 0.0;
		else
			m_Output = 1.0;

		if (dOldOut != m_Output)
			pDynaModel->DiscontinuityRequest();
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}