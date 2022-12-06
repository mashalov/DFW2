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

		DEVICEVECTOR LinkedGenerators;
		VariableIndex S;						// переменная состояния - скольжение

		double Mj = 0.0;						// суммарный момент инерции
		bool InfPower = false;					// признак наличия ШБМ
		CSynchroZone();		
		virtual ~CSynchroZone() = default;
		bool Energized = false;					// признак наличия источника напряжения

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel)  override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
	};

#define DFW2_SQRT_EPSILON DFW2_EPSILON

	struct VirtualBranch;
	struct VirtualZeroBranch;

	class CDynaNodeBase : public CDevice
	{
	public:
		using CDevice::CDevice;

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
		

		double Pn,Qn,Pg,Qg;
		double G,B, Gr0, Br0;
		// переменные, используемые для SSE выравниваем на 16 байт
		// и выстраиваем последовательно. К отдельным значениям доступ
		// организуем через ссылки
		alignas(16) double Sload[2],  Sgen[2];
		double& Pnr{ Sload[0] };
		double& Qnr{ Sload[1] };
		double& Pgr{ Sgen[0] };
		double& Qgr{ Sgen[1] };
		// у комплексных чисел вещественная и мнимая части последовательно в векторе по стандарту
		alignas(16) cplx VreVim;		// комплексное напряжение для работы SSE и упрощения выражений
										// это значение синхронизируется с переменными состояния Vre и Vim
										// в Update... и Predict...
		alignas(16) cplx YiiSuper;		// собственная проводимость суперузла
		alignas(16) cplx dLRCLoad;		// расчетные значения прозводных СХН по напряжению
		alignas(16) cplx dLRCGen;			

		

		cplx LRCShuntPart, LRCShuntPartSuper;
		// напряжения, ниже которых в СХН чисто шунтовая характеристика
		double VshuntPartBelow, VshuntPartBelowSuper;
		double Gshunt, Bshunt;
		double Unom;					// номинальное напряжение
		double V0;						// напряжение в начальных условиях (используется для "подтяжки" СХН к исходному режиму)
		double  V0Super;				// максимальное V0 в узлах суперузла для расчета границы шунта СХН (скорее всего не понадобится)
		bool InMetallicSC = false;
		bool LowVoltage = false;		// признак низкого модуля напряжения
		bool SavedLowVoltage = false;	// сохраненный признак низкого напряжения для возврата на предыдущий шаг
		double LRCVicinity = 0.0;		// окрестность сглаживания СХН

		ptrdiff_t LRCLoadFlowId  = 0;	// идентификаторы СХН и ДСХН
		ptrdiff_t LRCTransientId = 0;

	
		CSynchroZone *pSyncZone = nullptr;		// синхронная зона, к которой принадлежит узел
		eLFNodeType eLFNodeType_;
		ptrdiff_t Nr;
		cplx Yii;						// собственная проводимость
		cplx Iconst;					// постоянный ток в узле
		cplx IconstSuper;				// постоянный ток в суперузле
		double Vold;					// модуль напряжения на предыдущей итерации
		CDynaLRC *pLRC = nullptr;		// указатель на СХН узла в динамике
		CDynaLRC *pLRCLF = nullptr;		// указатель на СХН узла в УР
		CDynaLRC *pLRCGen = nullptr;	// СХН для генерации, которая не задана моделями генераторов

		DynaReactors reactors;			// список реакторов

		double LFVref, LFQmin, LFQmax;	// заданный модуль напряжения и пределы по реактивной мощности для УР
		double LFQminGen, LFQmaxGen;	// суммарные ограничения реактивной мощности генераторов в узле
		virtual ~CDynaNodeBase() = default;
		double* GetVariablePtr(ptrdiff_t nVarIndex)  override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex)  override;
		void GetPnrQnr();
		void GetPnrQnrSuper();
		void GetPnrQnr(double Vnode);
		void GetPnrQnrSuper(double Vnode);
		bool AllLRCsInShuntPart(double Vtest);
		void BuildEquations(CDynaModel* pDynaModel)  override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
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
		bool InMatrix() const override;
		void InitNordsiek(CDynaModel* pDynaModel) override;
		void SuperNodeLoadFlow(CDynaModel *pDynaModel);
		void SuperNodeLoadFlowYU(CDynaModel* pDynaModel);
		inline CDynaNodeBase* GetSuperNode() { return pSuperNodeParent ? pSuperNodeParent : this; }
		bool IsDangling();
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
				
		// возвращает ток узла от нагрузки/генерации/шунтов
		cplx GetSelfImbInotSuper(double& Vsq);
		cplx GetSelfImbISuper(double& Vsq);

		inline double GetSelfdPdV() { return -2 * V * YiiSuper.real() + dLRCLoad.real(); }
		inline double GetSelfdQdV() { return  2 * V * YiiSuper.imag() + dLRCLoad.imag(); }

		inline bool IsLFTypePQ() const { return eLFNodeType_ != eLFNodeType::LFNT_PV; }
		inline bool IsLFTypePV() const 
		{ 
			return eLFNodeType_ == eLFNodeType::LFNT_PV || 
				   eLFNodeType_ == eLFNodeType::LFNT_PVQMAX || 
				   eLFNodeType_ == eLFNodeType::LFNT_PVQMIN;
		}
		inline bool IsLFTypeSlack() const { return eLFNodeType_ == eLFNodeType::LFNT_BASE; }

		void SetMatrixRow(ptrdiff_t MatrixRow) noexcept { MatrixRow_ = MatrixRow; }

		VariableIndexExternal GetExternalVariable(std::string_view VarName) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);

		// структура данных для описания потокораспределения
		// внутри суперузла по ветвям с нулевыми сопротивлениями

		struct ZeroLFData
		{
			// индекс узла в матрице потокораспределения суперузла
			ptrdiff_t SuperNodeLFIndex = 0;
			// диагональный элемент Y
			double Yii = 0.0;
			// инъекция из базисного узла
			double SlackInjection = 0.0;
			// переменные состояния индикаторов "напряжений" суперузлов
			VariableIndex vRe, vIm;
			// диапазон виртуальных ветвей, инцидентных узлу
			VirtualBranch* pVirtualBranchesBegin = nullptr;	
			VirtualBranch* pVirtualBranchesEnd = nullptr;
			VirtualBranch* pVirtualZeroBranchesBegin = nullptr;
			VirtualBranch* pVirtualZeroBranchesEnd = nullptr;

			using LFMatrixType = std::vector<CDynaNodeBase*>;

			// все динамические данные для расчета потокораспредения
			// внутри суперузла упаковываем в смартпойинтер для минимизации c/d
			struct ZeroSuperNodeData
			{
				
				// вектор всех ветвей, связывающих узлы суперузла
				// с другими суперузлами - вектор ветвей с сопротивлениями
				// в этом векторе параллельные ветви будут эквивалентированы
				std::unique_ptr<VirtualBranch[]>  pVirtualBranches, pVirtualZeroBranches;
				// строки матрицы собраны в векторе
				LFMatrixType LFMatrix;
				ptrdiff_t nZcount = 0;
			};
			std::unique_ptr<ZeroSuperNodeData> ZeroSupeNode;
		} 
		ZeroLF;

		// Создать постоянные данные для расчета потокораспределения с нулевыми сопротивлениями
		void CreateZeroLoadFlowData();
		// Включить суперузел в расчет потокораспределения с нулевыми сопротивлениями
		void RequireSuperNodeLF();
		// указатель на родительский суперузел
		CDynaNodeBase* pSuperNodeParent = nullptr;
		const CLinkPtrCount* const GetSuperLink(ptrdiff_t nLinkIndex);

		VirtualBranch	  *pVirtualBranchBegin_, *pVirtualBranchEnd_;
		VirtualZeroBranch *pVirtualZeroBranchBegin_, *pVirtualZeroBranchEnd_, *pVirtualZeroBranchParallelsBegin_;

		VirtualZeroBranch* AddZeroBranch(CDynaBranch* pBranch);
		void TidyZeroBranches();
		// выбирает исходное напряжение либо равное расчетному, либо (если расчетное равно почему-то нулю), номинальному
		inline void PickV0() noexcept { V0 = (V > 0) ? static_cast<double>(V) : Unom; }
		void UpdateSerializer(CSerializerBase* Serializer) override;

		void AddToTopologyCheck();

		static constexpr const char* m_cszV = "V";
		static constexpr const char* m_cszDelta = "Delta";
		static constexpr const char* m_cszVre = "Vre";
		static constexpr const char* m_cszVim = "Vim";
		static constexpr const char* m_cszGsh = "gsh";
		static constexpr const char* m_cszBsh = "bsh";
		static constexpr const char* m_cszPload = "pn";
		static constexpr const char* m_cszLFNodeTypeNames[5] = { "Slack", "Load", "Gen", "GenMax", "GenMin" };

	protected:
		// Рассчитать полную информацию о потоках в ветвях по рассчитанным взаимным потокам
		void CalculateZeroLFBranches();
		// Обновить данные из родительского суперузла
		void FromSuperNode();
		// Вывести данные об изменении модуля напряжения выше/ниже порогового
		void SetLowVoltage(bool bLowVoltage);
		// Рассчитать шаг до изменения модуля напряжения узла выше/ниже порогового
		double FindVoltageZC(CDynaModel *pDynaModel, const RightVector *pRvre, const RightVector *pRvim, double Hyst, bool bCheckForLow);
	};

	class CDynaNodeMeasure;

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

		CDynaNodeMeasure* pMeasure = nullptr;

		VariableIndex Lag;
		VariableIndex S;
		//double Sip;
		//double Cop;
		//double Sv, Dlt;
		CDynaNode();
		virtual ~CDynaNode() = default;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		void Predict()  override;			// допонительная обработка прогноза
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel)  override;
		eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice *pCauseDevice = nullptr)  override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel)  override;
		void UpdateSerializer(CSerializerBase* Serializer) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);


		static constexpr const char* m_cszS = "S";
		static constexpr const char* m_cszSz = "Sz";
	};

	// "виртуальная" ветвь для узла. Заменяет собой настоящую включенную ветвь или несколько
	// включенных параллельных ветвей эквивалентным Y. Эти два параметра - все что нужно
	// чтобы рассчитать небаланс узла и производные для смежных узлов
	struct VirtualBranch
	{
		alignas(16) cplx Y;
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
		size_t PVSwitchCount = 0;												// счетчик переключений PV-PQ
		alignas(16) double ImbS[2];												// небалансы по P и Q
		double& ImbP{ ImbS[0] };
		double& ImbQ{ ImbS[1] };
		double ImbSquare = 0.0;													// квадрат модуля небаланса
		bool bVisited = false;													// признак просмотра для графовых алгоритмов
		double LFQmin;															// исходные ограничения реактивной мощности до ввода в суперузел
		double LFQmax;
		double NodeVoltageViolation_;											// отклонение напряжения от уставки
		double NodePowerViolation_;												// отклонение мощности от ограничения

		double NodeVoltageViolation()
		{
			if (pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_BASE ||
				pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PQ)
				throw dfw2error(fmt::format("Attempt to get node voltage violation from non generator node {}", pNode->GetVerbalName()));

			return NodeVoltageViolation_ = (pNode->V - pNode->LFVref) / pNode->LFVref;
		}

		CDynaNodeBase::eLFNodeType LFNodeType;									// исходный тип узла до ввода в суперузел
		alignas(16) double UncontrolledP = 0.0;
		alignas(8)  double UncontrolledQ = 0.0;									// постоянные значения активной и реактивной генерации в суперузле

		void Store(CDynaNodeBase *pStoreNode) noexcept
		{
			pNode = pStoreNode;
			LFQmin = pNode->LFQmin;
			LFQmax = pNode->LFQmax;
			LFNodeType = pNode->eLFNodeType_;
		}
		void Restore() noexcept
		{
			pNode->LFQmin = LFQmin;
			pNode->LFQmax = LFQmax;
			pNode->eLFNodeType_ = LFNodeType;
		}
	};

	using QUEUE = std::list<_MatrixInfo*>;
	using MATRIXINFO = std::vector<_MatrixInfo*>;
	using NodeQueue = std::queue<CDynaNodeBase*>;
	using NodeSet = std::set<CDynaNodeBase*>;
	using NODEISLANDMAP = std::map<CDynaNodeBase*, NodeSet> ;
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
		_IterationControl IterationControl_;
		// вывод данных о ходе итерационного процесса (УР или YU) с возможностью выбрать уровень вывода:
		// для УР - информация, для YU - отладка
		void DumpIterationControl(DFW2MessageStatus OutputStatus = DFW2MessageStatus::DFW2LOG_INFO);
		std::string GetIterationControlString();
		friend class CLoadFlow;
		LINKSVEC SuperLinks;
		ORIGINALLINKSVEC OriginalLinks;
		std::unique_ptr<BranchNodes[]> pOriginalBranchNodes;
		void ClearSuperLinks();
		void DumpNodeIslands(NODEISLANDMAP& Islands);
		void DumpNetwork();
		std::unique_ptr<VirtualBranch[]> pVirtualBranches;
		std::unique_ptr<VirtualZeroBranch[]> pZeroBranches;
		VirtualZeroBranch *pZeroBranchesEnd_ = nullptr;
		CDeviceContainer *pSynchroZones = nullptr;
		NodeSet TopologyCheck, ZeroLFSet;
	public:
		const VirtualZeroBranch* GetZeroBranchesEnd() const noexcept { return pZeroBranchesEnd_; }
		const NodeSet& GetZeroLFSet() const { return ZeroLFSet; }
		CDynaNodeBase* FindGeneratorNodeInSuperNode(CDevice *pGen);
		void CalculateShuntParts();
		CMultiLink& GetCheckSuperLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		void GetNodeIslands(NODEISLANDMAP& JoinableNodes, NODEISLANDMAP& Islands);
		inline _IterationControl& IterationControl() { return IterationControl_; }
		CDynaNodeContainer(CDynaModel *pDynaModel);
		virtual ~CDynaNodeContainer();
		void ProcessTopology();
		void ProcessTopologyInitial();
		bool LULF();
		void ProcessTopologyRequest();
		void AddToTopologyCheck(CDynaNodeBase* pNode);
		void ResetTopologyCheck();
		bool DynamicLRC = true;
		void LinkToLRCs(CDeviceContainer& containerLRC);
		void LinkToReactors(CDeviceContainer& containerReactors);
		void RequireSuperNodeLF(CDynaNodeBase *pNode);
	};
}

