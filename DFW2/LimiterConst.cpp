#include "stdafx.h"
#include "LimiterConst.h"
#include "DynaModel.h"

using namespace DFW2;

bool CLimiterConst::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double dInput = m_Input->Value();
	double dOdI = -1.0;

	switch (GetCurrentState())
	{
	case LS_MAX:
		*m_Output = dInput = m_dMax;
		dOdI = 0.0;
		break;
	case LS_MIN:
		*m_Output = dInput = m_dMin;
		dOdI = 0.0;
		break;
	}

	if (!m_pDevice->IsStateOn())
		dOdI = 0.0;

	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), dOdI);
	return bRes && pDynaModel->Status();
}

bool CLimiterConst::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dInput = m_Input->Value();
		switch (GetCurrentState())
		{
		case LS_MAX:
			*m_Output = dInput = m_dMax;
			break;
		case LS_MIN:
			*m_Output = dInput = m_dMin;
			break;
		}
		pDynaModel->SetFunction(A(m_OutputEquationIndex), *m_Output - dInput);
	}
	else
		pDynaModel->SetFunction(A(m_OutputEquationIndex), 0.0);

	return pDynaModel->Status();
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
	return StateMid(pDynaModel, m_Input->Value(), m_Input->Index());
}

double CLimiterConst::OnStateMin(CDynaModel *pDynaModel)
{
	return StateMin(pDynaModel, m_Input->Value() - m_dMin, m_Input->Value(), m_dMin, m_Input->Index());
}

double CLimiterConst::OnStateMax(CDynaModel *pDynaModel)
{
	return StateMax(pDynaModel, m_Input->Value() - m_dMax, m_Input->Value(), m_dMax, m_Input->Index());
}

eDEVICEFUNCTIONSTATUS CLimiterConst::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dInput   = m_Input->Value();
		double CheckMax = dInput - m_dMax;
		double CheckMin = m_dMin - dInput;
		*m_Output = m_Input->Value();

		// Bigger or Equal - very important !

		eLIMITEDSTATES OldState = GetCurrentState();

		if (CheckMax >= 0)
		{
			SetCurrentState(pDynaModel, LS_MAX);
			*m_Output = m_dMax;
		}
		else if (CheckMin >= 0)
		{
			SetCurrentState(pDynaModel, LS_MIN);
			*m_Output = m_dMin;
		}
		else
		{
			SetCurrentState(pDynaModel, LS_MID);
		}

		if (OldState != GetCurrentState())
			pDynaModel->DiscontinuityRequest();
	}

	return DFS_OK;
}

bool CLimiterConst::UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount)
{
	bool bRes = true;
	double LimiterParameters[2] = { -1E6, 1E6 };

	nParametersCount = min(nParametersCount, sizeof(LimiterParameters) / sizeof(LimiterParameters[0]));

	for (size_t i = 0; i < nParametersCount; i++)
		LimiterParameters[i] = pParameters[i];

	SetMinMax(pDynaModel, LimiterParameters[0], LimiterParameters[1]);
	return bRes;
}

