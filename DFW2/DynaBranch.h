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
		DynaReactors reactorsHead, reactorsTail;	// список отключаемых реакторов в начале и конце
		ptrdiff_t NrIp, NrIq;						// количество реакторов в начале и в конце
		CDynaNodeBase *m_pNodeIp, *m_pNodeIq;		// узлы начала и конца
		CDynaNodeBase *m_pNodeSuperIp, 
					  *m_pNodeSuperIq;				// суперузлы начала и конца
		double GIp0, BIp0, GIq0, BIq0;				// статические расчетные проводимости в начале и в конце, учитывающие проводимости ветви и статические реакторы
		double GIp, BIp, GIq, BIq;					// расчетные проводимости в начале и в конце, статические проводимости и отключаемые реакторы
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


		CDynaBranch() : CDevice() {};
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

		static constexpr const char* m_cszBranchStateNames[4] = { "On", "Off", "Htrip", "Ttrip", };
	};

	struct BranchComp
	{
		bool operator()(const CDynaBranch* lhs, const CDynaBranch* rhs) const
		{
			return std::tie(lhs->key.Ip, lhs->key.Iq, lhs->key.Np) < 
					std::tie(rhs->key.Ip, rhs->key.Iq, rhs->key.Np);
		}
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
		void CreateMeasures(CDeviceContainer& containerBranchMeasures);
	};
}
