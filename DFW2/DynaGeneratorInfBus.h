#pragma once
#include "DeviceContainer.h"
#include "DynaVoltageSource.h"

namespace DFW2
{
	class CDynaGeneratorInfBus : public CDynaVoltageSource
	{
	protected:
		bool SetUpDelta();
	public:
		virtual cplx GetXofEqs() { return cplx(0,xd1); }
		double Eqs, Delta, xd1;
		CDynaGeneratorInfBus();
		virtual double Xgen();
		virtual cplx Igen(ptrdiff_t nIteration);
		virtual ~CDynaGeneratorInfBus() {}
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual cplx GetEMF() { return polar(Eqs, Delta); }
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool InitExternalVariables(CDynaModel *pDynaModel);
		virtual bool CalculatePower();
		static const CDeviceContainerProperties DeviceProperties();
	};
}

