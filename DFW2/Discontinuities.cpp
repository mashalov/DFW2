#include "stdafx.h"
#include "Discontinuities.h"
#include "DynaModel.h"

using namespace DFW2;

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
	for (MODELACTIONITR it = m_Actions.begin(); it != m_Actions.end(); it++)
		if ((*it)->Type() == eDFW2_ACTION_TYPE::AT_STOP)
		{
			bStopFound = true;
			break;
		}
	return bStopFound;
}

bool CStaticEvent::RemoveStateAction(CDiscreteDelay *pDelayObject) const
{
	bool bRes = false;
	MODELACTIONITR itFound = m_Actions.end();
	for (MODELACTIONITR it = m_Actions.begin(); it != m_Actions.end(); it++)
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

	for (MODELACTIONITR it = tempList.begin(); it != tempList.end() && State != eDFW2_ACTION_STATE::AS_ERROR; it++)
	{
		State = (*it)->Do(pDynaModel);
	}
	return State;
}

bool CDiscontinuities::AddEvent(double dTime, CModelAction* Action)
{
	bool bRes = true;
	CStaticEvent newEvent(dTime);
	std::pair<STATICEVENTITR, bool> checkInsert = m_StaticEvent.insert(newEvent);
	checkInsert.first->AddAction(Action);
	return bRes;
}

bool CDiscontinuities::SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dTime)
{
	bool bRes = true;
	CStateObjectIdToTime newObject(pDelayObject, dTime);
	STATEEVENTITR it = m_StateEvents.find(pDelayObject);
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
	STATEEVENTITR it = m_StateEvents.find(newObject);
	return it != m_StateEvents.end();
}

bool CDiscontinuities::RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	bool bRes = true;
	CStateObjectIdToTime newObject(pDelayObject, 0.0);
	STATEEVENTITR it = m_StateEvents.find(newObject);
	if (it != m_StateEvents.end())
	{
		STATICEVENTITR itEvent = m_StaticEvent.lower_bound(it->Time());
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
		STATICEVENTITR itEvent = m_StaticEvent.lower_bound(CStaticEvent(dTime));
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


CModelActionChangeBranchParameterBase::CModelActionChangeBranchParameterBase(CDynaBranch* pBranch) : CModelActionChangeVariable(nullptr, 0.0),
																									 m_pDynaBranch(pBranch)
{

}

CModelActionChangeBranchState::CModelActionChangeBranchState(CDynaBranch *pBranch, enum CDynaBranch::BranchState NewState) :
																									CModelActionChangeBranchParameterBase(pBranch),
																									m_NewState(NewState)
{
	

}

CModelActionChangeBranchImpedance::CModelActionChangeBranchImpedance(CDynaBranch* pBranch, const cplx& Impedance) :
	CModelActionChangeBranchParameterBase(pBranch),
	m_Impedance(Impedance)
{

}
eDFW2_ACTION_STATE CModelActionChangeBranchImpedance::Do(CDynaModel* pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);
	m_pDynaBranch->R = m_Impedance.real();
	m_pDynaBranch->X = m_Impedance.imag();
	pDynaModel->ProcessTopologyRequest();
	return State;
}


CModelActionChangeBranchR::CModelActionChangeBranchR(CDynaBranch* pBranch, double R) :
	CModelActionChangeBranchImpedance(pBranch, { R, pBranch->X })
{

}
eDFW2_ACTION_STATE CModelActionChangeBranchR::Do(CDynaModel* pDynaModel, double R)
{
	m_Impedance = { R, m_pDynaBranch->X };
	return CModelActionChangeBranchImpedance::Do(pDynaModel);
}

CModelActionChangeBranchX::CModelActionChangeBranchX(CDynaBranch* pBranch, double X) :
	CModelActionChangeBranchImpedance(pBranch, { pBranch->R, X })
{

}

eDFW2_ACTION_STATE CModelActionChangeBranchX::Do(CDynaModel* pDynaModel, double X)
{
	m_Impedance = { m_pDynaBranch->R, X };
	return CModelActionChangeBranchImpedance::Do(pDynaModel);
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

	return Do(pDynaModel);
}


