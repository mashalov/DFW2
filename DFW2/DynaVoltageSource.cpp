#include "stdafx.h"
#include "DynaVoltageSource.h"
using namespace DFW2;

CDynaVoltageSource::CDynaVoltageSource() : CDynaPowerInjector()
{

}

eDEVICEFUNCTIONSTATUS CDynaVoltageSource::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaPowerInjector::UpdateExternalVariables(pDynaModel);
}

const CDeviceContainerProperties CDynaVoltageSource::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaPowerInjector::DeviceProperties();
	props.SetType(DEVTYPE_VOLTAGE_SOURCE);
	props.nEquationsCount = CDynaVoltageSource::VARS::V_LAST;
	return props;
}
