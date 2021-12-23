#include "stdafx.h"
#include "Relay.h"
#include "DynaModel.h"

using namespace DFW2;

// инициализация реле
bool CRelay::Init(CDynaModel *pDynaModel)
{
	bool bRes{ true };
	// задаем уставки
	SetRefs(pDynaModel, m_dUpper, m_dLower, m_bMaxRelay);
	// получаем исходное состояние
	eCurrentState = GetInstantState();
	// обрабатываем разрыв
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

// отслеживание состояния в несработанном состоянии
double CRelay::OnStateOff(CDynaModel *pDynaModel)
{
	double OnBound = m_dUpperH;
	double Check = m_dUpperH - m_Input;

	if (!m_bMaxRelay)
	{
		OnBound = m_dLowerH;
		Check = m_Input - m_dLowerH;
	}

	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, Check, m_Input, OnBound, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eRELAYSTATES::RS_ON);

	return rH; 
}

// отслеживание состояния в сработанном состоянии
double CRelay::OnStateOn(CDynaModel *pDynaModel)
{
	double OnBound = m_dLowerH;
	double Check = m_Input - m_dLowerH;

	if (!m_bMaxRelay)
	{
		OnBound = m_dUpperH;
		Check = m_dUpperH - m_Input;
	}

	double rH = 1.0;
	if (CDynaPrimitive::ChangeState(pDynaModel, Check, m_Input, OnBound, m_Input.Index, rH))
		SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);
	return rH;

}


CRelay::eRELAYSTATES CRelay::GetInstantState()
{
	CRelay::eRELAYSTATES State = eCurrentState;	// берем текущее состояние
	if (m_bMaxRelay)
	{
		// для максимального реле 
		if (eCurrentState == eRELAYSTATES::RS_OFF)
		{
			// если реле не сработано
			// проверяем превышает ли вход уставку на срабатывание
			if (m_Input > m_dUpper)
				State = eRELAYSTATES::RS_ON;
		}
		else
		{
			// если реле сработано - проверяем не ниже ли вход уставки на отпускание
			if (m_Input <= m_dLower)
				State = eRELAYSTATES::RS_OFF;
		}
	}
	else
	{
		// для минимального реле
		if (eCurrentState == eRELAYSTATES::RS_OFF)
		{
			// если реле не сработано
			// проверяем не ниже ли вход уставки на срабатывание
			if (m_Input < m_dLower)
				State = eRELAYSTATES::RS_ON;
		}
		else
		{
			// если реле сработано
			// проверяем не выше ли вход уставки на отпускание
			if (m_Input >= m_dUpper)
				State = eRELAYSTATES::RS_OFF;
		}
	}
	return State;
}

eDEVICEFUNCTIONSTATUS CRelay::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	// проверяем работает ли устройство, содержащее реле
	if (m_Device.IsStateOn())
	{
		// получаем текущее состояние
		CRelay::eRELAYSTATES State = GetInstantState();
		// ставим полученное состояние
		SetCurrentState(pDynaModel, State);
		// транслируем состояние из enum в 0/1 на выход
		m_Output = (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0;
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CRelay::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	// уставка и коэффициент возврата по умолчанию
	double Rt0(0.0), Rt1(1.0);
	// забиваем значения по умолчанию значениями из заданного вектора по порядку
	CDynaPrimitive::UnserializeParameters({ Rt0, Rt1 }, Parameters);
	// вводим уставки максимального реле
	SetRefs(pDynaModel, Rt0, Rt0 * Rt1, true);
	return true;
}
bool CRelayMin::UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters)
{
	// уставка и коэффициент возврата по умолчанию
	double Rt0(0.0), Rt1(1.0);
	// забиваем значения по умолчанию значениями из заданного вектора по порядку
	CDynaPrimitive::UnserializeParameters({ Rt0, Rt1 }, Parameters);
	// вводим уставки минимального реле
	SetRefs(pDynaModel, Rt0, Rt0 * Rt1, false);
	return true;
}


void CRelay::SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay)
{
	m_dUpper = dUpper;		// порог срабатывания
	m_dLower = dLower;		// порог отпускания
	m_bMaxRelay = MaxRelay;	// признак максимального реле

	// TODO еще бы проверять m_dUpper > m_dLower

	m_dUpperH = m_dUpper + pDynaModel->GetHysteresis(m_dUpper);	// порог на срабатывание увеличиваем на величину гистерезиса
	m_dLowerH = m_dLower - pDynaModel->GetHysteresis(m_dLower);	// порог на отпускание уменьшаем на величину гистерезиса

}


