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

		CDynaBranchMeasure* pMeasure_ = nullptr;

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
		CDynaNodeBase *pNodeIp_, *pNodeIq_;			// узлы начала и конца
		CDynaNodeBase *pNodeSuperIp_, 
					  *pNodeSuperIq_;				// суперузлы начала и конца
		double GIp0, BIp0, GIq0, BIq0;				// статические расчетные проводимости в начале и в конце, учитывающие проводимости ветви и статические реакторы
		double GIp, BIp, GIq, BIq;					// расчетные проводимости в начале и в конце, статические проводимости и отключаемые реакторы
		cplx Yip, Yiq, Yips, Yiqs;					// компоненты взаимных и собственных проводимостей 
													// для матрицы узловых проводимостей

		cplx Sb, Se;								// поток в ветви

		double deltaDiff = 0.0;						// разность углов по линии

		// состояние ветви
		enum class BranchState	
		{
			BRANCH_ON,				// полностью включена
			BRANCH_OFF,				// полностью отключена
			BRANCH_TRIPIP,			// отключена в начале
			BRANCH_TRIPIQ			// отключена в конце
		} 
			BranchState_;


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
		// возвращает продольную проводимость относительно заданного узла
		inline const cplx& OppositeY(const CDynaNodeBase* pOriginNode) const
		{
			_ASSERTE(pOriginNode == pNodeIq_ || pOriginNode == pNodeIp_);
			return (pNodeIp_ == pOriginNode) ? Yip : Yiq;
		}
		// возвращает ток от заданного узла
		inline const cplx CurrentFrom(const CDynaNodeBase* pOriginNode) const
		{
			_ASSERTE(pOriginNode == pNodeIq_ || pOriginNode == pNodeIp_);
			if (pNodeIp_ == pOriginNode)
				return cplx(pNodeIq_->Vre, pNodeIq_->Vim) * Yip;
			else
				return cplx(pNodeIp_->Vre, pNodeIp_->Vim) * Yiq;
		}
		void UpdateSerializer(CSerializerBase* Serializer) override;

		// возвращает узел с обратного конца ветви относительно заданого
		CDynaNodeBase* GetOppositeNode(CDynaNodeBase* pOriginNode);
		// возвращает суперузел с обратного конца ветви относительно заданного
		CDynaNodeBase* GetOppositeSuperNode(CDynaNodeBase* pOriginNode);
		// возвращает true если ветвь внутри суперузла (суперузлы с обоих концов одинаковые, и ненулевые)
		inline bool InSuperNode() const { return (pNodeSuperIp_ == pNodeSuperIq_) && pNodeSuperIp_; }

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* cszBranchStateNames_[4] = { "On", "Off", "Htrip", "Ttrip", };
		static constexpr const char *cszAliasBranch_ = "vetv";
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
