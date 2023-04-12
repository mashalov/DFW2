#include "stdafx.h"
#include "Discontinuities.h"
#include "DynaModel.h"

using namespace DFW2;

void CModelAction::Log(CDynaModel* pDynaModel, std::string_view message)
{
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_MESSAGE, fmt::format("{} t={} : {}",
		CDFW2Messages::m_cszEvent,
		pDynaModel->GetCurrentTime(),
		message
	));
}



CStaticEvent::~CStaticEvent()
{
	for (auto&& it : Actions_)
		delete it;
}

CDiscontinuities::CDiscontinuities(CDynaModel *pDynaModel) : pDynaModel_(pDynaModel)
{
}

CStaticEvent::CStaticEvent(double Time) : Time_(Time)
{
	
}


bool CStaticEvent::AddAction(CModelAction* Action) const
{
	bool bRes{ true };
	Actions_.push_back(Action);
	return bRes;
}

bool CStaticEvent::ContainsStop() const
{
	bool bStopFound{ false };
	for (const auto& action : Actions_)
		if (action->Type() == eDFW2_ACTION_TYPE::AT_STOP)
		{
			bStopFound = true;
			break;
		}
	return bStopFound;
}

bool CStaticEvent::RemoveStateAction(CDiscreteDelay *pDelayObject) const
{
	bool bRes{ false };
	auto itFound{ Actions_.end() };
	for (auto it = Actions_.begin(); it != Actions_.end(); it++)
		if ((*it)->Type() == eDFW2_ACTION_TYPE::AT_STATE)
		{
			if (static_cast<CModelActionState*>(*it)->GetDelayObject() == pDelayObject)
			{
				delete *it;
				itFound = it;
				break;
			}
		}
	if (itFound != Actions_.end())
		Actions_.erase(itFound);
	return bRes;
}

eDFW2_ACTION_STATE CStaticEvent::DoActions(CDynaModel *pDynaModel) const
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_INACTIVE };

	// внутри ModelAction::Do может быть вызывано удаление ивента
	// которое вызовет сбой итерации. Поэтому мы копируем исходный список
	// во временный, итерации ведем по временному, а возможность удалять 
	// даем из основного списка ивентов

	MODELACTIONLIST tempList{ Actions_ };

	for (auto&& it : tempList)
	{
		if (State == eDFW2_ACTION_STATE::AS_ERROR)
			break;
		State = it->Do(pDynaModel);
	}
	return State;
}

bool CDiscontinuities::AddEvent(double Time, CModelAction* Action)
{
	bool bRes{ true };
	// событие может добавляться в отрицательное время,
	// но мы уводим его на начало расчета
	CStaticEvent newEvent((std::max)(Time, 0.0));
	auto checkInsert{ StaticEvent_.insert(newEvent) };
	checkInsert.first->AddAction(Action);
	return bRes;
}

bool CDiscontinuities::SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double Time)
{
	bool bRes{ true };
	// событие может добавляться в отрицательное время,
	// но мы уводим его на начало расчета
	Time = (std::max)(Time, 0.0);
	CStateObjectIdToTime newObject(pDelayObject, Time);
	auto it{ StateEvents_.find(pDelayObject) };
	if (it == StateEvents_.end())
	{
		StateEvents_.insert(newObject);
		AddEvent(Time, new CModelActionState(pDelayObject));
	}
	else
	{
		RemoveStateDiscontinuity(pDelayObject);
		AddEvent(Time, new CModelActionState(pDelayObject));
		StateEvents_.insert(pDelayObject);
	}
	return bRes;
}

bool CDiscontinuities::CheckStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	CStateObjectIdToTime newObject(pDelayObject, 0.0);
	auto it{ StateEvents_.find(newObject) };
	return it != StateEvents_.end();
}

bool CDiscontinuities::RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	bool bRes{ true };
	CStateObjectIdToTime newObject(pDelayObject, 0.0);
	// находим выдержку в условных событиях
	auto it{ StateEvents_.find(newObject) };
	if (it != StateEvents_.end())
	{
		// находим статические события для  времени условного события
		auto itEvent{ StaticEvent_.lower_bound(it->Time()) };
		if (itEvent != StaticEvent_.end())
		{
			itEvent->RemoveStateAction(pDelayObject);
			if (!itEvent->ActionsCount())
				StaticEvent_.erase(itEvent);
		}
		StateEvents_.erase(it);
	}
	return bRes;
}

