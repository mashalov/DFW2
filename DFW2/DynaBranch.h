#pragma once
#include "DeviceContainer.h"
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
		virtual void UpdateVerbalName();
	public:
		// Состояния вначале и в конце ветви
		double dStateBegin;
		double dStateEnd;

		CDynaBranchMeasure *m_pMeasure;

		ptrdiff_t Ip, Iq, Np;						// номера узлов начала и конца, номер парр. цепи
		double R, X;								// сопротивление
		double Ktr, Kti;							// комплексный коэффициент трансформации
		double G, B, GrIp, GrIq, BrIp, BrIq;		// проводимость ветви, проводимости реакторов в начали и в конце
		ptrdiff_t NrIp, NrIq;						// количество реакторов в начале и в конце
		CDynaNodeBase *m_pNodeIp, *m_pNodeIq;		// узлы начала и конца
		double GIp, BIp, GIq, BIq;					// расчетные проводимости в начале и в конце
		cplx Yip, Yiq, Yips, Yiqs;					// компоненты взаимных и собственных проводимостей 
													// для матрицы узловых проводимостей

		// состояние ветви
		enum BranchState	
		{
			BRANCH_ON,				// полностью включена
			BRANCH_OFF,				// полностью отключена
			BRANCH_TRIPIP,			// отключена в начале
			BRANCH_TRIPIQ			// отключена в конце
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

		// переменные состояния
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
