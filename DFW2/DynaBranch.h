#pragma once
#include "DeviceContainer.h"
#include "DynaReactor.h"
#include "DynaNode.h"


namespace DFW2
{

	// у ветви может быть объект типа
	// CDynaBranchMeasure - если нам потребуются
	// результаты расчета потоков
	class CDynaBranchMeasure;

	class CDynaBranch : public CDevice
	{
	protected:
		void UpdateVerbalName() override;
	public:

		CDynaBranchMeasure* m_pMeasure = nullptr;

		struct Key 
		{
			ptrdiff_t Ip, Iq, Np;						// номера узлов начала и конца, номер парр. цепи
		};

		Key key;
		double R, X;								// сопротивление
		double Ktr, Kti;							// комплексный коэффициент трансформации
		double G, B, GrIp, GrIq, BrIp, BrIq;		// проводимость ветви, проводимости реакторов в начали и в конце
		DynaReactors reactors;						// список реакторов
		ptrdiff_t NrIp, NrIq;						// количество реакторов в начале и в конце
		CDynaNodeBase *m_pNodeIp, *m_pNodeIq;		// узлы начала и конца
		CDynaNodeBase *m_pNodeSuperIp, 
					  *m_pNodeSuperIq;				// суперузлы начала и конца
		double GIp, BIp, GIq, BIq;					// расчетные проводимости в начале и в конце
		cplx Yip, Yiq, Yips, Yiqs;					// компоненты взаимных и собственных проводимостей 
													// для матрицы узловых проводимостей

		cplx Sb, Se;								// поток в ветви с нулевым сопротивлением

		double deltaDiff = 0.0;						// разность углов по линии

		// состояние ветви
		enum class BranchState	
		{
			BRANCH_ON,				// полностью включена
			BRANCH_OFF,				// полностью отключена
			BRANCH_TRIPIP,			// отключена в начале
			BRANCH_TRIPIQ			// отключена в конце
		} 
			m_BranchState;

		CDynaBranch();
		virtual ~CDynaBranch() = default;
		cplx GetYBranch(bool bFixNegativeZ = false);
		bool LinkToContainer(CDeviceContainer *pContainer, CDeviceContainer *pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom) override;
		eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice = nullptr) override;
		bool DisconnectBranchFromNode(CDynaNodeBase* pNode);
		bool BranchAndNodeConnected(CDynaNodeBase* pNode);
		eDEVICESTATE GetState() const override;
		eDEVICEFUNCTIONSTATUS SetBranchState(BranchState eBranchState, eDEVICESTATECAUSE eStateCause);
		void CalcAdmittances(bool bFixNegativeZs);
		bool IsZeroImpedance();

		void UpdateSerializer(CSerializerBase* Serializer) override;

		CDynaNodeBase* GetOppositeNode(CDynaNodeBase* pOriginNode);
		CDynaNodeBase* GetOppositeSuperNode(CDynaNodeBase* pOriginNode);

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static const char* m_cszBranchStateNames[4];
	};

	struct BranchComp
	{
		bool operator()(const CDynaBranch* lhs, const CDynaBranch* rhs) const
		{
			return std::tie(lhs->key.Ip, lhs->key.Iq, lhs->key.Np) < 
					std::tie(rhs->key.Ip, rhs->key.Iq, rhs->key.Np);
		}
	};


	// Блок расчета параметров потоков по ветви. Добавляется в Якоби для ветви,
	// по которой необходимы эти параметры
	class CDynaBranchMeasure : public CDevice
	{
	protected:
		CDynaBranch *m_pBranch;			// ветвь, для которой выполняеются расчеты потоков
	public:
		enum VARS
		{
			V_IBRE,						// Ток в прямоугольных координатах в начале
			V_IBIM,
			V_IERE,						// Ток в прямоугольных координатах в конце
			V_IEIM,
			V_IB,						// Модуль тока в начале
			V_IE,						// Модуль тока в конце
			V_PB,						// Активная и реактивная мощности в начале
			V_QB,
			V_PE,						// Активная и реактивная мощности в конце
			V_QE,
			V_SB,						// Полные мощности в начале и в конце 
			V_SE,
			V_LAST
		};

		VariableIndex Ibre, Ibim, Iere, Ieim, Ib, Ie, Pb, Qb, Pe, Qe, Sb, Se;
		CDynaBranchMeasure(CDynaBranch *pBranch);

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);
		static void CalculateFlows(const CDynaBranch* pBranch, cplx& cIb, cplx& cIe, cplx& cSb, cplx& cSe);
	};


	class CDynaBranchContainer : public CDeviceContainer
	{
	protected:
		std::set<CDynaBranch*, BranchComp> BranchKeyMap;
	public:
		void IndexBranchIds();
		CDynaBranch* GetByKey(const CDynaBranch::Key& key);
		using CDeviceContainer::CDeviceContainer;
		void LinkToReactors(CDeviceContainer& containerReactors);
	};
}