void CDiscontinuities::Init()
{
}

double CDiscontinuities::NextEventTime()
{
	double dNextEventTime{ -1 };
	if (!StaticEvent_.empty())
		dNextEventTime = StaticEvent_.begin()->Time();

	/*if (m_itCurrentStaticEvent != m_StaticEvent.end())
	{
		dNextEventTime = m_itCurrentStaticEvent->Time();
	}*/
	return dNextEventTime;
}

void CDiscontinuities::PassTime(double Time)
{
	// событие может добавляться в отрицательное время,
	// но мы уводим его на начало расчета
	Time = (std::max)(Time, 0.0);
	if (!pDynaModel_->IsInDiscontinuityMode())
	{
		auto itEvent{ StaticEvent_.lower_bound(CStaticEvent(Time)) };
		if (itEvent != StaticEvent_.end())
		{
			if (itEvent != StaticEvent_.begin())
				StaticEvent_.erase(StaticEvent_.begin(), itEvent);
		}
	}
}


eDFW2_ACTION_STATE CDiscontinuities::ProcessStaticEvents()
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_INACTIVE };

	if (!StaticEvent_.empty())
	{
		// исполняем первое статическое событие
		State = StaticEvent_.begin()->DoActions(pDynaModel_);
	}
	return State;
}

CModelActionChangeVariable::CModelActionChangeVariable(double *pVariable, double TargetValue) : CModelAction(eDFW2_ACTION_TYPE::AT_CV),
																								TargetValue_(TargetValue),
																								pVariable_(pVariable)
{

}

eDFW2_ACTION_STATE CModelActionChangeVariable::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	*pVariable_ = TargetValue_;
	return State;
}


CModelActionStop::CModelActionStop() : CModelAction(eDFW2_ACTION_TYPE::AT_STOP)
{

}

eDFW2_ACTION_STATE CModelActionStop::Do(CDynaModel *pDynaModel) 
{ 
	pDynaModel->StopProcess();
	return eDFW2_ACTION_STATE::AS_DONE;
}

void CModelActionChangeBranchParameterBase::WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view Description)
{
	pDynaModel->WriteSlowVariable(pDynaBranch_->GetType(),
		{ pDynaBranch_->key.Ip, pDynaBranch_->key.Iq, pDynaBranch_->key.Np },
		VariableName,
		Value,
		PreviousValue,
		Description);
}

void CModelActionChangeNodeParameterBase::WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view Description)
{
	pDynaModel->WriteSlowVariable(pDynaNode_->GetType(),
		{ pDynaNode_->GetId() },
		VariableName,
		Value,
		PreviousValue,
		Description);
}


CModelActionChangeBranchState::CModelActionChangeBranchState(CDynaBranch *pBranch, enum CDynaBranch::BranchState NewState) :
																									CModelActionChangeBranchParameterBase(pBranch),
																									NewState_(NewState)
{
	

}

eDFW2_ACTION_STATE CModelActionChangeBranchImpedance::Do(CDynaModel* pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	Log(pDynaModel, fmt::format("{} Z={} -> Z={}",
		pDynaBranch_->GetVerbalName(),
		cplx(pDynaBranch_->R, pDynaBranch_->X),
		Impedance_
	));

	CDevice::FromComplex(pDynaBranch_->R, pDynaBranch_->X, Impedance_);
	pDynaBranch_->ProcessTopologyRequest();

	return State;
}

eDFW2_ACTION_STATE CModelActionChangeBranchR::Do(CDynaModel* pDynaModel, double R)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, "R", R, pDynaBranch_->R, pDynaBranch_->GetVerbalName());

	Log(pDynaModel, fmt::format("{} R={} -> R={}",
		pDynaBranch_->GetVerbalName(),
		pDynaBranch_->R,
		R
		));

	pDynaBranch_->R = R;
	pDynaBranch_->ProcessTopologyRequest();
	return State;

}

