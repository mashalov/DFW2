#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGenerator3C.h"

namespace DFW2
{
	class CDynaGeneratorMustang : public CDynaGenerator3C
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		cplx GetXofEqs() override { return cplx(0, xq); }
	public:

		CDynaGeneratorMustang();
		virtual ~CDynaGeneratorMustang() {}

		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		const cplx& CalculateEgen() override;

		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}
