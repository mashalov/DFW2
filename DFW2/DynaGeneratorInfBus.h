#pragma once
#include "DeviceContainer.h"
#include "DynaVoltageSource.h"

namespace DFW2
{
	class CDynaGeneratorInfBusBase : public CDynaVoltageSource
	{
	protected:
		bool SetUpDelta();
	public:
		virtual cplx GetXofEqs() { return cplx(0,xd1); }
		double Eqs, Delta, xd1;
		CDynaGeneratorInfBusBase();
		virtual double Xgen();
		virtual cplx Igen(ptrdiff_t nIteration);
		virtual ~CDynaGeneratorInfBusBase() {}
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual cplx GetEMF() { return polar(Eqs, Delta); }
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		virtual bool CalculatePower();
		static const CDeviceContainerProperties DeviceProperties();
	};

	class CDynaGeneratorInfBus : public CDynaGeneratorInfBusBase
	{
	protected:
		double EqsCos, EqsSin;
	public:
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
	};
}

