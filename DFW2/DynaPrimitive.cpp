#include "stdafx.h"
#include "DynaPrimitive.h"
#include "DynaModel.h"
#include "MathUtils.h"

using namespace DFW2;

bool CDynaPrimitive::Init(CDynaModel *pDynaModel)
{
	return true;
}

bool CDynaPrimitiveLimited::Init(CDynaModel *pDynaModel)
{
	bool bRes{ CDynaPrimitive::Init(pDynaModel) };
	if (bRes)
	{
		eCurrentState = eLIMITEDSTATES::Mid;
		
		if (Min_ > Max_)
		{
			Device_.Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongPrimitiveLimits,
																   GetVerbalName(), 
																   Device_.GetVerbalName(), 
																   Min_, 
																   Max_));
			bRes = false;
		}

		SetMinMax(pDynaModel, Min_, Max_);

		if ((Output_ > MaxH_ || Output_ < MinH_ ) && Device_.IsStateOn())
		{
			Device_.Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongPrimitiveInitialConditions,
																   GetVerbalName(), 
																   Device_.GetVerbalName(), 
																   Output_, Min_, Max_));
			bRes = false;
		}
	}
	return bRes;
}

double CDynaPrimitive::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	return 1.0;
}

bool CDynaPrimitive::UnserializeParameters(DOUBLEREFVEC ParametersList, const DOUBLEVECTOR& Parameters)
{
	auto dest{ ParametersList.begin() };
	for (auto& src : Parameters)
		if (dest != ParametersList.end())
		{
			dest->get() = src;
			dest++;
		}
	return true;
}

bool CDynaPrimitive::UnserializeParameters(PRIMITIVEPARAMETERSDEFAULT ParametersList, const DOUBLEVECTOR& Parameters)
{
	auto src{ Parameters.begin() };
	for (auto& dest : ParametersList)
		if (src != Parameters.end())
		{
			dest.first.get() = *src;
			src++;
		}
		else
			dest.first.get() = dest.second;
	return true;
}

bool CDynaPrimitive::ChangeState(CDynaModel *pDynaModel, double Diff, double TolCheck, double Constraint, ptrdiff_t ValueIndex, double &rH)
{
	// Diff			- контроль знака - если < 0 - переходим в LS_MID
	// TolCheck		- значение для контроля относительной погрешности
	// Constraint	- константа, относительно которой определяем пересечение

	bool bChangeState{ false };
	rH = 1.0;

	if (Diff < 0.0)
	{
		// обращаемся к устройству с запросом
		// на уточнение зеро-кроссинга
		if (Device_.DetectZeroCrossingFine(this))
		{
			const auto pRightVector{ pDynaModel->GetRightVector(ValueIndex) };
			// определяем расстояние до точки zero-crossing в долях от текущего шага
			rH = pDynaModel->FindZeroCrossingToConst(pRightVector, Constraint);
			if (pDynaModel->GetZeroCrossingInRange(rH))
			{
				// проверяем погрешность зеро-кроссинга по значению со взвешиванием
				// так же как в контроле точности шага
				const double derr{ std::abs(pRightVector->GetWeightedError(Diff, TolCheck)) };
				if (derr < pDynaModel->GetZeroCrossingTolerance())
				{
					// если точность удовлетворительная, изменяем состояние
					// примитива и шаг не уменьшаем
					bChangeState = true;
					rH = 1.0;
				}
				else
				{
					// если точность зеро-кроссинга не достигнута,
					// проверяем можно ли еще снизить шаг

					if (pDynaModel->ZeroCrossingStepReached(rH))
					{
						// если нет - меняем состояние
						bChangeState = true;
						rH = 1.0;
					}
				}
			}
			else
			{
				bChangeState = true;
				_ASSERTE(0); // корня нет, но знак изменился !
				//rH = pDynaModel->FindZeroCrossingToConst(pRightVector, Constraint);
				rH = 1.0;
			}
		}
		else
		{
			// если устройство не разрешило уточнение 
			// времени зеро-кроссинга - меняем состояние
			bChangeState = true;
		}
	}

	return bChangeState;
}

// определение доли шага зерокроссинга для примитива с минимальным и максимальным ограничениями

