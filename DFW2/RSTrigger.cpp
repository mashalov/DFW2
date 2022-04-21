#include "stdafx.h"
#include "RSTrigger.h"
#include "DynaModel.h"

using namespace DFW2;

bool CRSTrigger::Init(CDynaModel *pDynaModel)
{
	Output_ = 0.0;
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
}

eDEVICEFUNCTIONSTATUS CRSTrigger::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double dOldOut{ Output_ };
		const double& R{ Input_ };
		const double& S{ Input1_ };

		if (S > 0.0)
		{
			if (R > 0.0)
				pDynaModel->SetVariableNordsiek(Output_, ResetPriority_ ? 0.0 : 1.0);
			else
				pDynaModel->SetVariableNordsiek(Output_, 1.0);
		}
		else
		{
			if (R > 0.0)
				pDynaModel->SetVariableNordsiek(Output_, 0.0);
		}

		if (dOldOut != Output_)
			pDynaModel->DiscontinuityRequest(Device_, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CRSTrigger::UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double dResetPriority{ 1.0 };
	// по умолчанию приоритет имеет вход Reset
	CDynaPrimitive::UnserializeParameters(PRIMITIVEPARAMETERSDEFAULT{ { dResetPriority, 1.0 } }, Parameters);
	ResetPriority_ = dResetPriority > 0;
	return true;
}

bool CRSTrigger::GetResetPriority() const
{
	return ResetPriority_;
}

void CRSTrigger::SetResetPriority(bool bResetPriority)
{
	ResetPriority_ = bResetPriority;
}