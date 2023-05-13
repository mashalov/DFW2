#include "stdafx.h"
#include "AndOrNot.h"
#include "DynaModel.h"

using namespace DFW2;

bool CAnd::Init(CDynaModel *pDynaModel)
{
	Output_ = 0.0;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
}

bool COr::Init(CDynaModel *pDynaModel)
{
	Output_ = 0.0;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
}

bool CNot::Init(CDynaModel *pDynaModel)
{
	Output_ = 0.0;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
}


eDEVICEFUNCTIONSTATUS CAnd::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double dOldOut{ Output_ };

		if (Input_ > 0 && Input1_ > 0)
			pDynaModel->SetVariableNordsiek(Output_, 1.0);
		else
			pDynaModel->SetVariableNordsiek(Output_, 0.0);

		if (dOldOut != Output_)
			pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS COr::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double dOldOut{ Output_ };

		pDynaModel->SetVariableNordsiek(Output_, 0.0);
		for (auto& inp : Inputs_)
		{
			if (inp > 0)
			{
				pDynaModel->SetVariableNordsiek(Output_, 1.0);
				break;
			}
		}

		if (dOldOut != Output_)
			pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CNot::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double dOldOut{ Output_ };

		if (Input_ > 0 )
			pDynaModel->SetVariableNordsiek(Output_, 0.0);
		else
			pDynaModel->SetVariableNordsiek(Output_, 1.0);

		if (dOldOut != Output_)
			pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}