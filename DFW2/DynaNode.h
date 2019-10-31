#pragma once
#include "DeviceContainer.h"
#include "DynaLRC.h"
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

		double S = 0.0;							// переменная состояния - скольжение
		double Mj = 0.0;						// суммарный момент инерции
		bool m_bInfPower = false;				// признак наличия ШБМ
		CSynchroZone();		
		virtual ~CSynchroZone() {}
		bool m_bEnergized = false;				// признак наличия источника напряжения

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		static const CDeviceContainerProperties DeviceProperties();
	};

#define DFW2_SQRT_EPSILON DFW2_EPSILON

	struct VirtualBranch;

	class CDynaNodeBase : public CDevice
	{
	public:

		void UpdateVreVim();
		void UpdateVDelta();
		void UpdateVreVimSuper();
		void UpdateVDeltaSuper();

		enum eLFNodeType
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
			V_DELTA,
			V_V,
			V_RE,
			V_IM,
			V_LAST		// последняя переменная (реально не существует) указывает количество уравнений устройства
		};

		double Delta;
		double V;
		double Vre;
		double Vim;

#ifdef _DEBUG
		double Vrastr, Deltarastr, Qgrastr, Pnrrastr, Qnrrastr;
		void GrabRastrResult()
		{
			Vrastr = V;
			Deltarastr = Delta;
			Qgrastr = Qg;
			Pnrrastr = Pn;
			Qnrrastr = Qn;
		}
