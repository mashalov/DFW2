#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorInfBus.h"

namespace DFW2
{
	class CDynaGeneratorMotion : public CDynaGeneratorInfBusBase
	{
	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
	public:
		enum VARS
		{
			V_S = CDynaGeneratorInfBusBase::V_LAST,
			V_DELTA,
			V_LAST
		};

		enum CONSTVARS
		{
			C_UNOM = CDynaGeneratorInfBusBase::CONSTVARS::C_LAST,
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
		virtual void UpdateSerializer(SerializerPtr& Serializer);

		static const CDeviceContainerProperties DeviceProperties();

		static const _TCHAR *m_cszUnom;
	};
}

