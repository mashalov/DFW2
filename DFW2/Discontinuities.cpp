#include "stdafx.h"
#include "Discontinuities.h"
#include "DynaModel.h"

using namespace DFW2;

void CModelAction::Log(const CDynaModel* pDynaModel, std::string_view message)
{
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_MESSAGE, fmt::format("{} t={} : {}",
		CDFW2Messages::m_cszEvent,
		pDynaModel->GetCurrentIntegrationTime(),
		message
	));
}

bool CStaticEvent::AddAction(ModelActionT&& Action) const
{
	bool bRes{ true };
	Actions_.emplace_back(std::move(Action));
	return bRes;
}

bool CStaticEvent::RemoveStateAction(CDiscreteDelay *pDelayObject) const
{
	bool bRes{ false };
	auto itFound{ std::find_if(Actions_.begin(),
		Actions_.end(),
		[&pDelayObject](const ModelActionT& Action) -> bool
		{
			return Action->Type() == eDFW2_ACTION_TYPE::AT_STATE && 
				static_cast<CModelActionState*>(Action.get())->GetDelayObject() == pDelayObject;
		}) };

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

	std::list<CModelAction*> tempList;

	std::transform(Actions_.begin(),
		Actions_.end(), 
		std::back_inserter(tempList), 
		[](const ModelActionT& Action) -> CModelAction*
		{ 
			return Action.get(); 
		});

	for (auto&& it : tempList)
	{
		if (State == eDFW2_ACTION_STATE::AS_ERROR)
			break;
		State = it->Do(pDynaModel);
	}
	return State;
}

bool CDiscontinuities::AddEvent(double Time, ModelActionT&& Action)
{
	bool bRes{ true };
	// событие может добавляться в отрицательное время,
	// но мы уводим его на начало расчета
	auto checkInsert{ StaticEvent_.emplace((std::max)(Time, 0.0)) };
	checkInsert.first->AddAction(std::move(Action));
	return bRes;
}

bool CDiscontinuities::SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double Time)
{
	bool bRes{ true };
	// событие может добавляться в отрицательное время,
	// но мы уводим его на начало расчета
	Time = (std::max)(Time, 0.0);
	CStateObjectIdToTime newObject(pDelayObject, Time);
	if (auto it{ StateEvents_.find(pDelayObject) }; it == StateEvents_.end())
	{
		StateEvents_.insert(newObject);
		AddEvent(Time, std::make_unique<CModelActionState>(pDelayObject));
	}
	else
	{
		RemoveStateDiscontinuity(pDelayObject);
		AddEvent(Time, std::make_unique<CModelActionState>(pDelayObject));
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
	return dNextEventTime;
}

void CDiscontinuities::PassTime(double Time)
{
	// событие может добавляться в отрицательное время,
	// но мы уводим его на начало расчета
	Time = (std::max)(Time, 0.0);
	if (!pDynaModel_->IsInDiscontinuityMode())
	{
		// для поиска используем заранее созданный CStaticEventSearch
		// вместо создания CStaticEvent для каждого поиска
		auto itEvent{ StaticEvent_.lower_bound(TimeSearch_.Time(Time)) };
		if (itEvent != StaticEvent_.end() && itEvent != StaticEvent_.begin())
				StaticEvent_.erase(StaticEvent_.begin(), itEvent);
	}
}

size_t CDiscontinuities::EventsLeft(double Time) const 
{ 
	// считаем количество статических событий и событий состояния со временем, превышающим заденное 
	// для поиска используем заранее созданный CStaticEventSearch
	// вместо создания CStaticEvent для каждого поиска
	return std::distance(std::upper_bound(StaticEvent_.begin(), StaticEvent_.end(), TimeSearch_.Time(Time)), StaticEvent_.end()) +
		std::count_if(StateEvents_.begin(), StateEvents_.end(), [Time](const auto& evt) {
			return evt.Time() > Time;
			});
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

eDFW2_ACTION_STATE CModelActionStop::Do(CDynaModel *pDynaModel) 
{ 
	pDynaModel->StopProcess();
	return eDFW2_ACTION_STATE::AS_DONE;
}

void CModelActionChangeDeviceParameter::WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view Description)
{
	pDynaModel->WriteSlowVariable(pDevice_->GetType(),
		{ pDevice_->GetId() },
		VariableName,
		Value,
		PreviousValue,
		Description);
}

void CModelActionChangeBranchParameterBase::WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view Description)
{
	pDynaModel->WriteSlowVariable(Branch()->GetType(),
		{ Branch()->key.Ip, Branch()->key.Iq, Branch()->key.Np },
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
	Log(pDynaModel, CModelActionChangeBranchImpedance::cszz_, cplx(Branch()->R, Branch()->X), Impedance_);
	CDevice::FromComplex(Branch()->R, Branch()->X, Impedance_);
	Branch()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeBranchR::Do(CDynaModel* pDynaModel, double R)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszR_, R, Branch()->R, Branch()->GetVerbalName());
	Log(pDynaModel, CDynaNodeBase::cszR_, Branch()->R, R);
	Branch()->R = R;
	Branch()->ProcessTopologyRequest();
	return State;

}

