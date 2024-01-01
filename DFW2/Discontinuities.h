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
		eDFW2_ACTION_TYPE Type_;
	public:
		CModelAction(eDFW2_ACTION_TYPE Type) : Type_(Type) {}
		virtual ~CModelAction() = default;
		eDFW2_ACTION_TYPE Type() { return Type_;  }
		virtual eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) { return eDFW2_ACTION_STATE::AS_INACTIVE; }
		virtual eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) { return Do(pDynaModel,0); }
		void Log(CDynaModel* pDynaModel, std::string_view message);
		static bool isfinite(const cplx& value) { return std::isfinite(value.real()) && std::isfinite(value.imag()); }
	};

	using ModelActionT = std::unique_ptr<CModelAction>;

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
		CDiscreteDelay *pDiscreteDelay_;
	public:
		CModelActionState(CDiscreteDelay *pDiscreteDelay);
		CDiscreteDelay *GetDelayObject() { return pDiscreteDelay_; }
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeVariable : public CModelAction
	{
	protected:
		double TargetValue_;
		double *pVariable_;
	public:
		CModelActionChangeVariable(double *pVariable, double TargetValue);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeBranchParameterBase : public CModelActionChangeVariable
	{
	protected:
		CDynaBranch* pDynaBranch_;
		void WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view ChangeDescription);
	public:
		CModelActionChangeBranchParameterBase(CDynaBranch* pBranch) : CModelActionChangeVariable(nullptr, 0.0), pDynaBranch_(pBranch) {}
	};

	class CModelActionChangeBranchState : public CModelActionChangeBranchParameterBase
	{
	protected:
		CDynaBranch::BranchState NewState_;
	public:
		CModelActionChangeBranchState(CDynaBranch *pBranch, CDynaBranch::BranchState NewState);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
		virtual eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value);
	};

	class CModelActionChangeBranchImpedance : public CModelActionChangeBranchParameterBase
	{
	protected:
		cplx Impedance_;
	public:
		CModelActionChangeBranchImpedance(CDynaBranch* pBranch, const cplx& Impedance) : CModelActionChangeBranchParameterBase(pBranch), Impedance_(Impedance) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel) override;
	};

	class CModelActionChangeBranchR : public CModelActionChangeBranchParameterBase
	{
	protected:
		double BranchR_;
	public:
		CModelActionChangeBranchR(CDynaBranch* pBranch, double R) : CModelActionChangeBranchParameterBase(pBranch), BranchR_(R) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double R) override;
	};

	class CModelActionChangeBranchX : public CModelActionChangeBranchParameterBase
	{
	protected:
		double BranchX_;
	public:
		CModelActionChangeBranchX(CDynaBranch* pBranch, double X) : CModelActionChangeBranchParameterBase(pBranch), BranchX_(X) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double X) override;
	};

	class CModelActionChangeBranchB : public CModelActionChangeBranchParameterBase
	{
	protected:
		double BranchB_;
	public:
		CModelActionChangeBranchB(CDynaBranch* pBranch, double B) : CModelActionChangeBranchParameterBase(pBranch), BranchB_(B) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double B) override;
	};

	class CModelActionChangeNodeParameterBase : public CModelAction
	{
	protected:
		CDynaNode* pDynaNode_;
		void WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view ChangeDescription);
	public:
		CModelActionChangeNodeParameterBase(CDynaNode *pNode) : CModelAction(eDFW2_ACTION_TYPE::AT_CV), pDynaNode_(pNode)  {}

	};

	class CModelActionChangeNodeShunt : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx ShuntRX_;
	public:
		CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeNodePQLoad : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx InitialLoad_;
		double Pload_;
	public:
		CModelActionChangeNodePQLoad(CDynaNode* pNode, double Pload);
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
	};

	class CModelActionChangeNodeShuntAdmittance : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx ShuntGB_;
	public:
		CModelActionChangeNodeShuntAdmittance(CDynaNode *pNode, const cplx& ShuntGB) : CModelActionChangeNodeParameterBase(pNode), ShuntGB_(ShuntGB) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeNodeShuntR : public CModelActionChangeNodeParameterBase
	{
	protected:
		double NodeR_;
	public:
		CModelActionChangeNodeShuntR(CDynaNode* pNode, double R) : CModelActionChangeNodeParameterBase(pNode), NodeR_(R) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};

	class CModelActionChangeNodeShuntX : public CModelActionChangeNodeParameterBase
	{
	protected:
		double NodeX_;
	public:
		CModelActionChangeNodeShuntX(CDynaNode *pNode, double X) : CModelActionChangeNodeParameterBase(pNode), NodeX_(X) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};

	class CModelActionChangeNodeShuntToUsc : public CModelActionChangeNodeParameterBase
	{
	protected:
		double Usc_, RXratio_;
	public:
		CModelActionChangeNodeShuntToUsc(CDynaNode* pNode, double Usc, double RXratio) : CModelActionChangeNodeParameterBase(pNode), Usc_(Usc), RXratio_(RXratio) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
	};

	class CModelActionChangeNodeShuntG : public CModelActionChangeNodeParameterBase
	{
	protected:
		double NodeG_ = 0.0;
	public:
		CModelActionChangeNodeShuntG(CDynaNode* pNode, double G) : CModelActionChangeNodeParameterBase(pNode), NodeG_(G) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};

	class CModelActionChangeNodeShuntB : public CModelActionChangeNodeParameterBase
	{
	protected:
		double NodeB_ = 0.0;
	public:
		CModelActionChangeNodeShuntB(CDynaNode* pNode, double B) : CModelActionChangeNodeParameterBase(pNode), NodeB_(B) { }
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};
		
	class CModelActionChangeNodeLoad : public CModelActionChangeVariable
	{
	protected:
		CDynaNode *pDynaNode_;
		cplx newLoad_;
	public:
		CModelActionChangeNodeLoad(CDynaNode *pNode, cplx& LoadPower);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionRemoveNodeShunt : public CModelActionChangeNodeParameterBase
	{
	public:
		CModelActionRemoveNodeShunt(CDynaNode* pNode) : CModelActionChangeNodeParameterBase(pNode) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	class CModelActionChangeDeviceState : public CModelActionChangeVariable
	{
	protected:
		CDevice* pDevice_;
		eDEVICESTATE NewState_;
	public:
		CModelActionChangeDeviceState(CDevice* pDevice, eDEVICESTATE NewState);
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel) override;
		virtual eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value);
	};

	using MODELACTIONLIST = std::list<ModelActionT>;

	class CStaticEvent
	{
	protected:
		double Time_;
		mutable MODELACTIONLIST Actions_;
	public:
		CStaticEvent(double Time) : Time_(Time) {}
		virtual ~CStaticEvent() = default;
		bool AddAction(ModelActionT&& Action) const;
		double Time() const { return Time_; }
		ptrdiff_t ActionsCount() const { return Actions_.size(); }
		bool RemoveStateAction(CDiscreteDelay *pDelayObject) const;
		eDFW2_ACTION_STATE DoActions(CDynaModel *pDynaModel) const;

		bool operator<(const CStaticEvent& evt) const
		{
			return Time_ < evt.Time_ && !Consts::Equal(Time_,evt.Time_);
		}
	};

	//! Класс для подстановки в качестве аргумента для поиска в сете
	class CStaticEventSearch : public CStaticEvent
	{
	public:
		CStaticEventSearch() : CStaticEvent(0.0) {}
		//! Возвращает CStaticEvent с заданным времением
		const CStaticEventSearch& Time(double Time)
		{
			Time_ = Time;
			return *this;
		}
	};


	class CStateObjectIdToTime
	{
		CDiscreteDelay* pDelayObject_;
		mutable double Time_ = 0.0;
	public:
		double Time() const { return Time_; }
		void Time(double Time) const { Time_ = Time; }
		CStateObjectIdToTime(CDiscreteDelay *pDelayObject) : pDelayObject_(pDelayObject) {}
		CStateObjectIdToTime(CDiscreteDelay *pDelayObject, double Time) : pDelayObject_(pDelayObject), Time_(Time) {}
		bool operator<(const CStateObjectIdToTime& cid) const
		{
			return pDelayObject_ < cid.pDelayObject_;
		}
	};

	using STATICEVENTSET = std::set<CStaticEvent>;
	using STATEEVENTSET = std::set<CStateObjectIdToTime>;

	class CDiscontinuities
	{
	protected:
		CDynaModel *pDynaModel_;
		STATICEVENTSET StaticEvent_;
		STATEEVENTSET StateEvents_;
		// Вместо создания для поиска CStaticEvent() 
		// используем заранее созданный CStativEventSearch
		// которому для поиска задается время
		// для вызова из const-функция он mutable
		mutable CStaticEventSearch TimeSearch_;
	public:
		CDiscontinuities(CDynaModel* pDynaModel) : pDynaModel_(pDynaModel) {}
		virtual ~CDiscontinuities() = default;
		bool AddEvent(double Time, ModelActionT&& Action);
		bool SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double Time);
		bool RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject);
		bool CheckStateDiscontinuity(CDiscreteDelay *pDelayObject);
		void Init();
		double NextEventTime();
		void PassTime(double Time);
		eDFW2_ACTION_STATE ProcessStaticEvents();
		size_t EventsLeft(double Time) const;
	};
}
