#pragma once
#include "DynaPowerInjector.h"
namespace DFW2
{
	class CDynaVoltageSource : public CDynaPowerInjector
	{
	public:
		using CDynaPowerInjector::CDynaPowerInjector;
		virtual ~CDynaVoltageSource() = default;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		virtual cplx GetEMF() = 0;		// комплекс ЭДС источника напряжения
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}