eDFW2_ACTION_STATE CModelActionChangeBranchX::Do(CDynaModel* pDynaModel, double X)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszX_, X, Branch()->X, Branch()->GetVerbalName());

	Log(pDynaModel, CDynaNodeBase::cszX_,  Branch()->X, X);

	Branch()->X = X;
	Branch()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeBranchB::Do(CDynaModel* pDynaModel, double B)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszb_, B, Branch()->B, Branch()->GetVerbalName());
	Log(pDynaModel, CDynaNodeBase::cszb_, Branch()->B, B);
	Branch()->B = B;
	Branch()->ProcessTopologyRequest();
	return State;
}



eDFW2_ACTION_STATE CModelActionChangeBranchState::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	if (!CDevice::IsFunctionStatusOK(Branch()->SetBranchState(NewState_, eDEVICESTATECAUSE::DSC_EXTERNAL)))
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
		CDevice::cszState_, 
		Value, 
		static_cast<double>(Branch()->BranchState_),
		Branch()->GetVerbalName());

	CModelAction::Log(pDynaModel, fmt::format("{} {}=\"{}\" -> {}=\"{}\"",
			Branch()->GetVerbalName(),
			CDevice::cszState_,
			stringutils::enum_text(Branch()->BranchState_, CDynaBranch::cszBranchStateNames_),
			CDevice::cszState_,
			stringutils::enum_text(NewState_, CDynaBranch::cszBranchStateNames_)));

	return Do(pDynaModel);
}



CModelActionChangeNodeShunt::CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX) : CModelActionChangeNodeParameterBase(pNode),
												 												  ShuntRX_(ShuntRX)
{
	
}

