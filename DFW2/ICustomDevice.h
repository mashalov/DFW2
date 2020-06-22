#pragma once
#include "DeviceContainerPropertiesBase.h"

namespace DFW2
{
	class CustomDeviceData
	{

	};

	class ICustomDevice
	{
	public:
		virtual void Destroy() = 0;
		virtual void BuildRightHand(CustomDeviceData* pData) = 0;
		virtual void GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps) = 0;
		virtual DOUBLEVECTOR& GetConstantData() = 0;
		virtual void SetConstsDefaultValues() = 0;
		virtual VariableIndexVec GetVariables() = 0;
	};

	typedef ICustomDevice* (__cdecl* CustomDeviceFactory)();
}
