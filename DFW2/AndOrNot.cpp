#include "stdafx.h"
#include "AndOrNot.h"
#include "DynaModel.h"

using namespace DFW2;

bool CAnd::Init(CDynaModel *pDynaModel)
{
	m_Output = 0.0;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
}

bool COr::Init(CDynaModel *pDynaModel)
{
	m_Output = 0.0;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
}

bool CNot::Init(CDynaModel *pDynaModel)
{
	m_Output = 0.0;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
}


eDEVICEFUNCTIONSTATUS CAnd::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		const double dOldOut{ m_Output };

		if (m_Input > 0 && m_Input1 > 0)
			pDynaModel->SetVariableNordsiek(m_Output, 1.0);
		else
			pDynaModel->SetVariableNordsiek(m_Output, 0.0);

		if (dOldOut != m_Output)
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS COr::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		const double dOldOut{ m_Output };

		pDynaModel->SetVariableNordsiek(m_Output, 0.0);
		for (auto& inp : m_Inputs)
		{
			if (inp > 0)
			{
				pDynaModel->SetVariableNordsiek(m_Output, 1.0);
				break;
			}
		}

		if (dOldOut != m_Output)
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CNot::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		const double dOldOut{ m_Output };

		if (m_Input > 0 )
			pDynaModel->SetVariableNordsiek(m_Output, 0.0);
		else
			pDynaModel->SetVariableNordsiek(m_Output, 1.0);

		if (dOldOut != m_Output)
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}