double CDynaPrimitiveLimited::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	const eLIMITEDSTATES oldCurrentState{ eCurrentState };
	switch (eCurrentState)
	{
	case eLIMITEDSTATES::Mid:
		rH = OnStateMid(pDynaModel);
		break;
	case eLIMITEDSTATES::Max:
		rH = OnStateMax(pDynaModel);
		break;
	case eLIMITEDSTATES::Min:
		rH = OnStateMin(pDynaModel);
		break;
	}

	return rH;
}

void CDynaPrimitiveLimited::SetCurrentState(CDynaModel* pDynaModel, eLIMITEDSTATES CurrentState)
{
	if (eCurrentState != CurrentState)
	{
		const auto fnDecodeState = [](CDynaPrimitiveLimited::eLIMITEDSTATES State)
		{
			constexpr std::array<const char*, 3> VerbStates = {"Min", "Mid", "Max"};
			return VerbStates[static_cast<std::underlying_type<CDynaPrimitiveLimited::eLIMITEDSTATES>::type>(State)];
		};

		// если состояние изменилось, запрашиваем обработку разрыва
		pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
		pDynaModel->LogTime(DFW2MessageStatus::DFW2LOG_DEBUG,
				fmt::format(CDFW2Messages::m_cszPrimitiveChangesState,
					GetVerbalName(),
					Device_.GetVerbalName(),
					Output_,
					Min_, Max_,
					(fnDecodeState)(eCurrentState),
					(fnDecodeState)(CurrentState)));
	}
	eCurrentState = CurrentState;
}

void CDynaPrimitiveLimited::SetMinMax(CDynaModel *pDynaModel, double dMin, double dMax)
{
	Min_ = dMin;
	Max_ = dMax;
	MinH_ = Min_ - pDynaModel->GetHysteresis(Min_);
	MaxH_ = Max_ + pDynaModel->GetHysteresis(Max_);
}

void CDynaPrimitiveBinary::InvertState(CDynaModel *pDynaModel)
{
	SetCurrentState(pDynaModel, GetCurrentState() == eRELAYSTATES::RS_ON ? eRELAYSTATES::RS_OFF : eRELAYSTATES::RS_ON);
}

// изменить состояние примитива
void CDynaPrimitiveBinary::SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)
{
	if (eCurrentState != CurrentState)
	{
		// если текущее состояние не соответствует заданному
		// запрашиваем обработку разрыва
		pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
	}
	eCurrentState = CurrentState;
}

double CDynaPrimitiveBinaryOutput::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };

	if (Device_.IsStateOn())
	{
		eRELAYSTATES oldCurrentState{ eCurrentState };

		switch (eCurrentState)
		{
		case eRELAYSTATES::RS_ON:
			rH = OnStateOn(pDynaModel);
			break;
		case eRELAYSTATES::RS_OFF:
			rH = OnStateOff(pDynaModel);
			break;
		}

		if (oldCurrentState != eCurrentState)
			RequestZCDiscontinuity(pDynaModel);
	}
	return rH;
}

void CDynaPrimitiveBinary::RequestZCDiscontinuity(CDynaModel* pDynaModel)
{
	pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
}

void CDynaPrimitiveBinary::BuildEquations(CDynaModel *pDynaModel)
{
	pDynaModel->SetElement(Output_, Output_, 1.0);
}

void CDynaPrimitiveBinary::BuildRightHand(CDynaModel *pDynaModel)
{
	pDynaModel->SetFunction(Output_, 0.0);
}

std::string CDynaPrimitive::GetVerboseName()
{
	const auto pRightVector{ Device_.GetModel()->GetRightVector(Output_) };
	constexpr const char* cszUnknown{ "\"unknown\"" };
	return fmt::format("{} {} {} {}", 
		GetVerbalName(), 
		Device_.GetVerbalName(), 
		pRightVector ? Device_.VariableNameByPtr(pRightVector->pValue) : cszUnknown,
		pRightVector ? *pRightVector->pValue : 0.0);
}


bool CDynaPrimitive::CheckTimeConstant(const double& T)
{
	if (Equal(T, 0.0))
	{
		Device_.Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongPrimitiveTimeConstant,
			GetVerbalName(),
			Device_.GetVerbalName(),
			T));
		return false;
	}
	else
		return true;
}