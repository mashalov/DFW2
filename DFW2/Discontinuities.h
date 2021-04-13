#pragma once
#include "Header.h"
#include "list"
#include "set"
#include "DynaBranch.h"

//Откл 
//Объект
//Сост узла
//Сост ветви
//Узел Gш
//Узел Bш
//Узел Rш
//Узел Xш
//Узел Pn
//Узел Qn
//ЭГП
//Якоби
//Узел PnQn
//Узел PnQn0
//ГРАМ
//РТ

namespace DFW2
{
	enum DFW2_ACTION_TYPE
	{
		AT_EMPTY,
		AT_STOP,
		AT_CV,
		AT_STATE
	};


	enum DFW2_ACTION_STATE
	{
		AS_INACTIVE,
		AS_DONE,
		AS_ERROR
	};

	class CDynaModel;

	class CModelAction
	{
	protected:
		DFW2_ACTION_TYPE m_Type;
	public:
		CModelAction(DFW2_ACTION_TYPE Type) : m_Type(Type) {}
		DFW2_ACTION_TYPE Type() { return m_Type;  }
		virtual DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) { return AS_INACTIVE; }
		virtual DFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) { return Do(pDynaModel,0); }
	};

	class CModelActionStop : public CModelAction
	{
	public:
		CModelActionStop();
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};


	class CDiscreteDelay;

	class CModelActionState : public CModelAction
	{
	protected:
		CDiscreteDelay *m_pDiscreteDelay;
	public:
		CModelActionState(CDiscreteDelay *pDiscreteDelay);
		CDiscreteDelay *GetDelayObject() { return m_pDiscreteDelay; }
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeVariable : public CModelAction
	{
	protected:
		double m_dTargetValue;
		double *m_pVariable;
	public:
		CModelActionChangeVariable(double *pVariable, double TargetValue);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeBranchState : public CModelActionChangeVariable
	{
	protected:
		CDynaBranch *m_pDynaBranch;
		CDynaBranch::BranchState m_NewState;
	public:
		CModelActionChangeBranchState(CDynaBranch *pBranch, CDynaBranch::BranchState NewState);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
		virtual DFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue);
	};

	class CModelActionChangeNodeParameterBase : public CModelAction
	{
	protected:
		CDynaNode *m_pDynaNode;
	public:
		CModelActionChangeNodeParameterBase(CDynaNode *pNode) : CModelAction(AT_CV), m_pDynaNode(pNode) {}
	};

	class CModelActionChangeNodeShunt : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx m_ShuntRX;
	public:
		CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeNodeShuntAdmittance : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx m_ShuntGB;
	public:
		CModelActionChangeNodeShuntAdmittance(CDynaNode *pNode, const cplx& ShuntGB);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeNodeShuntR : public CModelActionChangeNodeShunt
	{
	public:
		CModelActionChangeNodeShuntR(CDynaNode *pNode, double R);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) override;
	};

	class CModelActionChangeNodeShuntX : public CModelActionChangeNodeShunt
	{
	public:
		CModelActionChangeNodeShuntX(CDynaNode *pNode, double X);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) override;
	};

	class CModelActionChangeNodeShuntG : public CModelActionChangeNodeShuntAdmittance
	{
	public:
		CModelActionChangeNodeShuntG(CDynaNode *pNode, double G);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) override;
	};

	class CModelActionChangeNodeShuntB : public CModelActionChangeNodeShuntAdmittance
	{
	public:
		CModelActionChangeNodeShuntB(CDynaNode *pNode, double B);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) override;
	};
		
	class CModelActionChangeNodeLoad : public CModelActionChangeVariable
	{
	protected:
		CDynaNode *m_pDynaNode;
		cplx m_newLoad;
	public:
		CModelActionChangeNodeLoad(CDynaNode *pNode, cplx& LoadPower);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionRemoveNodeShunt : public CModelActionChangeNodeParameterBase
	{
	public:
		CModelActionRemoveNodeShunt(CDynaNode *pNode);
		DFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	typedef std::list<CModelAction*> MODELACTIONLIST;
	typedef MODELACTIONLIST::iterator MODELACTIONITR;



	class CStaticEvent
	{
	protected:
		double m_dTime;
		mutable MODELACTIONLIST m_Actions;
		mutable DFW2_ACTION_TYPE m_ActionsResult;
	public:
		CStaticEvent(double dTime);
		virtual ~CStaticEvent();
		bool AddAction(CModelAction* Action) const;
		double Time() const { return m_dTime; }
		bool ContainsStop() const;
		ptrdiff_t ActionsCount() const { return m_Actions.size(); }
		bool RemoveStateAction(CDiscreteDelay *pDelayObject) const;
		DFW2_ACTION_STATE DoActions(CDynaModel *pDynaModel) const;

		bool operator<(const CStaticEvent& evt) const
		{
			return m_dTime < evt.m_dTime && !Equal(m_dTime,evt.m_dTime);
		}
	};

	class CStateObjectIdToTime
	{
		CDiscreteDelay* m_pDelayObject;
		mutable double m_dTime;
	public:
		double Time() const { return m_dTime; }
		void Time(double dTime) const { m_dTime = dTime; }
		CStateObjectIdToTime(CDiscreteDelay *pDelayObject) : m_pDelayObject(pDelayObject) {}
		CStateObjectIdToTime(CDiscreteDelay *pDelayObject, double dTime) : m_pDelayObject(pDelayObject), m_dTime(dTime) {}
		bool operator<(const CStateObjectIdToTime& cid) const
		{
			return m_pDelayObject < cid.m_pDelayObject;
		}
	};

	typedef std::set<CStaticEvent> STATICEVENTSET;
	typedef STATICEVENTSET::iterator STATICEVENTITR;
	typedef std::set<CStateObjectIdToTime> STATEEVENTSET;
	typedef STATEEVENTSET::iterator STATEEVENTITR;
	

	class CDiscontinuities
	{
	protected:
		CDynaModel *m_pDynaModel;
		STATICEVENTSET m_StaticEvent;
		STATEEVENTSET m_StateEvents;
	public:
		CDiscontinuities(CDynaModel *pDynaModel);
		virtual ~CDiscontinuities() = default;
		bool AddEvent(double dTime, CModelAction* Action);
		bool SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dTime);
		bool RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject);
		bool CheckStateDiscontinuity(CDiscreteDelay *pDelayObject);
		void Init();
		double NextEventTime();
		void PassTime(double dTime);
		DFW2_ACTION_STATE ProcessStaticEvents();
	};
}
