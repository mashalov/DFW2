#pragma once
#include "DynaPowerInjector.h"
namespace DFW2
{
	class CDynaVoltageSource : public CDynaPowerInjector
	{
	public:
		CDynaVoltageSource();
		virtual cplx GetEMF() = 0;
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);

		static const CDeviceContainerProperties DeviceProperties();
	};
}

