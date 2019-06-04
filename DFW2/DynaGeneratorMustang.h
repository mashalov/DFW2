#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGenerator3C.h"

namespace DFW2
{
	class CDynaGeneratorMustang : public CDynaGenerator3C
	{
	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual cplx GetXofEqs() { return cplx(0, xq); }
	public:

		CDynaGeneratorMustang();
		virtual ~CDynaGeneratorMustang() {}

		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		virtual bool CDynaGeneratorMustang::CalculatePower();

		static const CDeviceContainerProperties DeviceProperties();
	};
}
