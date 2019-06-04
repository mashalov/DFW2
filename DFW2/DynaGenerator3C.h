#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGenerator1C.h"

namespace DFW2
{
	class CDynaGenerator3C : public CDynaGenerator1C
	{
	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual cplx GetXofEqs() { return cplx(0, xq); }
	public:

		enum VARS
		{
			V_EQSS = 10,
			V_EDSS,
			V_LAST
		};

		double Eqss, Edss;
		double Td0ss, Tq0ss;
		double xd2, xq1, xq2;

		CDynaGenerator3C();
		virtual ~CDynaGenerator3C() {}

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual cplx GetEMF() { return cplx(Eqss, Edss) *polar(1.0, Delta); }
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		virtual bool CalculatePower();
		double Xgen();
		virtual const cplx& CalculateEgen();

		static const CDeviceContainerProperties DeviceProperties();
	};
}
