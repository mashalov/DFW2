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


CDiscontinuities::~CDiscontinuities()
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
		if ((*it)->Type() == AT_STOP)
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
		if ((*it)->Type() == AT_STATE)
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

DFW2_ACTION_STATE CStaticEvent::DoActions(CDynaModel *pDynaModel) const
{
	DFW2_ACTION_STATE State = AS_INACTIVE;

	// внутри ModelAction::Do может быть вызывано удаление ивента
	// которое вызовет сбой итерации. Поэтому мы копируем исходный список
	// во временный, итерации ведем по временному, а возможность удалять 
	// даем из основного списка ивентов

	MODELACTIONLIST tempList = m_Actions;

	for (MODELACTIONITR it = tempList.begin(); it != tempList.end() && State != AS_ERROR; it++)
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


DFW2_ACTION_STATE CDiscontinuities::ProcessStaticEvents()
{
	DFW2_ACTION_STATE State = AS_INACTIVE;

	if (!m_StaticEvent.empty())
	{
		State = m_StaticEvent.begin()->DoActions(m_pDynaModel);
	}
	return State;
}

CModelActionChangeVariable::CModelActionChangeVariable(double *pVariable, double TargetValue) : CModelAction(AT_CV),
																								m_dTargetValue(TargetValue),
																								m_pVariable(pVariable)
{

}

DFW2_ACTION_STATE CModelActionChangeVariable::Do(CDynaModel *pDynaModel)
{
	DFW2_ACTION_STATE State = AS_DONE;
	*m_pVariable = m_dTargetValue;
	return State;
}


CModelActionStop::CModelActionStop() : CModelAction(AT_STOP)
{

}

DFW2_ACTION_STATE CModelActionStop::Do(CDynaModel *pDynaModel) 
{ 
	pDynaModel->StopProcess();
	return AS_DONE; 
}


CModelActionChangeBranchState::CModelActionChangeBranchState(CDynaBranch *pBranch, enum CDynaBranch::BranchState NewState) :
																									CModelActionChangeVariable(NULL,0.0),
																									m_pDynaBranch(pBranch),
																									m_NewState(NewState)
{
	

}


DFW2_ACTION_STATE CModelActionChangeBranchState::Do(CDynaModel *pDynaModel)
{
	DFW2_ACTION_STATE State = AS_DONE;

	if (!CDevice::IsFunctionStatusOK(m_pDynaBranch->SetBranchState(m_NewState, eDEVICESTATECAUSE::DSC_EXTERNAL)))
		State = AS_ERROR;

	return State;
}

DFW2_ACTION_STATE CModelActionChangeBranchState::Do(CDynaModel *pDynaModel, double dValue)
{
	int nState = static_cast<int>(dValue);
	CDynaBranch::BranchState variants[4] =
	{
		CDynaBranch::BRANCH_OFF,
		CDynaBranch::BRANCH_ON,
		CDynaBranch::BRANCH_TRIPIP,
		CDynaBranch::BRANCH_TRIPIQ
	};

	if (nState >= 0 && nState <= 3)
		m_NewState = variants[nState];
	else
	if (nState > 0)
		m_NewState = CDynaBranch::BRANCH_ON;
	else
		m_NewState = CDynaBranch::BRANCH_OFF;

	return Do(pDynaModel);
}


CModelActionChangeNodeShunt::CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX) : CModelActionChangeNodeParameterBase(pNode),
																							m_ShuntRX(ShuntRX)
{
	
}

DFW2_ACTION_STATE CModelActionChangeNodeShunt::Do(CDynaModel *pDynaModel)
{
	DFW2_ACTION_STATE State = AS_DONE;
	cplx y = 1.0 / m_ShuntRX;
	m_pDynaNode->Gshunt = y.real();
	m_pDynaNode->Bshunt = y.imag();
	m_pDynaNode->ProcessTopologyRequest();
	return State;
}


CModelActionChangeNodeShuntR::CModelActionChangeNodeShuntR(CDynaNode *pNode, double R) : CModelActionChangeNodeShunt(pNode,cplx(R,0.0))
{

}

