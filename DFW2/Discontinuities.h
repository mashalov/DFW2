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
	enum class eDFW2_ACTION_TYPE
	{
		AT_EMPTY,
		AT_STOP,
		AT_CV,
		AT_STATE
	};


	enum class eDFW2_ACTION_STATE
	{
		AS_INACTIVE,
		AS_DONE,
		AS_ERROR
	};

	class CDynaModel;

	class CModelAction
	{
	protected:
		eDFW2_ACTION_TYPE m_Type;
	public:
		CModelAction(eDFW2_ACTION_TYPE Type) : m_Type(Type) {}
		eDFW2_ACTION_TYPE Type() { return m_Type;  }
		virtual eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) { return eDFW2_ACTION_STATE::AS_INACTIVE; }
		virtual eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) { return Do(pDynaModel,0); }
		void Log(CDynaModel* pDynaModel, std::string_view message);
	};

	class CModelActionStop : public CModelAction
	{
	public:
		CModelActionStop();
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};


	class CDiscreteDelay;

	class CModelActionState : public CModelAction
	{
	protected:
		CDiscreteDelay *m_pDiscreteDelay;
	public:
		CModelActionState(CDiscreteDelay *pDiscreteDelay);
		CDiscreteDelay *GetDelayObject() { return m_pDiscreteDelay; }
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeVariable : public CModelAction
	{
	protected:
		double m_dTargetValue;
		double *m_pVariable;
	public:
		CModelActionChangeVariable(double *pVariable, double TargetValue);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeBranchParameterBase : public CModelActionChangeVariable
	{
	protected:
		CDynaBranch* m_pDynaBranch;
	public:
		CModelActionChangeBranchParameterBase(CDynaBranch* pBranch);
	};

	class CModelActionChangeBranchState : public CModelActionChangeBranchParameterBase
	{
	protected:
		CDynaBranch::BranchState m_NewState;
	public:
		CModelActionChangeBranchState(CDynaBranch *pBranch, CDynaBranch::BranchState NewState);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
		virtual eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue);
	};

	class CModelActionChangeBranchImpedance : public CModelActionChangeBranchParameterBase
	{
	protected:
		cplx m_Impedance;
	public:
		CModelActionChangeBranchImpedance(CDynaBranch* pBranch, const cplx& Impedance);
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel) override;
	};

	class CModelActionChangeBranchR : public CModelActionChangeBranchImpedance
	{
	public:
		CModelActionChangeBranchR(CDynaBranch* pBranch, double R);
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double R) override;
	};

	class CModelActionChangeBranchX : public CModelActionChangeBranchImpedance
	{
	public:
		CModelActionChangeBranchX(CDynaBranch* pBranch, double X);
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double X) override;
	};

	class CModelActionChangeNodeParameterBase : public CModelAction
	{
	protected:
		CDynaNode* m_pDynaNode;
	public:
		CModelActionChangeNodeParameterBase(CDynaNode *pNode) : CModelAction(eDFW2_ACTION_TYPE::AT_CV), m_pDynaNode(pNode)  {}
	};

	class CModelActionChangeNodeShunt : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx m_ShuntRX;
	public:
		CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeNodeShuntAdmittance : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx m_ShuntGB;
	public:
		CModelActionChangeNodeShuntAdmittance(CDynaNode *pNode, const cplx& ShuntGB);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeNodeShuntR : public CModelActionChangeNodeShunt
	{
	public:
		CModelActionChangeNodeShuntR(CDynaNode *pNode, double R);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) override;
	};

	class CModelActionChangeNodeShuntX : public CModelActionChangeNodeShunt
	{
	public:
		CModelActionChangeNodeShuntX(CDynaNode *pNode, double X);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) override;
	};

	class CModelActionChangeNodeShuntG : public CModelActionChangeNodeShuntAdmittance
	{
	public:
		CModelActionChangeNodeShuntG(CDynaNode *pNode, double G);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) override;
	};

	class CModelActionChangeNodeShuntB : public CModelActionChangeNodeShuntAdmittance
	{
	public:
		CModelActionChangeNodeShuntB(CDynaNode *pNode, double B);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double dValue) override;
	};
		
	class CModelActionChangeNodeLoad : public CModelActionChangeVariable
	{
	protected:
		CDynaNode *m_pDynaNode;
		cplx m_newLoad;
	public:
		CModelActionChangeNodeLoad(CDynaNode *pNode, cplx& LoadPower);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionRemoveNodeShunt : public CModelActionChangeNodeParameterBase
	{
	public:
		CModelActionRemoveNodeShunt(CDynaNode *pNode);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	typedef std::list<CModelAction*> MODELACTIONLIST;
	typedef MODELACTIONLIST::iterator MODELACTIONITR;



	class CStaticEvent
	{
	protected:
		double m_dTime;
		mutable MODELACTIONLIST m_Actions;
	public:
		CStaticEvent(double dTime);
		virtual ~CStaticEvent();
		bool AddAction(CModelAction* Action) const;
		double Time() const { return m_dTime; }
		bool ContainsStop() const;
		ptrdiff_t ActionsCount() const { return m_Actions.size(); }
		bool RemoveStateAction(CDiscreteDelay *pDelayObject) const;
		eDFW2_ACTION_STATE DoActions(CDynaModel *pDynaModel) const;

		bool operator<(const CStaticEvent& evt) const
		{
			return m_dTime < evt.m_dTime && !Equal(m_dTime,evt.m_dTime);
		}
	};

	class CStateObjectIdToTime
	{
		CDiscreteDelay* m_pDelayObject;
		mutable double m_dTime = 0.0;
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
		eDFW2_ACTION_STATE ProcessStaticEvents();
	};
}
