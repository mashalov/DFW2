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
			V_S,
			V_LAST
		};

		DEVICEVECTOR m_LinkedGenerators;

		double S;
		double Mj;
		bool m_bPassed;
		bool m_bInfPower;
		CSynchroZone();
		virtual ~CSynchroZone() {}
		bool m_bEnergized;
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		void Clear();

		static const CDeviceContainerProperties DeviceProperties();
	};


	class CDynaNodeBase : public CDevice
	{
	public:

		void UpdateVreVim();

		enum CONSTVARS
		{
			C_GSH,
			C_BSH
		};

		enum VARS
		{
			V_DELTA,
			V_V,
			V_LAST
		};

		double Delta;
		double V;
				
		double Pn,Qn,Pg,Qg,Pnr,Qnr;
		double G,B, Gr0, Br0;
		double Gshunt, Bshunt;
		double Unom;
		double V0;
		bool m_bInMetallicSC;

		double dLRCVicinity;

		double dLRCPn;
		double dLRCQn;


		CSynchroZone *m_pSyncZone;
		ptrdiff_t m_nZoneDistance;

		ptrdiff_t Nr;
		ptrdiff_t Type;
		cplx Yii;
		cplx VreVim;
		double Vold;
		CDynaLRC *m_pLRC;
		//double m_dLRCKdef;
		CDynaNodeBase();
		virtual ~CDynaNodeBase();
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		virtual void Predict();
		void GetPnrQnr();
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool NewtonUpdateEquation(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		void MarkZoneEnergized();
		void ProcessTopologyRequest();
		void CalcAdmittances();
		void CalcAdmittances(bool bSeidell);

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
		enum VARS
		{
			V_LAG = 2,
			V_S,
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
	public:
		CDynaNodeContainer(CDynaModel *pDynaModel);
		virtual ~CDynaNodeContainer();
		bool ProcessTopology();
		bool Seidell(); 
		bool LULF();
		void ProcessTopologyRequest();
	};
}

