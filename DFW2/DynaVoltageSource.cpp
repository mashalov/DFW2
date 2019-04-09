#include "stdafx.h"
#include "DynaVoltageSource.h"
using namespace DFW2;

CDynaVoltageSource::CDynaVoltageSource() : CDynaPowerInjector()
{

}

bool CDynaVoltageSource::InitExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaPowerInjector::InitExternalVariables(pDynaModel);
}

const CDeviceContainerProperties CDynaVoltageSource::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaPowerInjector::DeviceProperties();
	props.SetType(DEVTYPE_VOLTAGE_SOURCE);
	props.nEquationsCount = CDynaVoltageSource::VARS::V_LAST;
	return props;
}