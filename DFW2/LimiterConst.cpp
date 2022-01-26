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
		pDynaModel->SetVariableNordsiek(m_Output, m_dMax);
		dOdI = 0.0;
		break;
	case eLIMITEDSTATES::Min:
		pDynaModel->SetVariableNordsiek(m_Output, m_dMin);
		dOdI = 0.0;
		break;
	}

	if (!m_Device.IsStateOn())
		dOdI = 0.0;

	pDynaModel->SetElement(m_Output, m_Output, 1.0);
	pDynaModel->SetElement(m_Output, m_Input, dOdI);
}

void CLimiterConst::BuildRightHand(CDynaModel *pDynaModel)
{
	double dOdI{ m_Output - m_Input };

	/*if (m_Device.GetId() == 115 && pDynaModel->GetIntegrationStepNumber() == 38320)
	{
		const auto prv{ pDynaModel->GetRightVector(m_Output.Index) };
		m_Device.DebugLog(fmt::format("t={} / {} I={} O={} State={} {} {} {}",
			pDynaModel->GetCurrentTime(),
			pDynaModel->GetIntegrationStepNumber(),
			m_Input, m_Output, GetCurrentState(), prv->Nordsiek[0], prv->Nordsiek[1], prv->Nordsiek[2]));
	}
	*/

	if (m_Device.IsStateOn())
	{
		switch (GetCurrentState())
		{
		case eLIMITEDSTATES::Max:
			pDynaModel->SetVariableNordsiek(m_Output, m_dMax);
			dOdI = 0.0;
			break;
		case eLIMITEDSTATES::Min:
			pDynaModel->SetVariableNordsiek(m_Output, m_dMin);
			dOdI = 0.0;
			break;
		}
		pDynaModel->SetFunction(m_Output, dOdI);
	}
	else
		pDynaModel->SetFunction(m_Output, 0.0);
}

bool CLimiterConst::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitiveLimited::Init(pDynaModel);
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

// контроль зерокроссинга для состояния вне ограничения
double CLimiterConst::OnStateMid(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, m_dMaxH - m_Input, m_Input, m_dMaxH, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
	if (GetCurrentState() == eLIMITEDSTATES::Mid && !pDynaModel->GetZeroCrossingInRange(rH))
		if (CDynaPrimitive::ChangeState(pDynaModel, m_Input - m_dMinH, m_Input, m_dMinH, m_Input.Index, rH))
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Min);
	return rH;

}

double CLimiterConst::OnStateMin(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, m_dMin - m_Input, m_Input, m_dMin, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
	return rH;
}

double CLimiterConst::OnStateMax(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, m_Input - m_dMax, m_Input, m_dMax, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
	return rH;
}

eDEVICEFUNCTIONSTATUS CLimiterConst::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		const double CheckMax{ m_Input - m_dMax }, CheckMin{ m_dMin - m_Input };

		pDynaModel->CopyVariableNordsiek(m_Output, m_Input);

		// Bigger or Equal - very important !

		eLIMITEDSTATES OldState = GetCurrentState();

		if (CheckMax >= 0)
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Max);
			pDynaModel->SetVariableNordsiek(m_Output, m_dMax);
		}
		else if (CheckMin >= 0)
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Min);
			pDynaModel->SetVariableNordsiek(m_Output, m_dMin);
		}
		else
		{
			SetCurrentState(pDynaModel, eLIMITEDSTATES::Mid);
		}

		if (OldState != GetCurrentState())
			pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
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

