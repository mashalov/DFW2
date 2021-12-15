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
	for (auto&& it : m_Actions)
		delete it;
}

CDiscontinuities::CDiscontinuities(CDynaModel *pDynaModel) : m_pDynaModel(pDynaModel)
{
}

CStaticEvent::CStaticEvent(double dTime) : m_dTime(dTime)
{
	
}


bool CStaticEvent::AddAction(CModelAction* Action) const
{
	bool bRes = true;
	m_Actions.push_back(Action);
	return bRes;
}

bool CStaticEvent::ContainsStop() const
{
	bool bStopFound = false;
	for (const auto& action : m_Actions)
		if (action->Type() == eDFW2_ACTION_TYPE::AT_STOP)
		{
			bStopFound = true;
			break;
		}
	return bStopFound;
}

bool CStaticEvent::RemoveStateAction(CDiscreteDelay *pDelayObject) const
{
	bool bRes = false;
	auto itFound = m_Actions.end();
	for (auto it = m_Actions.begin(); it != m_Actions.end(); it++)
		if ((*it)->Type() == eDFW2_ACTION_TYPE::AT_STATE)
		{
			if (static_cast<CModelActionState*>(*it)->GetDelayObject() == pDelayObject)
			{
				delete *it;
				itFound = it;
				break;
			}
		}
	if (itFound != m_Actions.end())
		m_Actions.erase(itFound);
	return bRes;
}

eDFW2_ACTION_STATE CStaticEvent::DoActions(CDynaModel *pDynaModel) const
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_INACTIVE);

	// внутри ModelAction::Do может быть вызывано удаление ивента
	// которое вызовет сбой итерации. Поэтому мы копируем исходный список
	// во временный, итерации ведем по временному, а возможность удалять 
	// даем из основного списка ивентов

	MODELACTIONLIST tempList = m_Actions;

	for (auto&& it : tempList)
	{
		if (State == eDFW2_ACTION_STATE::AS_ERROR)
			break;
		State = it->Do(pDynaModel);
	}
	return State;
}

bool CDiscontinuities::AddEvent(double dTime, CModelAction* Action)
{
	bool bRes = true;
	CStaticEvent newEvent(dTime);
	auto checkInsert = m_StaticEvent.insert(newEvent);
	checkInsert.first->AddAction(Action);
	return bRes;
}

bool CDiscontinuities::SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dTime)
{
	bool bRes = true;
	CStateObjectIdToTime newObject(pDelayObject, dTime);
	auto it = m_StateEvents.find(pDelayObject);
	if (it == m_StateEvents.end())
	{
		m_StateEvents.insert(newObject);
		AddEvent(dTime, new CModelActionState(pDelayObject));
	}
	else
	{
		RemoveStateDiscontinuity(pDelayObject);
		AddEvent(dTime, new CModelActionState(pDelayObject));
		m_StateEvents.insert(pDelayObject);
	}

	return bRes;
}

bool CDiscontinuities::CheckStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	CStateObjectIdToTime newObject(pDelayObject, 0.0);
	auto it = m_StateEvents.find(newObject);
	return it != m_StateEvents.end();
}

bool CDiscontinuities::RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	bool bRes = true;
	CStateObjectIdToTime newObject(pDelayObject, 0.0);
	auto it = m_StateEvents.find(newObject);
	if (it != m_StateEvents.end())
	{
		auto itEvent = m_StaticEvent.lower_bound(it->Time());
		if (itEvent != m_StaticEvent.end())
		{
			itEvent->RemoveStateAction(pDelayObject);
			if (!itEvent->ActionsCount())
				m_StaticEvent.erase(itEvent);
		}
		m_StateEvents.erase(it);
	}
	return bRes;
}

void CDiscontinuities::Init()
{
}

double CDiscontinuities::NextEventTime()
{
	double dNextEventTime = -1;
	if (!m_StaticEvent.empty())
		dNextEventTime = m_StaticEvent.begin()->Time();

	/*if (m_itCurrentStaticEvent != m_StaticEvent.end())
	{
		dNextEventTime = m_itCurrentStaticEvent->Time();
	}*/
	return dNextEventTime;
}

