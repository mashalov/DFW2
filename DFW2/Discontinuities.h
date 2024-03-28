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

	//! Базовое действие над моделью
	class CModelAction
	{
	protected:
		eDFW2_ACTION_TYPE Type_;		// тип действия
	public:
		CModelAction(eDFW2_ACTION_TYPE Type) : Type_(Type) {}
		virtual ~CModelAction() = default;
		eDFW2_ACTION_TYPE Type() const { return Type_;  }
		//! Дейтвие без параметра
		virtual eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) { return eDFW2_ACTION_STATE::AS_INACTIVE; }
		//! Действие с параметром
		virtual eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) { return Do(pDynaModel,0); }
		//! Вывод в протокол через CDynaModel со стандартизованным форматированием
		void Log(const CDynaModel* pDynaModel, std::string_view message);
		//! Хелпер для проверки задано комплексное значение или нет
		static bool isfinite(const cplx& value) { return std::isfinite(value.real()) && std::isfinite(value.imag()); }
	};

	using ModelActionT = std::unique_ptr<CModelAction>;

	//! Действие останова расчета
	class CModelActionStop : public CModelAction
	{
	public:
		CModelActionStop() : CModelAction(eDFW2_ACTION_TYPE::AT_STOP) { }
		//! Действие просто выдает команду модели
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	//! Действие на изменение параметра устройства
	class CModelActionChangeDeviceParameter : public CModelAction
	{
	protected:
		CDevice* pDevice_ = nullptr;	// устройство, с которым нужно выполнить действие
		//! Функция репорта "было-стало" с двумя параметрами до и после действия
		template<typename T>
		void Log(const CDynaModel* pDynaModel, std::string_view ValueName, const T& NewValue)
		{
			CModelAction::Log(pDynaModel, fmt::format("{} {}={}",
				pDevice_->GetVerbalName(),
				ValueName,
				NewValue
			));
		}
		//! Функция репорта "стало" с одним параметром после действия, на случай если было значение "не задано"
		template<typename T>
		void Log(const CDynaModel* pDynaModel, std::string_view ValueName, const T& OldValue, const T& NewValue)
		{
			CModelAction::Log(pDynaModel, fmt::format("{} {}={} -> {}={}",
				pDevice_->GetVerbalName(),
				ValueName,
				OldValue,
				ValueName,
				NewValue
			));
		}
	public:
		CModelActionChangeDeviceParameter(eDFW2_ACTION_TYPE Type, CDevice* pDevice) :
			CModelAction(Type),
			pDevice_(pDevice) {}
		void WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view ChangeDescription);
	};

	//! Действие на изменение переменной устройства
	/*! Враппер для CModelActionChangeDeviceParameter с типом действия "Изменить переменную"
	*/
	class CModelActionChangeDeviceVariable : public CModelActionChangeDeviceParameter
	{
	protected:
		double *pVariable_ = nullptr;
		std::string VariableName_;
	public:
		CModelActionChangeDeviceVariable(CDevice* pDevice) : CModelActionChangeDeviceParameter(eDFW2_ACTION_TYPE::AT_CV, pDevice) {}
		CModelActionChangeDeviceVariable(CDevice* pDevice, std::string_view VariableName);
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel) override;
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
		inline bool Initialized() const { return pVariable_ != nullptr; }
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

	//! Базовый класс для действий с ветвями
	class CModelActionChangeBranchParameterBase : public CModelActionChangeDeviceVariable
	{
	protected:
		//! Хелпер с кастом наследованного pDevice_ в тип ветви
		inline CDynaBranch* Branch() { return static_cast<CDynaBranch*>(pDevice_);	}
		//! Отдельная реализация WriteSlowVariable для ветви со сложным ключом
		void WriteSlowVariable(CDynaModel* pDynaModel, std::string_view VariableName, double Value, double PreviousValue, std::string_view ChangeDescription);
	public:
		CModelActionChangeBranchParameterBase(CDynaBranch* pBranch) : CModelActionChangeDeviceVariable(pBranch) {}
	};

	//! Изменить состояния ветви
	class CModelActionChangeBranchState : public CModelActionChangeBranchParameterBase
	{
	protected:
		CDynaBranch::BranchState NewState_;
	public:
		CModelActionChangeBranchState(CDynaBranch *pBranch, CDynaBranch::BranchState NewState);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};

	//! Изменить комплексное сопротивление ветви
	class CModelActionChangeBranchImpedance : public CModelActionChangeBranchParameterBase
	{
	protected:
		cplx Impedance_;
	public:
		CModelActionChangeBranchImpedance(CDynaBranch* pBranch, const cplx& Impedance) : CModelActionChangeBranchParameterBase(pBranch), Impedance_(Impedance) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel) override;
		static constexpr const char* cszz_ = "z";
	};

	//! Изменить активное сопротвление ветви
	class CModelActionChangeBranchR : public CModelActionChangeBranchParameterBase
	{
	protected:
		double BranchR_;
	public:
		CModelActionChangeBranchR(CDynaBranch* pBranch, double R) : CModelActionChangeBranchParameterBase(pBranch), BranchR_(R) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double R) override;
	};

	//! Изменить реактивное сопротвление ветви
	class CModelActionChangeBranchX : public CModelActionChangeBranchParameterBase
	{
	protected:
		double BranchX_;
	public:
		CModelActionChangeBranchX(CDynaBranch* pBranch, double X) : CModelActionChangeBranchParameterBase(pBranch), BranchX_(X) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double X) override;
	};

	//! Изменить мнимую проводимость ветви
	class CModelActionChangeBranchB : public CModelActionChangeBranchParameterBase
	{
	protected:
		double BranchB_;
	public:
		CModelActionChangeBranchB(CDynaBranch* pBranch, double B) : CModelActionChangeBranchParameterBase(pBranch), BranchB_(B) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double B) override;
	};

	//! Базовый класс действий с узлом
	class CModelActionChangeNodeParameterBase : public CModelActionChangeDeviceVariable
	{
	protected:
		//! Хелпер с кастом наследованного pDevice_ в тип узла
		inline CDynaNode* Node() { return static_cast<CDynaNode*>(pDevice_); }
	public:
		CModelActionChangeNodeParameterBase(CDynaNode* pNode) : CModelActionChangeDeviceVariable(pNode) {}

	};

	//! Изменить комплексный шунт узла
	class CModelActionChangeNodeShunt : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx ShuntRX_;
	public:
		CModelActionChangeNodeShunt(CDynaNode *pNode, const cplx& ShuntRX);
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	//! Изменить комплексную нагрузку узла
	class CModelActionChangeLoad : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx InitialLoad_;
		double NewLoad_;
		void Log(const CDynaModel* pDynaModel, const cplx& SloadOld, const cplx& SloadNew)
		{
			CModelActionChangeNodeParameterBase::Log(pDynaModel, "Sload", SloadOld, SloadNew);
		}
	public:
		CModelActionChangeLoad(CDynaNode* pNode, double NewLoad) :
			CModelActionChangeNodeParameterBase(pNode),
			NewLoad_(NewLoad),
			InitialLoad_{ pNode->Pn, pNode->Qn } { }
	};

	//! Изменить активную нагрузку узла
	class CModelActionChangeNodePload : public CModelActionChangeLoad
	{
	public:
		CModelActionChangeNodePload(CDynaNode* pNode, double Pload) : CModelActionChangeLoad(pNode, Pload) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
	};

	//! Изменить реактивную нагрузку узла
	class CModelActionChangeNodeQload : public CModelActionChangeLoad
	{
	public:
		CModelActionChangeNodeQload(CDynaNode* pNode, double Qload) : CModelActionChangeLoad(pNode, Qload) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
	};

	//! Изменить комплекную нагрузку узла по активной с сохранением тангенса
	class CModelActionChangeNodePQload : public CModelActionChangeLoad
	{
	public:
		CModelActionChangeNodePQload(CDynaNode* pNode, double Pload) : CModelActionChangeLoad(pNode, Pload) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
	};

	//! Изменить комплексный шунт узла
	class CModelActionChangeNodeShuntAdmittance : public CModelActionChangeNodeParameterBase
	{
	protected:
		cplx ShuntGB_;
	public:
		CModelActionChangeNodeShuntAdmittance(CDynaNode *pNode, const cplx& ShuntGB) : CModelActionChangeNodeParameterBase(pNode), ShuntGB_(ShuntGB) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
		static constexpr const char* cszy_ = "y";
	};

	//! Изменить активный шунт узла
	class CModelActionChangeNodeShuntR : public CModelActionChangeNodeParameterBase
	{
	protected:
		double NodeR_;
	public:
		CModelActionChangeNodeShuntR(CDynaNode* pNode, double R) : CModelActionChangeNodeParameterBase(pNode), NodeR_(R) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};

	//! Изменить реактивный шунт узла
	class CModelActionChangeNodeShuntX : public CModelActionChangeNodeParameterBase
	{
	protected:
		double NodeX_;
	public:
		CModelActionChangeNodeShuntX(CDynaNode *pNode, double X) : CModelActionChangeNodeParameterBase(pNode), NodeX_(X) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};

	//! Задать остаточное напряжение КЗ в узле
	class CModelActionChangeNodeShuntToUscUref : public CModelActionChangeNodeParameterBase
	{
	protected:
		double Usc_;
	public:
		CModelActionChangeNodeShuntToUscUref(CDynaNode* pNode, double Usc) : CModelActionChangeNodeParameterBase(pNode), Usc_(Usc) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
	};

	//! Задать отношение R/X КЗ в узле
	class CModelActionChangeNodeShuntToUscRX : public CModelActionChangeNodeParameterBase
	{
	protected:
		double RXratio_;
	public:
		CModelActionChangeNodeShuntToUscRX(CDynaNode* pNode, double RXratio) : CModelActionChangeNodeParameterBase(pNode), RXratio_(RXratio) {}
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
	};

	//! Изменить активную проводимость узла
	class CModelActionChangeNodeShuntG : public CModelActionChangeNodeParameterBase
	{
	protected:
		double NodeG_ = 0.0;
	public:
		CModelActionChangeNodeShuntG(CDynaNode* pNode, double G) : CModelActionChangeNodeParameterBase(pNode), NodeG_(G) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};

	//! Изменить реактивную проводимость узла
	class CModelActionChangeNodeShuntB : public CModelActionChangeNodeParameterBase
	{
	protected:
		double NodeB_ = 0.0;
	public:
		CModelActionChangeNodeShuntB(CDynaNode* pNode, double B) : CModelActionChangeNodeParameterBase(pNode), NodeB_(B) { }
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel, double Value) override;
	};
	
	//! Удалить заданный шунт узла
	class CModelActionRemoveNodeShunt : public CModelActionChangeNodeParameterBase
	{
	public:
		CModelActionRemoveNodeShunt(CDynaNode* pNode) : CModelActionChangeNodeParameterBase(pNode) {}
		eDFW2_ACTION_STATE Do(CDynaModel *pDynaModel) override;
	};

	//! Изменить состояние устройства
	class CModelActionChangeDeviceState : public CModelActionChangeDeviceVariable
	{
	protected:
		eDEVICESTATE NewState_;
	public:
		CModelActionChangeDeviceState(CDevice* pDevice, eDEVICESTATE NewState) :
			CModelActionChangeDeviceVariable(pDevice),
			NewState_(NewState) { }

		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel) override;
		eDFW2_ACTION_STATE Do(CDynaModel* pDynaModel, double Value) override;
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
