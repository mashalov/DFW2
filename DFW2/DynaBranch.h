#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"


namespace DFW2
{

	class CDynaBranchMeasure;

	class CDynaBranch : public CDevice
	{
	protected:
		virtual void UpdateVerbalName();
	public:

		double dStateBegin;
		double dStateEnd;

		CDynaBranchMeasure *m_pMeasure;

		ptrdiff_t Ip, Iq, Np;
		double R, X;
		double Ktr, Kti;
		double G, B, GrIp, GrIq, BrIp, BrIq;
		ptrdiff_t NrIp, NrIq;
		CDynaNodeBase *m_pNodeIp, *m_pNodeIq;
		double GIp, BIp, GIq, BIq;
		cplx Yip, Yiq, Yips, Yiqs;

		enum BranchState
		{
			BRANCH_ON,
			BRANCH_OFF,
			BRANCH_TRIPIP,
			BRANCH_TRIPIQ
		} 
			m_BranchState;

		CDynaBranch();
		virtual ~CDynaBranch() {}
		cplx GetYBranch(bool bFixNegativeZ = false);
		virtual bool LinkToContainer(CDeviceContainer *pContainer, CDeviceContainer *pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom);
		virtual eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause);
		virtual eDEVICESTATE GetState();
		eDEVICEFUNCTIONSTATUS SetBranchState(BranchState eBranchState, eDEVICESTATECAUSE eStateCause);
		void CalcAdmittances();
		void CalcAdmittances(bool bSeidell);

		static const CDeviceContainerProperties DeviceProperties();
	};

	class CDynaBranchMeasure : public CDevice
	{
	protected:
		CDynaBranch *m_pBranch;
	public:
		enum VARS
		{
			V_IBRE,
			V_IBIM,
			V_IERE,
			V_IEIM,
			V_IB,
			V_IE,
			V_PB,
			V_QB,
			V_PE,
			V_QE,
			V_SB,
			V_SE,
			V_LAST
		};

		double Ibre, Ibim, Iere, Ieim, Ib, Ie, Pb, Qb, Pe, Qe, Sb, Se;

		CDynaBranchMeasure(CDynaBranch *pBranch);

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);

		static const CDeviceContainerProperties DeviceProperties();
	};
}