void CDiscontinuities::PassTime(double dTime)
{
	if (!m_pDynaModel->IsInDiscontinuityMode())
	{
		auto itEvent = m_StaticEvent.lower_bound(CStaticEvent(dTime));
		if (itEvent != m_StaticEvent.end())
		{
			if (itEvent != m_StaticEvent.begin())
				m_StaticEvent.erase(m_StaticEvent.begin(), itEvent);
		}
	}
}


eDFW2_ACTION_STATE CDiscontinuities::ProcessStaticEvents()
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_INACTIVE);

	if (!m_StaticEvent.empty())
	{
		State = m_StaticEvent.begin()->DoActions(m_pDynaModel);
	}
	return State;
}

CModelActionChangeVariable::CModelActionChangeVariable(double *pVariable, double TargetValue) : CModelAction(eDFW2_ACTION_TYPE::AT_CV),
																								m_dTargetValue(TargetValue),
																								m_pVariable(pVariable)
{

}

eDFW2_ACTION_STATE CModelActionChangeVariable::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);
	*m_pVariable = m_dTargetValue;
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
	pDynaModel->WriteSlowVariable(m_pDynaBranch->GetType(),
		{ m_pDynaBranch->key.Ip, m_pDynaBranch->key.Iq, m_pDynaBranch->key.Np },
		VariableName,
		Value,
		PreviousValue,
		Description);
}

void CModelActionChangeNodeParameterBase::WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view Description)
{
	pDynaModel->WriteSlowVariable(m_pDynaNode->GetType(),
		{ m_pDynaNode->GetId() },
		VariableName,
		Value,
		PreviousValue,
		Description);
}


CModelActionChangeBranchState::CModelActionChangeBranchState(CDynaBranch *pBranch, enum CDynaBranch::BranchState NewState) :
																									CModelActionChangeBranchParameterBase(pBranch),
																									m_NewState(NewState)
{
	

}

eDFW2_ACTION_STATE CModelActionChangeBranchImpedance::Do(CDynaModel* pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);
	Log(pDynaModel, fmt::format("{} Z={} -> Z={}",
		m_pDynaBranch->GetVerbalName(),
		cplx(m_pDynaBranch->R, m_pDynaBranch->X),
		m_Impedance
	));

	CDevice::FromComplex(m_pDynaBranch->R, m_pDynaBranch->X, m_Impedance);
	
	pDynaModel->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeBranchR::Do(CDynaModel* pDynaModel, double R)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	pDynaModel->WriteSlowVariable(m_pDynaBranch->GetType(),
		{ m_pDynaBranch->key.Ip, m_pDynaBranch->key.Iq, m_pDynaBranch->key.Np },
		"R",
		m_pDynaBranch->R,
		R,
		"");

	Log(pDynaModel, fmt::format("{} R={} -> R={}",
		m_pDynaBranch->GetVerbalName(),
		m_pDynaBranch->R,
		R
		));

	m_pDynaBranch->R = R;
	pDynaModel->ProcessTopologyRequest();
	return State;

}

eDFW2_ACTION_STATE CModelActionChangeBranchX::Do(CDynaModel* pDynaModel, double X)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	pDynaModel->WriteSlowVariable(m_pDynaBranch->GetType(),
		{ m_pDynaBranch->key.Ip, m_pDynaBranch->key.Iq, m_pDynaBranch->key.Np },
		"X",
		m_pDynaBranch->X,
		X,
		"");

	Log(pDynaModel, fmt::format("{} X={} -> X={}",
		m_pDynaBranch->GetVerbalName(),
		m_pDynaBranch->X,
		X
	));

	m_pDynaBranch->X = X;
	pDynaModel->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeBranchB::Do(CDynaModel* pDynaModel, double B)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	pDynaModel->WriteSlowVariable(m_pDynaBranch->GetType(),
		{ m_pDynaBranch->key.Ip, m_pDynaBranch->key.Iq, m_pDynaBranch->key.Np },
		"B",
		m_pDynaBranch->B,
		B,
		"");

	Log(pDynaModel, fmt::format("{} X={} -> X={}",
		m_pDynaBranch->GetVerbalName(),
		m_pDynaBranch->B,
		B
	));

	m_pDynaBranch->B = B;
	pDynaModel->ProcessTopologyRequest();
	return State;
}



