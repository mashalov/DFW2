#pragma once
#include "DeviceContainer.h"
#include <array>
#include <algorithm>

namespace DFW2
{
	class CDynaReactor : public CDevice
	{
	public:
		using CDevice::CDevice;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		double b, g;
		ptrdiff_t HeadNode, TailNode, ParrBranch;
		ptrdiff_t Type, Placement;

	protected:
		void UpdateSerializer(CSerializerBase* Serializer) override;
	};


	using DynaReactors = std::list<const CDynaReactor*>;


	class CDynaReactorContainer : public CDeviceContainer
	{
	protected:
	public:
		using CDeviceContainer::CDeviceContainer;
		virtual ~CDynaReactorContainer() = default;
		void CreateFromSerialized();
	};
}