eDFW2_ACTION_STATE CModelActionChangeNodeShunt::Do(CDynaModel* pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	const cplx rx{ 1.0 / cplx(Node()->Gshunt, Node()->Bshunt) };
	if(CModelAction::isfinite(rx))
		Log(pDynaModel, CModelActionChangeBranchImpedance::cszz_, rx, ShuntRX_);
	else
		Log(pDynaModel, CModelActionChangeBranchImpedance::cszz_, ShuntRX_);

	cplx y = 1.0 / ShuntRX_;
	CDevice::FromComplex(Node()->Gshunt, Node()->Bshunt, y);
	Node()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeQload::Do(CDynaModel* pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	const cplx Sload{ Node()->Pn, Node()->Qn };
	const cplx SloadNew{ Sload.real(), Value };
	Log(pDynaModel, Sload, SloadNew);
	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszQload_, SloadNew.imag(), Sload.imag(), Node()->GetVerbalName());
	Node()->Qn = SloadNew.imag();
	if (pDynaModel->ChangeActionsAreCumulative())
		pDynaModel->ProcessDiscontinuity();
	Node()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodePload::Do(CDynaModel* pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	const cplx Sload{ Node()->Pn, Node()->Qn};
	const cplx SloadNew{ Value, Sload.imag() };
	Log(pDynaModel, Sload, SloadNew);
	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszPload_, SloadNew.real(), Sload.real(), Node()->GetVerbalName());
	Node()->Pn = SloadNew.real();
	if (pDynaModel->ChangeActionsAreCumulative())
		pDynaModel->ProcessDiscontinuity();
	Node()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodePQload::Do(CDynaModel* pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	const cplx Sload{ Node()->Pn, Node()->Qn};
	double newQ{ Sload.imag() };
	if (!Consts::Equal(InitialLoad_.real(), 0.0))
		newQ = InitialLoad_.imag() / InitialLoad_.real() * Value;
	const cplx SloadNew{ Value, newQ };

	Log(pDynaModel, Sload, SloadNew);

	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszPload_, SloadNew.real(), Sload.real(), Node()->GetVerbalName());
	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszQload_, SloadNew.imag(), Sload.imag(), Node()->GetVerbalName());

	CDevice::FromComplex(Node()->Pn, Node()->Qn, SloadNew);

	// В RUSTab изменение параметра оказывается кумулятивным - последовательные изменения суммируются через V 
	// для формул вида V - k * Base. В этом проекте V фиксированное и не изменяется до обработки разрыва.
	// Новое значение получается после решения начальных условий. 
	// Для того чтобы сэмулировать поведение RUSTab можно использовать глобальную обработку разрыва
	// после каждого изменения. При этом изменяется и нагрузка узла от блока измерения, и значение действия по формуле выше.
	// Работает за счет того, что нагрузка рассчитывается на CDynaNodeMeasure::ProcessDiscontinuity,
	// а далее вызывается ProcessDiscontinuity в пользовательской модели автоматики/сценария.
	// В результате Value в параметрах функции вычисляется еще раз с только что поставленной нагрузкой
	// Медленно, но зато просто
	if(pDynaModel->ChangeActionsAreCumulative())
		pDynaModel->ProcessDiscontinuity();
	Node()->ProcessTopologyRequest();
	return State;
}


eDFW2_ACTION_STATE CModelActionChangeNodeShuntR::Do(CDynaModel *pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	cplx rx{ 1.0 / cplx(Node()->Gshunt, Node()->Bshunt) };

	if(CModelAction::isfinite(rx))
		Log(pDynaModel, CDynaNodeBase::cszR_, rx.real(), Value);
	else
	{
		rx = {};
		Log(pDynaModel, CDynaNodeBase::cszR_, Value);
	}

	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszR_, Value, rx.real(), Node()->GetVerbalName());

	rx.real(Value);
	rx = 1.0 / rx;
	CDevice::FromComplex(Node()->Gshunt, Node()->Bshunt, rx);
	Node()->ProcessTopologyRequest();
	return State;
}


eDFW2_ACTION_STATE CModelActionChangeNodeShuntX::Do(CDynaModel *pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	cplx rx{ 1.0 / cplx(Node()->Gshunt, Node()->Bshunt)};
	if (CModelAction::isfinite(rx))
		Log(pDynaModel, CDynaNodeBase::cszX_, rx.imag(), Value);
	else
	{
		rx = {};
		Log(pDynaModel, CDynaNodeBase::cszX_, Value);
	}

	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszX_, Value, rx.real(), Node()->GetVerbalName());

	rx.imag(Value);
	rx = 1.0 / rx;
	CDevice::FromComplex(Node()->Gshunt, Node()->Bshunt, rx);
	Node()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntToUscUref::Do(CDynaModel* pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	CModelAction::Log(pDynaModel, fmt::format(CDFW2Messages::m_cszNodeShortCircuitToUscUref, Node()->GetVerbalName(), Value));
	pDynaModel->AddShortCircuitNode(Node(), {Value, {}});
	Node()->ProcessTopologyRequest();
	return State;
}