eDFW2_ACTION_STATE CModelActionChangeBranchState::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	if (!CDevice::IsFunctionStatusOK(m_pDynaBranch->SetBranchState(m_NewState, eDEVICESTATECAUSE::DSC_EXTERNAL)))
		State = eDFW2_ACTION_STATE::AS_ERROR;

	return State;
}

eDFW2_ACTION_STATE CModelActionChangeBranchState::Do(CDynaModel *pDynaModel, double dValue)
{
	int nState = static_cast<int>(dValue);
	CDynaBranch::BranchState variants[4] =
	{
		CDynaBranch::BranchState::BRANCH_OFF,
		CDynaBranch::BranchState::BRANCH_ON,
		CDynaBranch::BranchState::BRANCH_TRIPIP,
		CDynaBranch::BranchState::BRANCH_TRIPIQ
	};

	if (nState >= 0 && nState <= 3)
		m_NewState = variants[nState];
	else
	if (nState > 0)
		m_NewState = CDynaBranch::BranchState::BRANCH_ON;
	else
		m_NewState = CDynaBranch::BranchState::BRANCH_OFF;

	WriteSlowVariable(pDynaModel, "State", dValue, static_cast<double>(m_pDynaBranch->m_BranchState), m_pDynaBranch->GetVerbalName());

	Log(pDynaModel, fmt::format("{} State=\"{}\"->State=\"{}\"",
			m_pDynaBranch->GetVerbalName(),
			stringutils::enum_text(m_pDynaBranch->m_BranchState, CDynaBranch::m_cszBranchStateNames),
			stringutils::enum_text(m_NewState, CDynaBranch::m_cszBranchStateNames)));

	return Do(pDynaModel);
}



CModelActionChangeNodeShunt::CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX) : CModelActionChangeNodeParameterBase(pNode),
																							m_ShuntRX(ShuntRX)
{
	
}

eDFW2_ACTION_STATE CModelActionChangeNodeShunt::Do(CDynaModel* pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	const cplx rx = 1.0 / cplx(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt);
	if(CModelAction::isfinite(rx))
		Log(pDynaModel, fmt::format("{} Z={} -> Z={}",
			m_pDynaNode->GetVerbalName(),
			rx,
			m_ShuntRX
		));
	else
		Log(pDynaModel, fmt::format("{} Z={}",
			m_pDynaNode->GetVerbalName(),
			m_ShuntRX
		));

	cplx y = 1.0 / m_ShuntRX;

	CDevice::FromComplex(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt,y);

	m_pDynaNode->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntR::Do(CDynaModel *pDynaModel, double dValue)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);
	cplx rx = 1.0 / cplx(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt);

	if(CModelAction::isfinite(rx))
		Log(pDynaModel, fmt::format("{} R={} -> R={}",
				m_pDynaNode->GetVerbalName(),
				rx.real(),
				dValue
		));
	else
	{
		rx = {};
		Log(pDynaModel, fmt::format("{} R={}",
			m_pDynaNode->GetVerbalName(),
			dValue
		));
	}

	WriteSlowVariable(pDynaModel, "R", dValue, rx.real(), m_pDynaNode->GetVerbalName());

	rx.real(dValue);
	rx = 1.0 / rx;

	CDevice::FromComplex(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt, rx);

	pDynaModel->ProcessTopologyRequest();
	return State;
}


eDFW2_ACTION_STATE CModelActionChangeNodeShuntX::Do(CDynaModel *pDynaModel, double dValue)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);
	cplx rx = 1.0 / cplx(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt);
	if (CModelAction::isfinite(rx))
		Log(pDynaModel, fmt::format("{} X={} -> X={}",
			m_pDynaNode->GetVerbalName(),
			rx.imag(),
			dValue
		));
	else
	{
		rx = {};
		Log(pDynaModel, fmt::format("{} X={}",
			m_pDynaNode->GetVerbalName(),
			dValue
		));
	}

	WriteSlowVariable(pDynaModel, "X", dValue, rx.real(), m_pDynaNode->GetVerbalName());

	rx.imag(dValue);
	rx = 1.0 / rx;

	CDevice::FromComplex(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt, rx);

	pDynaModel->ProcessTopologyRequest();
	return State;
}



