﻿#include "stdafx.h"
#include "DynaVoltageSource.h"
using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaVoltageSource::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaPowerInjector::UpdateExternalVariables(pDynaModel);
}

void CDynaVoltageSource::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaPowerInjector::DeviceProperties(props);
	props.SetType(DEVTYPE_VOLTAGE_SOURCE);
	props.EquationsCount = CDynaVoltageSource::VARS::V_LAST;
}

eDEVICEFUNCTIONSTATUS CDynaVoltageSource::InitModel(CDynaModel* pDynaModel)
{
	return CDynaPowerInjector::InitModel(pDynaModel);
}