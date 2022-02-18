#include "stdafx.h"
#include "Relay.h"
#include "DynaModel.h"

using namespace DFW2;

// инициализация реле
bool CRelay::Init(CDynaModel *pDynaModel)
{
	// задаем уставки
	SetRefs(pDynaModel, Upper_, Lower_, MaxRelay_);
	// получаем исходное состояние
	eCurrentState = GetInstantState();
	return CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));;
}

// отслеживание состояния в несработанном состоянии
double CRelay::OnStateOff(CDynaModel *pDynaModel)
{
	double OnBound{ UpperH_ };
	double Check{ UpperH_ - Input_ };

	if (!MaxRelay_)
	{
		OnBound = LowerH_;
		Check = Input_ - LowerH_;
	}

	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, Check, Input_, OnBound, Input_.Index, rH))
		SetCurrentState(pDynaModel, eRELAYSTATES::RS_ON);

	return rH; 
}

// отслеживание состояния в сработанном состоянии
double CRelay::OnStateOn(CDynaModel *pDynaModel)
{
	double OnBound{ LowerH_ };
	double Check{ Input_ - LowerH_ };

	if (!MaxRelay_)
	{
		OnBound = UpperH_;
		Check = UpperH_ - Input_;
	}

	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, Check, Input_, OnBound, Input_.Index, rH))
		SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);
	return rH;

}


CRelay::eRELAYSTATES CRelay::GetInstantState()
{
	CRelay::eRELAYSTATES State = eCurrentState;	// берем текущее состояние
	if (MaxRelay_)
	{
		// для максимального реле 
		if (eCurrentState == eRELAYSTATES::RS_OFF)
		{
			// если реле не сработано
			// проверяем превышает ли вход уставку на срабатывание
			if (Input_ > Upper_)
				State = eRELAYSTATES::RS_ON;
		}
		else
		{
			// если реле сработано - проверяем не ниже ли вход уставки на отпускание
			if (Input_ <= Lower_)
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
			if (Input_ < Lower_)
				State = eRELAYSTATES::RS_ON;
		}
		else
		{
			// если реле сработано
			// проверяем не выше ли вход уставки на отпускание
			if (Input_ >= Upper_)
				State = eRELAYSTATES::RS_OFF;
		}
	}
	return State;
}

eDEVICEFUNCTIONSTATUS CRelay::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	// проверяем работает ли устройство, содержащее реле
	if (Device_.IsStateOn())
	{
		// получаем текущее состояние
		CRelay::eRELAYSTATES State = GetInstantState();
		// ставим полученное состояние
		SetCurrentState(pDynaModel, State);
		// транслируем состояние из enum в 0/1 на выход
		pDynaModel->SetVariableNordsiek(Output_, (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
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
	Upper_ = dUpper;		// порог срабатывания
	Lower_ = dLower;		// порог отпускания
	MaxRelay_ = MaxRelay;	// признак максимального реле

	// TODO еще бы проверять m_dUpper > m_dLower

	UpperH_ = Upper_ + pDynaModel->GetHysteresis(Upper_);	// порог на срабатывание увеличиваем на величину гистерезиса
	LowerH_ = Lower_ - pDynaModel->GetHysteresis(Lower_);	// порог на отпускание уменьшаем на величину гистерезиса

}


void CRelayDelay::SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay, double dDelay)
{
	CRelay::SetRefs(pDynaModel, dUpper, dLower, MaxRelay);
	Delay_ = dDelay;
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

	bool bRes{ true };

	eCurrentState = eRELAYSTATES::RS_OFF;
	if (Device_.IsStateOn())
	{
		eCurrentState = GetInstantState();
		bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	}
	return bRes && CDiscreteDelay::Init(pDynaModel);
}

void CRelayDelay::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	if (Equal(Delay_, 0.0))
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

				if (Output_ > 0.0)
					pDynaModel->DiscontinuityRequest(Device_, DiscontinuityLevel::Light);
			}
			break;
		case eRELAYSTATES::RS_OFF:
			if (CurrentState == eRELAYSTATES::RS_ON)
			{
				pDynaModel->SetStateDiscontinuity(this, Delay_);
			}
			break;
		}

		eCurrentState = CurrentState;
	}
}

eDEVICEFUNCTIONSTATUS CRelayDelay::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (Device_.IsStateOn())
	{
		CRelay::eRELAYSTATES OldState(eCurrentState);
		CRelay::eRELAYSTATES State = GetInstantState();

		SetCurrentState(pDynaModel, State);

		if ((Delay_ > 0 && !pDynaModel->CheckStateDiscontinuity(this)) || Delay_ == 0)
			pDynaModel->SetVariableNordsiek(Output_, (eCurrentState == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
	}
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


void CRelayDelay::RequestZCDiscontinuity(CDynaModel* pDynaModel)
{
	if (Equal(Delay_, 0.0))
		pDynaModel->DiscontinuityRequest(Device_, DiscontinuityLevel::Light);
}

bool CRelayDelay::NotifyDelay(CDynaModel *pDynaModel)
{
	pDynaModel->RemoveStateDiscontinuity(this);
	pDynaModel->SetVariableNordsiek(Output_, (GetCurrentState() == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
	return true;
}

bool CRelayDelayLogic::Init(CDynaModel *pDynaModel)
{
	bool bRes{ CRelayDelay::Init(pDynaModel) };
	if (bRes)
	{
		if (Device_.IsStateOn())
		{
			if (eCurrentState == eRELAYSTATES::RS_ON && Delay_ > 0)
			{
				pDynaModel->SetStateDiscontinuity(this, Delay_);
				eCurrentState = eRELAYSTATES::RS_OFF;
				pDynaModel->SetVariableNordsiek(Output_, 0.0);
			}
		}
	}
	return bRes;
}

bool CRelayDelayLogic::NotifyDelay(CDynaModel *pDynaModel)
{
	const double dOldOut{ Output_ };
	CRelayDelay::NotifyDelay(pDynaModel);
	if (Output_ != dOldOut)
		pDynaModel->NotifyRelayDelay(this);
	return true;
}


bool CDiscreteDelay::Init(CDynaModel *pDynaModel)
{
	return true;
}