eDFW2_ACTION_STATE CModelActionChangeNodeShuntToUscRX::Do(CDynaModel* pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	CModelAction::Log(pDynaModel, fmt::format(CDFW2Messages::m_cszNodeShortCircuitToUscRX, Node()->GetVerbalName(), Value));
	pDynaModel->AddShortCircuitNode(Node(), {{}, RXratio_});
	Node()->ProcessTopologyRequest();
	return State;
}


eDFW2_ACTION_STATE CModelActionChangeNodeShuntAdmittance::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };
	const cplx y(Node()->Gshunt, Node()->Bshunt);
	if (CModelAction::isfinite(y))
		Log(pDynaModel, CModelActionChangeNodeShuntAdmittance::cszy_, y, ShuntGB_);
	else
		Log(pDynaModel, CModelActionChangeNodeShuntAdmittance::cszy_,  ShuntGB_);

	CDevice::FromComplex(Node()->Gshunt, Node()->Bshunt, ShuntGB_);
	Node()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntG::Do(CDynaModel *pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszg_, Value, Node()->G, Node()->GetVerbalName());

	if(std::isfinite(Node()->Gshunt))
		Log(pDynaModel, CDynaNodeBase::cszg_, Node()->Gshunt * 1e6, Value);
	else
		Log(pDynaModel, CDynaNodeBase::cszg_,  Value);

	Node()->Gshunt = Value * 1e-6;
	Node()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntB::Do(CDynaModel *pDynaModel, double Value)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	WriteSlowVariable(pDynaModel, CDynaNodeBase::cszb_, Value, Node()->B, Node()->GetVerbalName());

	if (std::isfinite(Node()->Bshunt))
		Log(pDynaModel, CDynaNodeBase::cszb_, Node()->Bshunt * 1e6, Value);
	else
		Log(pDynaModel, CDynaNodeBase::cszb_, Value);

	Node()->Bshunt = -Value * 1e-6;
	Node()->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionRemoveNodeShunt::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State{ eDFW2_ACTION_STATE::AS_DONE };

	const cplx y(Node()->Gshunt, Node()->Bshunt);
	if(CModelAction::isfinite(y))
		Log(pDynaModel, CModelActionChangeNodeShuntAdmittance::cszy_, cplx(Node()->Gshunt, Node()->Bshunt), cplx(0,0) );
	else
		Log(pDynaModel, CModelActionChangeNodeShuntAdmittance::cszy_, cplx(0,0) );

	Node()->Gshunt = Node()->Bshunt = 0.0;
	Node()->ProcessTopologyRequest();
	return State;
}


CModelActionState::CModelActionState(CDiscreteDelay *pDiscreteDelay) : CModelAction(eDFW2_ACTION_TYPE::AT_STATE),
																	   pDiscreteDelay_(pDiscreteDelay)
{

}


eDFW2_ACTION_STATE CModelActionState::Do(CDynaModel *pDynaModel)
{ 
	CDynaPrimitive& Primitive{ pDiscreteDelay_->Primitive() };
	pDiscreteDelay_->NotifyDelay(pDynaModel);
	pDynaModel->DiscontinuityRequest(Primitive, DiscontinuityLevel::Hard);
	return eDFW2_ACTION_STATE::AS_INACTIVE;
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
								CDevice::cszState_,
								static_cast<double>(NewState_),
								static_cast<double>(pDevice_->GetState()),
								pDevice_->GetVerbalName());

	CModelAction::Log(pDynaModel, fmt::format("{} {}=\"{}\" -> {}=\"{}\"",
		pDevice_->GetVerbalName(),
		CDevice::cszState_,
		stringutils::enum_text(pDevice_->GetState(), CDevice::cszStates_),
		CDevice::cszState_,
		stringutils::enum_text(NewState_, CDevice::cszStates_)));

	return Do(pDynaModel);
}