eDFW2_ACTION_STATE CModelActionChangeBranchX::Do(CDynaModel* pDynaModel, double X)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, "X", X, pDynaBranch_->X, pDynaBranch_->GetVerbalName());

	Log(pDynaModel, fmt::format("{} X={} -> X={}",
		pDynaBranch_->GetVerbalName(),
		pDynaBranch_->X,
		X
	));

	pDynaBranch_->X = X;
	pDynaBranch_->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeBranchB::Do(CDynaModel* pDynaModel, double B)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, "B", B, pDynaBranch_->B, pDynaBranch_->GetVerbalName());

	Log(pDynaModel, fmt::format("{} B={} -> B={}",
		pDynaBranch_->GetVerbalName(),
		pDynaBranch_->B,
		B
	));

	pDynaBranch_->B = B;
	pDynaBranch_->ProcessTopologyRequest();
	return State;
}



eDFW2_ACTION_STATE CModelActionChangeBranchState::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	if (!CDevice::IsFunctionStatusOK(pDynaBranch_->SetBranchState(NewState_, eDEVICESTATECAUSE::DSC_EXTERNAL)))
		State = eDFW2_ACTION_STATE::AS_ERROR;

	return State;
}

eDFW2_ACTION_STATE CModelActionChangeBranchState::Do(CDynaModel *pDynaModel, double Value)
{
	int nState{ static_cast<int>(Value) };
	CDynaBranch::BranchState variants[4] =
	{
		CDynaBranch::BranchState::BRANCH_OFF,
		CDynaBranch::BranchState::BRANCH_ON,
		CDynaBranch::BranchState::BRANCH_TRIPIP,
		CDynaBranch::BranchState::BRANCH_TRIPIQ
	};

	if (nState >= 0 && nState <= 3)
		NewState_ = variants[nState];
	else
	if (nState > 0)
		NewState_ = CDynaBranch::BranchState::BRANCH_ON;
	else
		NewState_ = CDynaBranch::BranchState::BRANCH_OFF;

	WriteSlowVariable(pDynaModel, 
		CDevice::m_cszState, 
		Value, 
		static_cast<double>(pDynaBranch_->BranchState_), 
		pDynaBranch_->GetVerbalName());

	Log(pDynaModel, fmt::format("{} {}=\"{}\" -> {}=\"{}\"",
			pDynaBranch_->GetVerbalName(),
			CDevice::m_cszState,
			stringutils::enum_text(pDynaBranch_->BranchState_, CDynaBranch::m_cszBranchStateNames),
			CDevice::m_cszState,
			stringutils::enum_text(NewState_, CDynaBranch::m_cszBranchStateNames)));

	return Do(pDynaModel);
}



CModelActionChangeNodeShunt::CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX) : CModelActionChangeNodeParameterBase(pNode),
												 												  ShuntRX_(ShuntRX)
{
	
}

eDFW2_ACTION_STATE CModelActionChangeNodeShunt::Do(CDynaModel* pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	const cplx rx{ 1.0 / cplx(pDynaNode_->Gshunt, pDynaNode_->Bshunt) };
	if(CModelAction::isfinite(rx))
		Log(pDynaModel, fmt::format("{} Z={} -> Z={}",
			pDynaNode_->GetVerbalName(),
			rx,
			ShuntRX_
		));
	else
		Log(pDynaModel, fmt::format("{} Z={}",
			pDynaNode_->GetVerbalName(),
			ShuntRX_
		));

	cplx y = 1.0 / ShuntRX_;

	CDevice::FromComplex(pDynaNode_->Gshunt, pDynaNode_->Bshunt,y);

	pDynaNode_->ProcessTopologyRequest();
	return State;
}

CModelActionChangeNodePQLoad::CModelActionChangeNodePQLoad(CDynaNode* pNode, double Pload) : CModelActionChangeNodeParameterBase(pNode),
																						Pload_(Pload),
																						InitialLoad_{pNode->Pn, pNode->Qn}
{

}