void CRelayDelay::SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay, double dDelay)
{
	CRelay::SetRefs(pDynaModel, dUpper, dLower, MaxRelay);
	m_dDelay = dDelay;
}

bool CRelayDelay::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	// уставка, коэффициент возврата и выдержка
	double Rt0(0.0), Rt1(0.0), Rt2(1.0);
	// в параметрах последовательно: уставка, выдержка и коэффициент возврата
	// коэффициент возврата, как правило, остается значением по умолчанию
	CDynaPrimitive::UnserializeParameters({Rt0, Rt1, Rt2}, Parameters);
	// задаем уставки
	SetRefs(pDynaModel, Rt0, Rt0 * Rt2, true, Rt1);
	return true;
}

bool CRelayMinDelay::UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters)
{
	// уставка, коэффициент возврата и выдержка
	double Rt0(0.0), Rt1(0.0), Rt2(1.0);
	CDynaPrimitive::UnserializeParameters({ Rt0, Rt1, Rt2 }, Parameters);
	SetRefs(pDynaModel, Rt0, Rt0 * Rt2, false, Rt1);
	return true;
}

bool CRelayDelay::Init(CDynaModel *pDynaModel)
{

	bool bRes = true;

	eCurrentState = eRELAYSTATES::RS_OFF;
	if (m_Device.IsStateOn())
	{
		eCurrentState = GetInstantState();
		bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	}
	return bRes && CDiscreteDelay::Init(pDynaModel);
}

void CRelayDelay::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	if (Equal(m_dDelay, 0.0))
	{
		CRelay::SetCurrentState(pDynaModel, CurrentState);
	}
	else
	{
		switch (eCurrentState)
		{
		case eRELAYSTATES::RS_ON:
			if (CurrentState == eRELAYSTATES::RS_OFF)
			{
				pDynaModel->RemoveStateDiscontinuity(this);

				if (m_Output > 0.0)
					pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
			}
			break;
		case eRELAYSTATES::RS_OFF:
			if (CurrentState == eRELAYSTATES::RS_ON)
			{
				pDynaModel->SetStateDiscontinuity(this, m_dDelay);
			}
			break;
		}

		eCurrentState = CurrentState;
	}
}

eDEVICEFUNCTIONSTATUS CRelayDelay::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_Device.IsStateOn())
	{
		CRelay::eRELAYSTATES OldState(eCurrentState);
		CRelay::eRELAYSTATES State = GetInstantState();

		SetCurrentState(pDynaModel, State);

		if ((m_dDelay > 0 && !pDynaModel->CheckStateDiscontinuity(this)) || m_dDelay == 0)
		{
			m_Output = (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0;
		}
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


void CRelayDelay::RequestZCDiscontinuity(CDynaModel* pDynaModel)
{
	if (Equal(m_dDelay, 0.0))
		pDynaModel->DiscontinuityRequest(m_Device, DiscontinuityLevel::Light);
}

bool CRelayDelay::NotifyDelay(CDynaModel *pDynaModel)
{
	pDynaModel->RemoveStateDiscontinuity(this);
	m_Output = (GetCurrentState() == eRELAYSTATES::RS_ON) ? 1.0 : 0.0;
	return true;
}


bool CRelayDelayLogic::Init(CDynaModel *pDynaModel)
{
	bool bRes = CRelayDelay::Init(pDynaModel);
	if (bRes)
	{
		if (m_Device.IsStateOn())
		{
			if (eCurrentState == eRELAYSTATES::RS_ON && m_dDelay > 0)
			{
				pDynaModel->SetStateDiscontinuity(this, m_dDelay);
				eCurrentState = eRELAYSTATES::RS_OFF;
				m_Output = 0;
			}
		}
	}
	return bRes;
}

bool CRelayDelayLogic::NotifyDelay(CDynaModel *pDynaModel)
{
	double dOldOut = m_Output;
	CRelayDelay::NotifyDelay(pDynaModel);
	if (m_Output != dOldOut)
		pDynaModel->NotifyRelayDelay(this);
	return true;
}


bool CDiscreteDelay::Init(CDynaModel *pDynaModel)
{
	return true;
}