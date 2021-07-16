﻿#pragma once
#include "DynaPowerInjector.h"
namespace DFW2
{
	class CDynaVoltageSource : public CDynaPowerInjector
	{
	public:
		using CDynaPowerInjector::CDynaPowerInjector;
		virtual ~CDynaVoltageSource() = default;

		virtual cplx GetEMF() = 0;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}

