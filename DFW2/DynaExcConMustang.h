#pragma once
#include "DynaExciterBase.h"
#include "DerlagContinuous.h"

namespace DFW2
{

	class CDynaExcConMustang : public CDevice
	{

	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		CLimitedLag Lag;

		CDerlag dVdt;
		CDerlag dEqdt;
		CDerlag dSdt;

		PrimitiveVariable LagIn;

		PrimitiveVariableExternal dVdtIn;
		PrimitiveVariableExternal dEqdtIn;
		PrimitiveVariableExternal dSdtIn;

		double dVdtOutValue[2], dEqdtOutValue[2], dSdtOutValue[2];

		CPrimitives<4> Prims;

	public:

		enum VARS
		{
			V_UF,
			V_USUM,
			V_SVT,
			V_DVDT,
			V_EQDT = V_DVDT + 2,
			V_SDT = V_EQDT + 2,
			V_LAST = V_SDT + 2
		};

		double K0u, K1u, Alpha, K1if, K0f, K1f, Umin, Umax, Tf, Tr;
		double Vref;
		double Usum, Uf, Svt;

		CDynaExcConMustang();
		virtual ~CDynaExcConMustang() {}
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);

		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);

		static const CDeviceContainerProperties DeviceProperties();
	};
}