DFW2_ACTION_STATE CModelActionChangeNodeShuntR::Do(CDynaModel *pDynaModel, double dValue)
{
	m_ShuntRX.real(dValue);
	return CModelActionChangeNodeShunt::Do(pDynaModel);
}


CModelActionChangeNodeShuntX::CModelActionChangeNodeShuntX(CDynaNode *pNode, double X) : CModelActionChangeNodeShunt(pNode, cplx(0.0, X))
{

}

DFW2_ACTION_STATE CModelActionChangeNodeShuntX::Do(CDynaModel *pDynaModel, double dValue)
{
	m_ShuntRX.imag(dValue);
	return CModelActionChangeNodeShunt::Do(pDynaModel);
}

CModelActionChangeNodeShuntAdmittance::CModelActionChangeNodeShuntAdmittance(CDynaNode *pNode, const cplx& ShuntGB) : CModelActionChangeNodeParameterBase(pNode),
																												m_ShuntGB(ShuntGB)
																												{

																												}
DFW2_ACTION_STATE CModelActionChangeNodeShuntAdmittance::Do(CDynaModel *pDynaModel)
{
	DFW2_ACTION_STATE State = AS_DONE;
	m_pDynaNode->Gshunt = m_ShuntGB.real();
	m_pDynaNode->Bshunt = m_ShuntGB.imag();
	pDynaModel->ProcessTopologyRequest();
	return State;
}


CModelActionChangeNodeShuntG::CModelActionChangeNodeShuntG(CDynaNode *pNode, double G) : CModelActionChangeNodeShuntAdmittance(pNode, cplx(G, 0.0))
{

}

DFW2_ACTION_STATE CModelActionChangeNodeShuntG::Do(CDynaModel *pDynaModel, double dValue)
{
	m_ShuntGB.real(dValue);
	return CModelActionChangeNodeShuntAdmittance::Do(pDynaModel);
}

CModelActionChangeNodeShuntB::CModelActionChangeNodeShuntB(CDynaNode *pNode, double B) : CModelActionChangeNodeShuntAdmittance(pNode, cplx(0.0, B))
{

}

DFW2_ACTION_STATE CModelActionChangeNodeShuntB::Do(CDynaModel *pDynaModel, double dValue)
{
	m_ShuntGB.imag(dValue);
	return CModelActionChangeNodeShuntAdmittance::Do(pDynaModel);
}



CModelActionRemoveNodeShunt::CModelActionRemoveNodeShunt(CDynaNode *pNode) : CModelActionChangeNodeParameterBase(pNode)
{

}

DFW2_ACTION_STATE CModelActionRemoveNodeShunt::Do(CDynaModel *pDynaModel)
{
	DFW2_ACTION_STATE State = AS_DONE;
	m_pDynaNode->Gshunt = m_pDynaNode->Bshunt = 0.0;
	m_pDynaNode->ProcessTopologyRequest();
	return State;
}


CModelActionChangeNodeLoad::CModelActionChangeNodeLoad(CDynaNode *pNode, cplx& LoadPower) : CModelActionChangeVariable(NULL, 0),
																						    m_pDynaNode(pNode),
																							m_newLoad(LoadPower)
{
	
}

DFW2_ACTION_STATE CModelActionChangeNodeLoad::Do(CDynaModel *pDynaModel)
{
	DFW2_ACTION_STATE State = AS_DONE;
	m_pDynaNode->Pn = m_newLoad.real();
	m_pDynaNode->Qn = m_newLoad.imag();
	m_pDynaNode->ProcessTopologyRequest();
	return State;
}


CModelActionState::CModelActionState(CDiscreteDelay *pDiscreteDelay) : CModelAction(AT_STATE),
																	   m_pDiscreteDelay(pDiscreteDelay)
{

}


DFW2_ACTION_STATE CModelActionState::Do(CDynaModel *pDynaModel)
{ 
	m_pDiscreteDelay->NotifyDelay(pDynaModel);
	pDynaModel->DiscontinuityRequest();
	return AS_INACTIVE; 
}