CModelActionChangeNodeShunt::CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX) : CModelActionChangeNodeParameterBase(pNode),
																							m_ShuntRX(ShuntRX)
{
	
}

eDFW2_ACTION_STATE CModelActionChangeNodeShunt::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE:: AS_DONE);
	cplx y = 1.0 / m_ShuntRX;
	m_pDynaNode->Gshunt = y.real();
	m_pDynaNode->Bshunt = y.imag();
	m_pDynaNode->ProcessTopologyRequest();
	return State;
}


CModelActionChangeNodeShuntR::CModelActionChangeNodeShuntR(CDynaNode *pNode, double R) : CModelActionChangeNodeShunt(pNode,cplx(R,0.0))
{

}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntR::Do(CDynaModel *pDynaModel, double dValue)
{
	m_ShuntRX.real(dValue);
	return CModelActionChangeNodeShunt::Do(pDynaModel);
}


CModelActionChangeNodeShuntX::CModelActionChangeNodeShuntX(CDynaNode *pNode, double X) : CModelActionChangeNodeShunt(pNode, cplx(0.0, X))
{

}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntX::Do(CDynaModel *pDynaModel, double dValue)
{
	m_ShuntRX.imag(dValue);
	return CModelActionChangeNodeShunt::Do(pDynaModel);
}

CModelActionChangeNodeShuntAdmittance::CModelActionChangeNodeShuntAdmittance(CDynaNode *pNode, const cplx& ShuntGB) : CModelActionChangeNodeParameterBase(pNode),
																												m_ShuntGB(ShuntGB)
																												{

																												}
eDFW2_ACTION_STATE CModelActionChangeNodeShuntAdmittance::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);
	m_pDynaNode->Gshunt = m_ShuntGB.real();
	m_pDynaNode->Bshunt = m_ShuntGB.imag();
	pDynaModel->ProcessTopologyRequest();
	return State;
}


CModelActionChangeNodeShuntG::CModelActionChangeNodeShuntG(CDynaNode *pNode, double G) : CModelActionChangeNodeShuntAdmittance(pNode, cplx(G, 0.0))
{

}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntG::Do(CDynaModel *pDynaModel, double dValue)
{
	m_ShuntGB.real(dValue);
	return CModelActionChangeNodeShuntAdmittance::Do(pDynaModel);
}

CModelActionChangeNodeShuntB::CModelActionChangeNodeShuntB(CDynaNode *pNode, double B) : CModelActionChangeNodeShuntAdmittance(pNode, cplx(0.0, B))
{

}

eDFW2_ACTION_STATE CModelActionChangeNodeShuntB::Do(CDynaModel *pDynaModel, double dValue)
{
	m_ShuntGB.imag(dValue);
	return CModelActionChangeNodeShuntAdmittance::Do(pDynaModel);
}



CModelActionRemoveNodeShunt::CModelActionRemoveNodeShunt(CDynaNode *pNode) : CModelActionChangeNodeParameterBase(pNode)
{

}

eDFW2_ACTION_STATE CModelActionRemoveNodeShunt::Do(CDynaModel *pDynaModel)
{
	eDFW2_ACTION_STATE State(eDFW2_ACTION_STATE::AS_DONE);
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
	m_pDynaNode->Pn = m_newLoad.real();
	m_pDynaNode->Qn = m_newLoad.imag();
	m_pDynaNode->ProcessTopologyRequest();
	return State;
}


CModelActionState::CModelActionState(CDiscreteDelay *pDiscreteDelay) : CModelAction(eDFW2_ACTION_TYPE::AT_STATE),
																	   m_pDiscreteDelay(pDiscreteDelay)
{

}


eDFW2_ACTION_STATE CModelActionState::Do(CDynaModel *pDynaModel)
{ 
	m_pDiscreteDelay->NotifyDelay(pDynaModel);
	pDynaModel->DiscontinuityRequest();
	return eDFW2_ACTION_STATE::AS_INACTIVE;
}