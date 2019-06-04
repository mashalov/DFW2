#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorInfBus.h"

namespace DFW2
{
	class CDynaGeneratorMotion : public CDynaGeneratorInfBus
	{
	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
	public:
		enum VARS
		{
			V_S = 2,
			V_DELTA,
			V_LAST
		};

		enum CONSTVARS
		{
			C_UNOM = CDynaGeneratorInfBus::CONSTVARS::C_LAST,
			C_LAST
		};


		double s;
		double	Unom, Kdemp,xq, Mj, Pt, Pnom, cosPhinom;

		CDynaGeneratorMotion();
		virtual ~CDynaGeneratorMotion() {}

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		static double ZeroGuardSlip(double Omega) { return (Omega > 0) ? Omega : DFW2_EPSILON; }
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);

		static const CDeviceContainerProperties DeviceProperties();

		static const _TCHAR *m_cszUnom;
	};
}