eDFW2_ACTION_STATE CModelActionChangeNodePQLoad::Do(CDynaModel* pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	const cplx Sload{ pDynaNode_->Pn, pDynaNode_->Qn };
	double newQ{ Sload.imag() };
	if (!Equal(InitialLoad_.real(), 0.0))
		newQ = InitialLoad_.imag() / InitialLoad_.real() * Value;
	const cplx SloadNew{ Value, newQ };

	Log(pDynaModel, fmt::format("{} Sload={} -> Sload={}",
		pDynaNode_->GetVerbalName(),
		Sload,
		SloadNew
	));

	WriteSlowVariable(pDynaModel, CDynaNodeBase::m_cszPload, SloadNew.real(), Sload.real(), pDynaNode_->GetVerbalName());
	WriteSlowVariable(pDynaModel, CDynaNodeBase::m_cszQload, SloadNew.imag(), Sload.imag(), pDynaNode_->GetVerbalName());

	CDevice::FromComplex(pDynaNode_->Pn, pDynaNode_->Qn, SloadNew);

	// В RUSTab изменение параметра оказывается кумулятивным - последовательные изменения суммируются через V 
	// для формул вида V - k * Base. В этом проекте V фиксированное и не изменяется до обработки разрыва.
	// Новое значение получается после решения начальных условий. 
	// Для того чтобы сэмулировать поведение RUSTab можно использовать глобальную обработку разрыва
	// после каждого изменения. При этом изменяется и нагрузка узла от блока измерения, и значение действия по формуле выше.
	// Работает за счет того, что нагрузка рассчитывается на CDynaNodeMeasure::ProcessDiscontinuuity,
	// а далее вызывается ProcessDiscontinuity в пользовательской модели автоматики/сценария.
	// В результате Value в параметрах функции вычисляется еще раз с только что поставленной нагрузкой
	// Медленно, но зато просто
	if(pDynaModel->ChangeActionsAreCumulative())
		pDynaModel->ProcessDiscontinuity();
	pDynaNode_->ProcessTopologyRequest();
	return State;
}


eDFW2_ACTION_STATE CModelActionChangeNodeShuntR::Do(CDynaModel *pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	cplx rx{ 1.0 / cplx(pDynaNode_->Gshunt, pDynaNode_->Bshunt) };

	if(CModelAction::isfinite(rx))
		Log(pDynaModel, fmt::format("{} R={} -> R={}",
				pDynaNode_->GetVerbalName(),
				rx.real(),
				Value
		));
	else
	{
		rx = {};
		Log(pDynaModel, fmt::format("{} R={}",
			pDynaNode_->GetVerbalName(),
			Value
		));
	}

	WriteSlowVariable(pDynaModel, "R", Value, rx.real(), pDynaNode_->GetVerbalName());

	rx.real(Value);
	rx = 1.0 / rx;

	CDevice::FromComplex(pDynaNode_->Gshunt, pDynaNode_->Bshunt, rx);
	pDynaNode_->ProcessTopologyRequest();

	return State;
}


eDFW2_ACTION_STATE CModelActionChangeNodeShuntX::Do(CDynaModel *pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	cplx rx{ 1.0 / cplx(pDynaNode_->Gshunt, pDynaNode_->Bshunt) };
	if (CModelAction::isfinite(rx))
		Log(pDynaModel, fmt::format("{} X={} -> X={}",
			pDynaNode_->GetVerbalName(),
			rx.imag(),
			Value
		));
	else
	{
		rx = {};
		Log(pDynaModel, fmt::format("{} X={}",
			pDynaNode_->GetVerbalName(),
			Value
		));
	}

	WriteSlowVariable(pDynaModel, "X", Value, rx.real(), pDynaNode_->GetVerbalName());

	rx.imag(Value);
	rx = 1.0 / rx;

	CDevice::FromComplex(pDynaNode_->Gshunt, pDynaNode_->Bshunt, rx);

	pDynaNode_->ProcessTopologyRequest();
	return State;
}



