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
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		void CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn) override;
		double Tm_;		// константы из модели Мустанг, которые имеет смысл
		double Txm_;	// рассчитать один раз
	public:

		using CDynaGenerator3C::CDynaGenerator3C;
		virtual ~CDynaGeneratorMustang() = default;

		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		const cplx& CalculateEgen() override;

		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}