eDFW2_ACTION_STATE CModelActionChangeNodeShuntAdmittance::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);
	const cplx y(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt);
	if (CModelAction::isfinite(y))
		Log(pDynaModel, fmt::format("{} Y={} -> Y={}",
			m_pDynaNode->GetVerbalName(),
			y,
			m_ShuntGB));
	else
		Log(pDynaModel, fmt::format("{} Y={}",
			m_pDynaNode->GetVerbalName(),
			m_ShuntGB));

	CDevice::FromComplex(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt, m_ShuntGB);

	pDynaModel->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntG::Do(CDynaModel *pDynaModel, double dValue)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	WriteSlowVariable(pDynaModel, "G", dValue, m_pDynaNode->G, m_pDynaNode->GetVerbalName());

	if(std::isfinite(m_pDynaNode->Gshunt))
		Log(pDynaModel, fmt::format("{} G={} -> G={}",
			m_pDynaNode->GetVerbalName(),
			m_pDynaNode->Gshunt,
			dValue
		));
	else
		Log(pDynaModel, fmt::format("{} G={}",
			m_pDynaNode->GetVerbalName(),
			dValue
		));

	m_pDynaNode->Gshunt = dValue;
	pDynaModel->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntB::Do(CDynaModel *pDynaModel, double dValue)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	WriteSlowVariable(pDynaModel, "B", dValue, m_pDynaNode->B, m_pDynaNode->GetVerbalName());

	if (std::isfinite(m_pDynaNode->Bshunt))
		Log(pDynaModel, fmt::format("{} B={} -> B={}",
			m_pDynaNode->GetVerbalName(),
			m_pDynaNode->Bshunt,
			dValue
		));
	else
		Log(pDynaModel, fmt::format("{} B={}",
			m_pDynaNode->GetVerbalName(),
			dValue
		));

	m_pDynaNode->Bshunt = dValue;
	pDynaModel->ProcessTopologyRequest();
	return State;
}

eDFW2_ACTION_STATE CModelActionRemoveNodeShunt::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	const cplx y(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt);
	if(CModelAction::isfinite(y))
		Log(pDynaModel, fmt::format("{} Y={} -> Y=0.0",
			m_pDynaNode->GetVerbalName(),
			cplx(m_pDynaNode->Gshunt, m_pDynaNode->Bshunt)));
	else
		Log(pDynaModel, fmt::format("{} Y=0.0",
			m_pDynaNode->GetVerbalName()));

	m_pDynaNode->Gshunt = m_pDynaNode->Bshunt = 0.0;
	m_pDynaNode->ProcessTopologyRequest();
	return State;
}


CModelActionChangeNodeLoad::CModelActionChangeNodeLoad(CDynaNode *pNode, cplx& LoadPower) : CModelActionChangeVariable(nullptr, 0),
																						    m_pDynaNode(pNode),
																							m_newLoad(LoadPower)
{
	
}

eDFW2_ACTION_STATE CModelActionChangeNodeLoad::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);

	CDevice::FromComplex(m_pDynaNode->Pn, m_pDynaNode->Qn,m_newLoad);

	m_pDynaNode->ProcessTopologyRequest();
	return State;
}


CModelActionState::CModelActionState(CDiscreteDelay *pDiscreteDelay) : CModelAction(eDFW2_ACTION_TYPE::AT_STATE),
																	   m_pDiscreteDelay(pDiscreteDelay)
{

}


eDFW2_ACTION_STATE CModelActionState::Do(CDynaModel *pDynaModel)
{ 
	CDevice* device(m_pDiscreteDelay->GetDevice());

	if (!device)
		device = pDynaModel->AutomaticDevice.GetDeviceByIndex(0);

	m_pDiscreteDelay->NotifyDelay(pDynaModel);
	pDynaModel->DiscontinuityRequest(*device);
	return eDFW2_ACTION_STATE::AS_INACTIVE;
}