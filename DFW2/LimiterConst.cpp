#include "stdafx.h"
#include "LimiterConst.h"
#include "DynaModel.h"

using namespace DFW2;

void CLimiterConst::BuildEquations(CDynaModel *pDynaModel)
{
	double dOdI{ -1.0 };

	switch (GetCurrentState())
	{
	case eLIMITEDSTATES::Max:
		pDynaModel->SetVariableNordsiek(Output_, Max_);
		dOdI = 0.0;
		break;
	case eLIMITEDSTATES::Min:
		pDynaModel->SetVariableNordsiek(Output_, Min_);
		dOdI = 0.0;
		break;
	}

	if (!Device_.IsStateOn())
		dOdI = 0.0;

	pDynaModel->SetElement(Output_, Output_, 1.0);
	pDynaModel->SetElement(Output_, Input_, dOdI);
}

void CLimiterConst::BuildRightHand(CDynaModel *pDynaModel)
{
	double dOdI{ Output_ - Input_ };

	/*if (m_Device.GetId() == 115 && pDynaModel->GetIntegrationStepNumber() == 38320)
	{
		const auto prv{ pDynaModel->GetRightVector(m_Output.Index) };
		m_Device.DebugLog(fmt::format("t={} / {} I={} O={} State={} {} {} {}",
			pDynaModel->GetCurrentTime(),
			pDynaModel->GetIntegrationStepNumber(),
			m_Input, m_Output, GetCurrentState(), prv->Nordsiek[0], prv->Nordsiek[1], prv->Nordsiek[2]));
	}
	*/

	if (Device_.IsStateOn())
	{
		switch (GetCurrentState())
		{
		case eLIMITEDSTATES::Max:
			pDynaModel->SetVariableNordsiek(Output_, Max_);
			dOdI = 0.0;
			break;
		case eLIMITEDSTATES::Min:
			pDynaModel->SetVariableNordsiek(Output_, Min_);
			dOdI = 0.0;
			break;
		}
		pDynaModel->SetFunction(Output_, dOdI);
	}
	else
		pDynaModel->SetFunction(Output_, 0.0);
}

bool CLimiterConst::Init(CDynaModel *pDynaModel)
{
	bool bRes{ CDynaPrimitiveLimited::Init(pDynaModel) };
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

// контроль зерокроссинга для состояния вне ограничения
double CLimiterConst::OnStateMid(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, MaxH_ - Input_, Input_, MaxH_, Input_.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
	if (GetCurrentState() == eLIMITEDSTATES::Mid && !pDynaModel->GetZeroCrossingInRange(rH))
		if (CDynaPrimitive::ChangeState(pDynaModel, Input_ - MinH_, Input_, MinH_, Input_.Index, rH))
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Min);
	return rH;

}

double CLimiterConst::OnStateMin(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, Min_ - Input_, Input_, Min_, Input_.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
	return rH;
}

double CLimiterConst::OnStateMax(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, Input_ - Max_, Input_, Max_, Input_.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
	return rH;
}

eDEVICEFUNCTIONSTATUS CLimiterConst::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		const double CheckMax{ Input_ - Max_ }, CheckMin{ Min_ - Input_ };

		pDynaModel->CopyVariableNordsiek(Output_, Input_);

		// Bigger or Equal - very important !

		eLIMITEDSTATES OldState{ GetCurrentState() };

		if (CheckMax >= 0)
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
			pDynaModel->SetVariableNordsiek(Output_, Max_);
		}
		else if (CheckMin >= 0)
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Min);
			pDynaModel->SetVariableNordsiek(Output_, Min_);
		}
		else
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
		}

		if (OldState != GetCurrentState())
			pDynaModel->DiscontinuityRequest(Device_, DiscontinuityLevel::Light);
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CLimiterConst::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	double dMin(-1E6), dMax(-dMin);
	CDynaPrimitive::UnserializeParameters({ dMin, dMax }, Parameters);
	SetMinMax(pDynaModel, dMin, dMax);
	return true;
}

