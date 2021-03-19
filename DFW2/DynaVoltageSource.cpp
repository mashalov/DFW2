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

void CDynaVoltageSource::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaPowerInjector::DeviceProperties(props);
	props.SetType(DEVTYPE_VOLTAGE_SOURCE);
	props.nEquationsCount = CDynaVoltageSource::VARS::V_LAST;
}
