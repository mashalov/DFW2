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
	double OnBound{ Upper_ };
	// нужно переключать если Check < 0
	double Check{ Upper_ - Input_ };

	if (!MaxRelay_)
	{
		OnBound = Lower_;
		Check = Input_ - Lower_;
	}

	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, Check, Input_, OnBound, Input_.Index, rH))
		SetCurrentState(pDynaModel, eRELAYSTATES::RS_ON);

	return rH; 
}

// отслеживание состояния в сработанном состоянии
double CRelay::OnStateOn(CDynaModel *pDynaModel)
{
	double OnBound{ UpperH_};
	double Check{ Input_ - UpperH_ };

	if (!MaxRelay_)
	{
		OnBound = LowerH_;
		Check = LowerH_ - Input_;
	}

	double rH{ 1.0 };
	if (CDynaPrimitive::ChangeState(pDynaModel, Check, Input_, OnBound, Input_.Index, rH))
		SetCurrentState(pDynaModel, eRELAYSTATES::RS_OFF);
	return rH;

}


CRelay::eRELAYSTATES CRelay::GetInstantState()
{
	CRelay::eRELAYSTATES State{ eCurrentState };	// берем текущее состояние
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
			if (Input_ <= UpperH_)
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
			if (Input_ >= LowerH_)
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
		const CRelay::eRELAYSTATES State{ GetInstantState() };
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
	double Rt0{ 0.0 }, Rt1{ 1.0 };
	// забиваем значения по умолчанию значениями из заданного вектора по порядку
	CDynaPrimitive::UnserializeParameters({ Rt0, Rt1 }, Parameters);
	// вводим уставки максимального реле
	SetRefs(pDynaModel, Rt0, Rt0 * Rt1, true);
	return true;
}
bool CRelayMin::UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters)
{
	// уставка и коэффициент возврата по умолчанию
	double Rt0{ 0.0 }, Rt1{ 1.0 };
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

	UpperH_ = Upper_ - pDynaModel->GetHysteresis(Upper_);	// порог на срабатывание уменьшаем на величину гистерезиса
	LowerH_ = Lower_ + pDynaModel->GetHysteresis(Lower_);	// порог на отпускание увеличиваем на величину гистерезиса

}


void CRelayDelay::SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay, double dDelay)
{
	CRelay::SetRefs(pDynaModel, dUpper, dLower, MaxRelay);
	Delay_ = dDelay;
}

bool CRelayDelay::UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters)
{
	// уставка, коэффициент возврата и выдержка
	double Rt0{ 0.0 }, Rt1{ 0.0 }, Rt2{ 1.0 };
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
	double Rt0{ 0.0 }, Rt1{ 0.0 }, Rt2{ 1.0 };
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
	if (EnableInstantSwitch(pDynaModel))
	{
		// для реле с нулевой сравниваем состояния до и после
		const auto OldState{ eCurrentState };
		CRelay::SetCurrentState(pDynaModel, CurrentState);
		// и если реле переходит в положение "влключено" - сразу выдаем уведомление на
		// модель
		if (OldState == eRELAYSTATES::RS_OFF && CurrentState == eRELAYSTATES::RS_ON)
			NotifyDelay(pDynaModel);
	}
	else
	{
		// для реле с выдержкой
		switch (eCurrentState)
		{
		case eRELAYSTATES::RS_ON:
			if (CurrentState == eRELAYSTATES::RS_OFF)
			{
				// при отключении убираем событие на срабатывание с выдержкой
				pDynaModel->RemoveStateDiscontinuity(this);

				if (Output_ > 0.0)
					pDynaModel->DiscontinuityRequest(Device_, DiscontinuityLevel::Light);
			}
			break;
		case eRELAYSTATES::RS_OFF:
			if (CurrentState == eRELAYSTATES::RS_ON)
			{
				// при включении ставим безусловное событие на время выдержки
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
		const CRelay::eRELAYSTATES OldState{ eCurrentState };
		const CRelay::eRELAYSTATES State{ GetInstantState() };

		SetCurrentState(pDynaModel, State);
		// если реле может переключаться мгновенно
		// или его нет в перечне событий, сразу ставим выходное значение
		if ((!EnableInstantSwitch(pDynaModel) && !pDynaModel->CheckStateDiscontinuity(this)) || EnableInstantSwitch(pDynaModel))
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
	// удаляем условное событие, так как оно 
	// готово к тому, чтобы отработать
	pDynaModel->RemoveStateDiscontinuity(this);
	// ставим значение выхода по состоянию реле
	pDynaModel->SetVariableNordsiek(Output_, (GetCurrentState() == eRELAYSTATES::RS_ON) ? 1.0 : 0.0);
	return true;
}

bool CRelayDelayLogic::Init(CDynaModel *pDynaModel)
{
	// для реле логики ставим уставки вверх и вниз вблизи 0.0 и 1.0
	// чуть сдвинув их внутрь на гистерезис
	SetRefs(pDynaModel, 1.0 - 2.0 * pDynaModel->GetHysteresis(1.0), 
						0.0 + 2.0 * pDynaModel->GetHysteresis(0.0), 
						true, Delay_);

	bool bRes{ CRelayDelay::Init(pDynaModel) };
	if (bRes)
	{
		if (Device_.IsStateOn())
		{
			if (eCurrentState == eRELAYSTATES::RS_ON)
			{
				pDynaModel->SetStateDiscontinuity(this, Delay_);
				eCurrentState = eRELAYSTATES::RS_OFF;
				pDynaModel->SetVariableNordsiek(Output_, 0.0);
			}
		}
	}
	return bRes;
}

// вариант для реле логики - оно не может переключаться в отрицательном времени
// если выдержка равна нулю
bool CRelayDelayLogic::EnableInstantSwitch(CDynaModel* pDynaModel) const
{
	return CRelayDelay::EnableInstantSwitch(pDynaModel) && pDynaModel->PositiveTime();
}

// вариант для обычного реле - может переключаться в отрицательном времени
// если выдержка равна нулю
bool CRelayDelay::EnableInstantSwitch(CDynaModel* pDynaModel) const
{
	return Delay_ <= DFW2_EPSILON;
}


bool CRelayDelayLogic::NotifyDelay(CDynaModel *pDynaModel)
{
	// запоминаем текущее значение выхода
	const double dOldOut{ Output_ };
	// ставим выход в 1 или 0 по состоянию реле
	CRelayDelay::NotifyDelay(pDynaModel);
	// если состояние выхода изменилось
	// информируем модель о событии на этом реле
	if (Output_ != dOldOut)
		pDynaModel->NotifyRelayDelay(this);
	return true;
}


bool CDiscreteDelay::Init(CDynaModel *pDynaModel)
{
	return true;
}