#pragma once
#include "DeviceContainer.h"
#include "DynaLRC.h"
#include "DynaReactor.h"
#include "IterationControl.h"
#include "queue"
#include "stack"

namespace DFW2
{
	class CDynaBranch;
	class CSynchroZone : public CDevice
	{
	public:
		enum VARS
		{
			V_S,			// скольжение в синхронной зоне
			V_LAST
		};

		DEVICEVECTOR m_LinkedGenerators;
		VariableIndex S;						// переменная состояния - скольжение

		double Mj = 0.0;						// суммарный момент инерции
		bool m_bInfPower = false;				// признак наличия ШБМ
		CSynchroZone();		
		virtual ~CSynchroZone() = default;
		bool m_bEnergized = false;				// признак наличия источника напряжения

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel)  override;
		bool BuildRightHand(CDynaModel* pDynaModel)  override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel)  override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
	};

#define DFW2_SQRT_EPSILON DFW2_EPSILON

	struct VirtualBranch;
	struct VirtualZeroBranch;

	class CDynaNodeBase : public CDevice
	{
	public:

		void UpdateVreVim();
		void UpdateVDelta();
		void UpdateVreVimSuper();
		void UpdateVDeltaSuper();

		enum class eLFNodeType
		{
			LFNT_BASE,
			LFNT_PQ,
			LFNT_PV,
			LFNT_PVQMAX,
			LFNT_PVQMIN
		};

		// в устройствах удобно объявить перечисления
		// для именования индексов переменных
		enum CONSTVARS
		{
			C_GSH,
			C_BSH
		};

		enum VARS
		{
			V_V,
			V_RE,
			V_IM,
			V_LAST		// последняя переменная (реально не существует) указывает количество уравнений устройства
		};

		VariableIndex Delta;
		VariableIndex V;
		VariableIndex Vre;
		VariableIndex Vim;

#ifdef _DEBUG
		double Vrastr, Deltarastr, Qgrastr, Pnrrastr, Qnrrastr;
		void GrabRastrResult()
		{
			Vrastr = V.Value;
			Deltarastr = Delta.Value;
			Qgrastr = Qg;
			Pnrrastr = Pnr;
			Qnrrastr = Qnr;
		}
#endif
				
		double Pn,Qn,Pg,Qg,Pnr,Qnr,Pgr,Qgr;
		double G,B, Gr0, Br0;
		double dLRCShuntPartP, dLRCShuntPartQ;
		double Gshunt, Bshunt;
		double Unom;					// номинальное напряжение
		double V0;						// напряжение в начальных условиях (используется для "подтяжки" СХН к исходному режиму)
		bool m_bInMetallicSC = false;
		bool m_bLowVoltage;				// признак низкого модуля напряжения
		bool m_bSavedLowVoltage;		// сохраненный признак низкого напряжения для возврата на предыдущий шаг
		double dLRCVicinity = 0.0;		// окрестность сглаживания СХН

		ptrdiff_t LRCLoadFlowId  = 0;	// идентификаторы СХН и ДСХН
		ptrdiff_t LRCTransientId = 0;

		double dLRCPn;					// расчетные значения прозводных СХН по напряжению
		double dLRCQn;
		double dLRCPg;
		double dLRCQg;

		CSynchroZone *m_pSyncZone = nullptr;		// синхронная зона, к которой принадлежит узел
		eLFNodeType m_eLFNodeType;
		ptrdiff_t Nr;
		cplx Yii;						// собственная проводимость
		cplx YiiSuper;					// собственная проводимость суперузла
		double Vold;					// модуль напряжения на предыдущей итерации
		CDynaLRC *m_pLRC = nullptr;		// указатель на СХН узла в динамике
		CDynaLRC *m_pLRCLF = nullptr;	// указатель на СХН узла в УР
		CDynaLRC *m_pLRCGen = nullptr;	// СХН для генерации, которая не задана моделями генераторов

		DynaReactors reactors;			// список реакторов

		double LFVref, LFQmin, LFQmax;	// заданный модуль напряжения и пределы по реактивной мощности для УР
		double LFQminGen, LFQmaxGen;	// суммарные ограничения реактивной мощности генераторов в узле
		CDynaNodeBase();
		virtual ~CDynaNodeBase() = default;
		double* GetVariablePtr(ptrdiff_t nVarIndex)  override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex)  override;
		void GetPnrQnr();
		void GetPnrQnrSuper();
		bool AllLRCsInShuntPart(double V, double Vmin);
		bool BuildEquations(CDynaModel* pDynaModel)  override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		void NewtonUpdateEquation(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		void MarkZoneEnergized();
		void ProcessTopologyRequest();
		void CalcAdmittances(bool bFixNegativeZs);
		void GetGroundAdmittance(cplx& y);
		void CalculateShuntParts();
		// инициализация узла для расчета УР
		void StartLF(bool bFlatStart, double ImbTol);
		void StoreStates() override;
		void RestoreStates() override;
		bool InMatrix() override;
		void InitNordsiek(CDynaModel* pDynaModel) override;
		void SuperNodeLoadFlow(CDynaModel *pDynaModel);
		bool IsDangling();
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		inline double GetSelfImbP() noexcept { return Pnr - Pgr - V * V * YiiSuper.real();	}
		inline double GetSelfImbQ() noexcept { return Qnr - Qgr + V * V * YiiSuper.imag(); }

		// небаланс узла без привязки к суперузлу
		inline double GetSelfImbPnotSuper() noexcept { return Pnr - Pgr - V * V * Yii.real(); }
		inline double GetSelfImbQnotSuper() noexcept { return Qnr - Qgr + V * V * Yii.imag(); }

		inline double GetSelfdPdV() noexcept { return -2 * V * YiiSuper.real() + dLRCPn; }
		inline double GetSelfdQdV() noexcept { return  2 * V * YiiSuper.imag() + dLRCQn; }

		inline bool IsLFTypePQ() noexcept { return m_eLFNodeType != eLFNodeType::LFNT_PV; }

		void SetMatrixRow(ptrdiff_t nMatrixRow) noexcept { m_nMatrixRow = nMatrixRow; }

		VariableIndexExternal GetExternalVariable(std::string_view VarName) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);

		CDynaNodeBase* m_pSuperNodeParent = nullptr;
		CLinkPtrCount* GetSuperLink(ptrdiff_t nLinkIndex);

		VirtualBranch	  *m_VirtualBranchBegin, *m_VirtualBranchEnd;
		VirtualZeroBranch *m_VirtualZeroBranchBegin, *m_VirtualZeroBranchEnd, *m_VirtualZeroBranchParallelsBegin;

		VirtualZeroBranch* AddZeroBranch(CDynaBranch* pBranch);
		void TidyZeroBranches();
		// выбирает исходное напряжение либо равное расчетному, либо (если расчетное равно почему-то нулю), номинальному
		inline void PickV0() noexcept { V0 = (V > 0) ? V : Unom; }
		void UpdateSerializer(CSerializerBase* Serializer) override;

		void AddToTopologyCheck();

		static const char* m_cszV;
		static const char* m_cszDelta;
		static const char* m_cszVre;
		static const char* m_cszVim;
		static const char* m_cszGsh;
		static const char* m_cszBsh;
		static const char* m_cszLFNodeTypeNames[5];

	protected:
		void FromSuperNode();
		void SetLowVoltage(bool bLowVoltage);
		double FindVoltageZC(CDynaModel *pDynaModel, RightVector *pRvre, RightVector *pRvim, double Hyst, bool bCheckForLow);

	};

	class CDynaNode : public CDynaNodeBase
	{
	public:
		// здесь переменные состояния добавляются к базовому классу
		enum VARS
		{
			V_DELTA = CDynaNodeBase::V_LAST,
			V_LAG, 			// дифференциальное уравнение лага скольжения в третьей строке
			V_S,			// скольжение
	//		V_SIP,
	//		V_COP,
	//		V_SV,
			V_LAST
		};

		VariableIndex Lag;
		//double Sip;
		//double Cop;
		VariableIndex S;
		//double Sv, Dlt;
		CDynaNode();
		virtual ~CDynaNode() = default;
		double* GetVariablePtr(ptrdiff_t nVarIndex)  override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel)  override;
		bool BuildRightHand(CDynaModel* pDynaModel)  override;
		bool BuildDerivatives(CDynaModel *pDynaModel)  override;
		void Predict()  override;			// допонительная обработка прогноза
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel)  override;
		eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice *pCauseDevice = nullptr)  override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel)  override;
		void UpdateSerializer(CSerializerBase* Serializer) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static const char *m_cszS;
		static const char *m_cszSz;
	};

	// "виртуальная" ветвь для узла. Заменяет собой настоящую включенную ветвь или несколько
	// включенных параллельных ветвей эквивалентным Y. Эти два параметра - все что нужно
	// чтобы рассчитать небаланс узла и производные для смежных узлов
	struct VirtualBranch
	{
		cplx Y;
		CDynaNodeBase *pNode;
	};


	struct VirtualZeroBranch
	{
		CDynaBranch *pBranch;
		CDynaBranch *pParallelTo = nullptr;
		ptrdiff_t nParallelCount = 1;
	};

	// маппинг узла в строки матрица
	struct _MatrixInfo
	{
		size_t nRowCount = 0;													// количество элементов в строке матрицы
		CDynaNodeBase *pNode;													// узел, к которому относится данное Info
		ptrdiff_t m_nPVSwitchCount = 0;											// счетчик переключений PV-PQ
		double m_dImbP, m_dImbQ;												// небалансы по P и Q
		bool bVisited = false;													// признак просмотра для графовых алгоритмов
		double LFQmin;															// исходные ограничения реактивной мощности до ввода в суперузел
		double LFQmax;
		double NodeViolation;													// отклонение параметра от ограничения или уставки
		CDynaNodeBase::eLFNodeType LFNodeType;									// исходный тип узла до ввода в суперузел
		double UncontrolledP = 0.0;
		double UncontrolledQ = 0.0;												// постоянные значения активной и реактивной генерации в суперузле

		void Store(CDynaNodeBase *pStoreNode) noexcept
		{
			pNode = pStoreNode;
			LFQmin = pNode->LFQmin;
			LFQmax = pNode->LFQmax;
			LFNodeType = pNode->m_eLFNodeType;
		}
		void Restore() noexcept
		{
			pNode->LFQmin = LFQmin;
			pNode->LFQmax = LFQmax;
			pNode->m_eLFNodeType = LFNodeType;
		}
	};

	using QUEUE = std::list<_MatrixInfo*>;
	using MATRIXINFO = std::vector<_MatrixInfo*>;
	using NodeQueue = std::queue<CDynaNodeBase*>;
	using NodeSet = std::set<CDynaNodeBase*>;
	using NODEISLANDMAP = std::map<CDynaNodeBase*, NodeSet> ;
	using NODEISLANDMAPITRCONST = std::map<CDynaNodeBase*, NodeSet>::const_iterator;
	using DEVICETODEVICEMAP = std::map<CDevice*, CDevice*>;
	using ORIGINALLINKSVEC = std::vector<std::unique_ptr<DEVICETODEVICEMAP>>;

	class CDynaNodeContainer : public CDeviceContainer
	{
	protected:
		struct BranchNodes
		{
			CDynaNodeBase *pNodeIp;
			CDynaNodeBase *pNodeIq;
		};

		void BuildSynchroZones();
		void EnergizeZones(ptrdiff_t &nDeenergizedCount, ptrdiff_t &nEnergizedCount);
		void CreateSuperNodes();
		void CreateSuperNodesStructure();
		void CalculateSuperNodesAdmittances(bool bFixNegativeZs = false);
		void PrepareLFTopology();
		void GetTopologySynchroZones(NODEISLANDMAP& NodeIslands);
		void SwitchOffDanglingNode(CDynaNodeBase *pNode, NodeSet& Queue);
		void SwitchOffDanglingNodes(NodeSet& Queue);
		void CalcAdmittances(bool bFixNegativeZs);
		void SwitchLRCs(bool bSwitchToDynamicLRC);
		_IterationControl m_IterationControl;
		void DumpIterationControl();
		std::string GetIterationControlString();
		friend class CLoadFlow;
		LINKSVEC m_SuperLinks;
		ORIGINALLINKSVEC m_OriginalLinks;
		std::unique_ptr<BranchNodes[]> m_pOriginalBranchNodes;
		void ClearSuperLinks();
		void DumpNodeIslands(NODEISLANDMAP& Islands);
		void DumpNetwork();
		std::unique_ptr<VirtualBranch[]> m_pVirtualBranches;
		std::unique_ptr<VirtualZeroBranch[]> m_pZeroBranches;
		VirtualZeroBranch *m_pZeroBranchesEnd = nullptr;
		CDeviceContainer *m_pSynchroZones = nullptr;
		NodeSet m_TopologyCheck;
	public:
		const VirtualZeroBranch* GetZeroBranchesEnd() const noexcept { return m_pZeroBranchesEnd; }
		CDynaNodeBase* FindGeneratorNodeInSuperNode(CDevice *pGen);
		void CalculateShuntParts();
		CMultiLink& GetCheckSuperLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		void GetNodeIslands(NODEISLANDMAP& JoinableNodes, NODEISLANDMAP& Islands);
		NODEISLANDMAPITRCONST GetNodeIsland(CDynaNodeBase* const pNode, const NODEISLANDMAP& Islands);
		_IterationControl& IterationControl();
		CDynaNodeContainer(CDynaModel *pDynaModel);
		virtual ~CDynaNodeContainer();
		void ProcessTopology();
		void ProcessTopologyInitial();
		bool Seidell(); 
		bool LULF();
		void ProcessTopologyRequest();
		void AddToTopologyCheck(CDynaNodeBase* pNode);
		void ResetTopologyCheck();
		bool m_bDynamicLRC = true;
		void LinkToLRCs(CDeviceContainer& containerLRC);
		void LinkToReactors(CDeviceContainer& containerReactors);
	};
}

