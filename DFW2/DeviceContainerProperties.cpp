#include "stdafx.h"
#include "DeviceContainerProperties.h"
using namespace DFW2;


const char* CDeviceContainerProperties::GetVerbalClassName() const noexcept
{
	return ClassName_.c_str();
}
const char* CDeviceContainerProperties::GetSystemClassName() const noexcept
{
	return ClassSysName_.c_str();
}

eDFW2DEVICETYPE CDeviceContainerProperties::GetType() const noexcept
{
	return eDeviceType;
}




