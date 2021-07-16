#pragma once
#include "DynaGeneratorMotion.h"

namespace DFW2
{
	class CDynaGeneratorPark3C : public CDynaGeneratorMotion
	{
	public:
		virtual ~CDynaGeneratorPark3C() = default;
		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}

