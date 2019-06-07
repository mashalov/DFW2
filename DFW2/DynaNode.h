#pragma once
#include "DeviceContainer.h"
#include "DynaLRC.h"


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

		double S;								// переменная состояния - скольжение
		double Mj;								// суммарный момент инерции
		bool m_bPassed;				
		bool m_bInfPower;						// признак наличия ШБМ
		CSynchroZone();		
		virtual ~CSynchroZone() {}
		bool m_bEnergized;						// признак наличия источника напряжения

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		// функция сброса параметров на исходные значения
		void Clear();	

		static const CDeviceContainerProperties DeviceProperties();
	};


	class CDynaNodeBase : public CDevice
	{
	public:

		void UpdateVreVim();

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
			V_LAST		// последняя переменная (реально не существует) указывает количество уравнений устройства
		};

		double Delta;
		double V;
				
		double Pn,Qn,Pg,Qg,Pnr,Qnr;
		double G,B, Gr0, Br0;
		double Gshunt, Bshunt;
		double Unom;					// номинальное напряжение
		double V0;						// напряжение в начальных условиях (используется для "подтяжки" СХН к исходному режиму)
		bool m_bInMetallicSC;

		double dLRCVicinity;			// окрестность сглаживания СХН

		double dLRCPn;					// расчетные значения прозводных СХН по напряжению
		double dLRCQn;


		CSynchroZone *m_pSyncZone;		// синхронная зона, к которой принадлежит узел
		ptrdiff_t m_nZoneDistance;
		eLFNodeType m_eLFNodeType;
		ptrdiff_t Nr;
		cplx Yii;						// собственная проводимость
		cplx VreVim;					// напряжение в декартовых координатах для упрощения расчета элементов якоби
		double Vold;					// модуль напряжения на предыдущей итерации
		CDynaLRC *m_pLRC;				// указатель на СХН узла
		double LFVref, LFQmin, LFQmax;	// заданный модуль напряжения и пределы по реактивной мощности для УР
		CDynaNodeBase();
		virtual ~CDynaNodeBase();
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		virtual void Predict();			// допонительная обработка прогноза
		void GetPnrQnr();
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool NewtonUpdateEquation(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		void MarkZoneEnergized();
		void ProcessTopologyRequest();
		void CalcAdmittances();
		void CalcAdmittances(bool bSeidell);
		
		inline double GetSelfImbP() { return Pnr - Pg - V * V * Yii.real();	}
		inline double GetSelfImbQ() { return Qnr - Qg + V * V * Yii.imag(); }

		inline double GetSelfdPdV() { return -2 * V * Yii.real() + dLRCPn;	}
		inline double GetSelfdQdV() { return  2 * V * Yii.imag() + dLRCQn; }

		inline bool IsLFTypePQ() { return m_eLFNodeType != eLFNodeType::LFNT_PV; }

		void SetMatrixRow(ptrdiff_t nMatrixRow) { m_nMatrixRow = nMatrixRow; }

		virtual ExternalVariable GetExternalVariable(const _TCHAR* cszVarName);

		static const CDeviceContainerProperties DeviceProperties();

		static const _TCHAR *m_cszV;
		static const _TCHAR *m_cszDelta;
		static const _TCHAR *m_cszGsh;
		static const _TCHAR *m_cszBsh;
	};

	class CDynaNode : public CDynaNodeBase
	{
	public:
		// здесь переменные состояния добавляются к базовому классу
		enum VARS
		{
			V_LAG = 2,			// дифференциальное уравнение лага скольжения в третьей строке
			V_S,				// скольжение
			V_LAST
		};

		double Lag;
		double S;
		CDynaNode();
		virtual ~CDynaNode() {}
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);

		static const CDeviceContainerProperties DeviceProperties();

		static const _TCHAR *m_cszS;
		static const _TCHAR *m_cszSz;
	};

	class CDynaNodeContainer : public CDeviceContainer
	{
	protected:
		CDeviceContainer *m_pSynchroZones;
		CDynaNodeBase* GetFirstNode();
		CDynaNodeBase* GetNextNode();
		CSynchroZone* CreateNewSynchroZone();
		bool BuildSynchroZones();
		bool  EnergizeZones(ptrdiff_t &nDeenergizedCount, ptrdiff_t &nEnergizedCount);
		bool m_bRebuildMatrix;
		void CalcAdmittances(bool bSeidell);
		bool CreateSuperNodes();
	public:
		CDynaNodeContainer(CDynaModel *pDynaModel);
		virtual ~CDynaNodeContainer();
		bool ProcessTopology();
		bool Seidell(); 
		bool LULF();
		void ProcessTopologyRequest();
	};
}