#endif
				
		double Pn,Qn,Pg,Qg,Pnr,Qnr,Pgr,Qgr;
		double G,B, Gr0, Br0;
		double Gshunt, Bshunt;
		double Unom;					// номинальное напряжение
		double V0;						// напряжение в начальных условиях (используется для "подтяжки" СХН к исходному режиму)
		bool m_bInMetallicSC = false;
		bool m_bLowVoltage;				// признак низкого модуля напряжения
		bool m_bSavedLowVoltage;		
		double dLRCVicinity = 0.0;		// окрестность сглаживания СХН

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
		double LFVref, LFQmin, LFQmax;	// заданный модуль напряжения и пределы по реактивной мощности для УР
		CDynaNodeBase();
		virtual ~CDynaNodeBase();
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		void GetPnrQnr();
		void GetPnrQnrSuper();
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual void NewtonUpdateEquation(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		void MarkZoneEnergized();
		void ProcessTopologyRequest();
		void CalcAdmittances();
		void CalcAdmittances(bool bSeidell);
		// инициализация узла для расчета УР
		void StartLF(bool bFlatStart, double ImbTol);
		virtual void StoreStates() override;
		virtual void RestoreStates() override;
		virtual bool InMatrix() override;
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		inline double GetSelfImbP() { return Pnr - Pgr - V * V * YiiSuper.real();	}
		inline double GetSelfImbQ() { return Qnr - Qgr + V * V * YiiSuper.imag(); }

		inline double GetSelfdPdV() { return -2 * V * YiiSuper.real() + dLRCPn;	}
		inline double GetSelfdQdV() { return  2 * V * YiiSuper.imag() + dLRCQn; }

		inline bool IsLFTypePQ() { return m_eLFNodeType != eLFNodeType::LFNT_PV; }

		void SetMatrixRow(ptrdiff_t nMatrixRow) { m_nMatrixRow = nMatrixRow; }

		virtual ExternalVariable GetExternalVariable(const _TCHAR* cszVarName);

		static const CDeviceContainerProperties DeviceProperties();

		CDynaNodeBase *m_pSuperNodeParent;
		CLinkPtrCount* GetSuperLink(ptrdiff_t nLinkIndex);

		VirtualBranch *m_VirtualBranchBegin, *m_VirtualBranchEnd;

		static const _TCHAR *m_cszV;
		static const _TCHAR *m_cszDelta;
		static const _TCHAR *m_cszVre;
		static const _TCHAR *m_cszVim;
		static const _TCHAR *m_cszGsh;
		static const _TCHAR *m_cszBsh;

	protected:
		void SetLowVoltage(bool bLowVoltage);
		double FindVoltageZC(CDynaModel *pDynaModel, RightVector *pRvre, RightVector *pRvim, double Hyst, bool bCheckForLow);
	};

	class CDynaNode : public CDynaNodeBase
	{
	public:
		// здесь переменные состояния добавляются к базовому классу
		enum VARS
		{
			V_LAG = CDynaNodeBase::V_LAST,			// дифференциальное уравнение лага скольжения в третьей строке
			V_S,									// скольжение
	//		V_SIP,
	//		V_COP,
	//		V_SV,
			V_LAST
		};

		double Lag;
		//double Sip;
		//double Cop;
		double S;
		//double Sv, Dlt;
		CDynaNode();
		virtual ~CDynaNode() {}
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual void Predict();			// допонительная обработка прогноза
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);

		static const CDeviceContainerProperties DeviceProperties();

		static const _TCHAR *m_cszS;
		static const _TCHAR *m_cszSz;
	};

	// "виртуальная" ветвь для узла. Заменяет собой настоящую включенную ветвь или несколько
	// включенных параллельных ветвей эквивалентным Y. Эти два параметра - все что нужно
	// чтобы рассчитать небаланс узла и производные для смежных узлов
	struct VirtualBranch
	{
		cplx Y;
		CDynaNodeBase *pNode;
	};

	// маппинг узла в строки матрица
	struct _MatrixInfo
	{
		size_t nRowCount = 0;													// количество элементов в строке матрицы
		CDynaNodeBase *pNode;													// узел, к которому относится данное Info
		ptrdiff_t m_nPVSwitchCount = 0;											// счетчик переключений PV-PQ
		double m_dImbP, m_dImbQ;												// небалансы по P и Q
		bool bVisited = false;													// признак просмотра для графовых алгоритмов
	};

	typedef vector<_MatrixInfo*> MATRIXINFO;
	typedef MATRIXINFO::iterator MATRIXINFOITR;
	typedef list<_MatrixInfo*> QUEUE;

	class _MaxNodeDiff
	{
	protected:
		CDynaNodeBase *m_pNode;
		double m_dDiff;
		typedef bool (OperatorFunc)(double lhs, double rhs);

		void UpdateOp(CDynaNodeBase *pNode, double dValue, OperatorFunc OpFunc)
		{
			if (pNode)
			{
				if (m_pNode)
				{
					if (OpFunc(dValue, m_dDiff))
					{
						m_pNode = pNode;
						m_dDiff = dValue;
					}
				}
				else
				{
					m_pNode = pNode;
					m_dDiff = dValue;
				}
			}
			else
				_ASSERTE(pNode);
		}

	public:
		_MaxNodeDiff() : m_pNode(nullptr),
			m_dDiff(0.0)
		{}

		ptrdiff_t GetId()
		{
			if (m_pNode)
				return m_pNode->GetId();
			return -1;
		}

		double GetDiff()
		{
			if (GetId() >= 0)
				return m_dDiff;
			return 0.0;
		}

		void UpdateMin(CDynaNodeBase *pNode, double Value)
		{
			UpdateOp(pNode, Value, [](double lhs, double rhs) -> bool { return lhs < rhs; });
		}

		void UpdateMax(CDynaNodeBase *pNode, double Value)
		{
			UpdateOp(pNode, Value, [](double lhs, double rhs) -> bool { return lhs > rhs; });
		}

		void UpdateMaxAbs(CDynaNodeBase *pNode, double Value)
		{
			UpdateOp(pNode, Value, [](double lhs, double rhs) -> bool { return fabs(lhs) > fabs(rhs); });
		}
	};

	struct _IterationControl
	{
		_MaxNodeDiff m_MaxImbP;
		_MaxNodeDiff m_MaxImbQ;
		_MaxNodeDiff m_MaxV;
		_MaxNodeDiff m_MinV;
		ptrdiff_t m_nQviolated = 0;
		double m_ImbRatio = 0.0;

		bool Converged(double m_dToleratedImb)
		{
			return fabs(m_MaxImbP.GetDiff()) < m_dToleratedImb && fabs(m_MaxImbQ.GetDiff()) < m_dToleratedImb && m_nQviolated == 0;
		}

		void Reset()
		{
			*this = _IterationControl();
		}

		void Update(_MatrixInfo *pMatrixInfo)
		{
			if (pMatrixInfo && pMatrixInfo->pNode)
			{
				CDynaNodeBase *pNode = pMatrixInfo->pNode;
				m_MaxImbP.UpdateMaxAbs(pNode, pMatrixInfo->m_dImbP);
				m_MaxImbQ.UpdateMaxAbs(pNode, pMatrixInfo->m_dImbQ);
				double VdVnom = pNode->V / pNode->Unom;
				m_MaxV.UpdateMax(pNode, VdVnom);
				m_MinV.UpdateMin(pNode, VdVnom);
			}
			else
				_ASSERTE(pMatrixInfo && pMatrixInfo->pNode);
		}
	};

	using NodeQueue = std::queue<CDynaNodeBase*>;
	using NodeSet = std::set<CDynaNodeBase*>;
	using NODEISLANDMAP = map<CDynaNodeBase*, NodeSet> ;
	using NODEISLANDMAPITRCONST = map<CDynaNodeBase*, NodeSet>::const_iterator;
	using DEVICETODEVICEMAP = map<CDevice*, CDevice*>;
	using ORIGINALLINKSVEC = vector<unique_ptr<DEVICETODEVICEMAP>>;

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
		bool m_bRebuildMatrix;
		bool CreateSuperNodes();
		void PrepareLFTopology();
		void GetTopologySynchroZones(NODEISLANDMAP& NodeIslands);
		void SwitchOffDanglingNode(CDynaNodeBase *pNode, NodeSet& Queue);
		void SwitchOffDanglingNodes(NodeSet& Queue);
		void CalcAdmittances(bool bSeidell);
		void SwitchLRCs(bool bSwitchToDynamicLRC);
		_IterationControl m_IterationControl;
		void DumpIterationControl();
		friend class CLoadFlow;
		LINKSVEC m_SuperLinks;
		ORIGINALLINKSVEC m_OriginalLinks;
		unique_ptr<BranchNodes[]> m_pOriginalBranchNodes;
		void ClearSuperLinks();
		void DumpNodeIslands(NODEISLANDMAP& Islands);
		unique_ptr<VirtualBranch[]> m_pVirtualBranches;
		CDeviceContainer *m_pSynchroZones = nullptr;
	public:
		CDynaNodeBase* FindGeneratorNodeInSuperNode(CDevice *pGen);
		CMultiLink& GetCheckSuperLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		void GetNodeIslands(NODEISLANDMAP& JoinableNodes, NODEISLANDMAP& Islands);
		NODEISLANDMAPITRCONST GetNodeIsland(CDynaNodeBase* const pNode, const NODEISLANDMAP& Islands);
		_IterationControl& IterationControl();
		CDynaNodeContainer(CDynaModel *pDynaModel);
		virtual ~CDynaNodeContainer();
		void ProcessTopology();
		bool Seidell(); 
		bool LULF();
		void ProcessTopologyRequest();
		bool m_bDynamicLRC = true;
	};

}