eDFW2_ACTION_STATE CModelActionChangeNodeShuntAdmittance::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	const cplx y(pDynaNode_->Gshunt, pDynaNode_->Bshunt);
	if (CModelAction::isfinite(y))
		Log(pDynaModel, fmt::format("{} Y={} -> Y={}",
			pDynaNode_->GetVerbalName(),
			y,
			ShuntGB_));
	else
		Log(pDynaModel, fmt::format("{} Y={}",
			pDynaNode_->GetVerbalName(),
			ShuntGB_));

	CDevice::FromComplex(pDynaNode_->Gshunt, pDynaNode_->Bshunt, ShuntGB_);

	pDynaNode_->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntG::Do(CDynaModel *pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, "G", Value, pDynaNode_->G, pDynaNode_->GetVerbalName());

	if(std::isfinite(pDynaNode_->Gshunt))
		Log(pDynaModel, fmt::format("{} G={} -> G={}",
			pDynaNode_->GetVerbalName(),
			pDynaNode_->Gshunt,
			Value
		));
	else
		Log(pDynaModel, fmt::format("{} G={}",
			pDynaNode_->GetVerbalName(),
			Value
		));

	pDynaNode_->Gshunt = Value;
	pDynaNode_->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntB::Do(CDynaModel *pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, "B", Value, pDynaNode_->B, pDynaNode_->GetVerbalName());

	if (std::isfinite(pDynaNode_->Bshunt))
		Log(pDynaModel, fmt::format("{} B={} -> B={}",
			pDynaNode_->GetVerbalName(),
			pDynaNode_->Bshunt,
			Value
		));
	else
		Log(pDynaModel, fmt::format("{} B={}",
			pDynaNode_->GetVerbalName(),
			Value
		));

	pDynaNode_->Bshunt = Value;
	pDynaNode_->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionRemoveNodeShunt::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	const cplx y(pDynaNode_->Gshunt, pDynaNode_->Bshunt);
	if(CModelAction::isfinite(y))
		Log(pDynaModel, fmt::format("{} Y={} -> Y=0.0",
			pDynaNode_->GetVerbalName(),
			cplx(pDynaNode_->Gshunt, pDynaNode_->Bshunt)));
	else
		Log(pDynaModel, fmt::format("{} Y=0.0",
			pDynaNode_->GetVerbalName()));

	pDynaNode_->Gshunt = pDynaNode_->Bshunt = 0.0;
	pDynaNode_->ProcessTopologyRequest();
	return State;
}


CModelActionChangeNodeLoad::CModelActionChangeNodeLoad(CDynaNode *pNode, cplx& LoadPower) : CModelActionChangeVariable(nullptr, 0),
																						    pDynaNode_(pNode),
																							newLoad_(LoadPower)
{
	
}

eDFW2_ACTION_STATE CModelActionChangeNodeLoad::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	CDevice::FromComplex(pDynaNode_->Pn, pDynaNode_->Qn, newLoad_);

	pDynaNode_->ProcessTopologyRequest();
	return State;
}


CModelActionState::CModelActionState(CDiscreteDelay *pDiscreteDelay) : CModelAction(eDFW2_ACTION_TYPE::AT_STATE),
																	   pDiscreteDelay_(pDiscreteDelay)
{

}


eDFW2_ACTION_STATE CModelActionState::Do(CDynaModel *pDynaModel)
{ 
	CDevice* device{ pDiscreteDelay_->GetDevice() };

	if (!device)
		device = pDynaModel->AutomaticDevice.GetDeviceByIndex(0);

	pDiscreteDelay_->NotifyDelay(pDynaModel);
	pDynaModel->DiscontinuityRequest(*device, DiscontinuityLevel::Hard);
	return eDFW2_ACTION_STATE::AS_INACTIVE;
}


CModelActionChangeDeviceState::CModelActionChangeDeviceState(CDevice* pDevice, eDEVICESTATE NewState) : CModelActionChangeVariable(nullptr,0),
																							    pDevice_(pDevice),
																								NewState_(NewState)
{

}

eDFW2_ACTION_STATE CModelActionChangeDeviceState::Do(CDynaModel* pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	if (!CDevice::IsFunctionStatusOK(pDevice_->ChangeState(NewState_, eDEVICESTATECAUSE::DSC_EXTERNAL)))
		State = eDFW2_ACTION_STATE::AS_ERROR;
	else
		pDevice_->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeDeviceState::Do(CDynaModel* pDynaModel, double Value)
{
	int nState{ static_cast<int>(Value) };

	if (nState > 0)
		NewState_ = eDEVICESTATE::DS_ON;
	else
		NewState_ = eDEVICESTATE::DS_OFF;

	pDynaModel->WriteSlowVariable(pDevice_->GetType(),
								{ pDevice_->GetId() },
								CDevice::m_cszState,
								static_cast<double>(NewState_),
								static_cast<double>(pDevice_->GetState()),
								pDevice_->GetVerbalName());

	Log(pDynaModel, fmt::format("{} {}=\"{}\" -> {}=\"{}\"",
		pDevice_->GetVerbalName(),
		CDevice::m_cszState,
		stringutils::enum_text(pDevice_->GetState(), CDevice::m_cszStates),
		CDevice::m_cszState,
		stringutils::enum_text(NewState_, CDevice::m_cszStates)));

	return Do(pDynaModel);
}


