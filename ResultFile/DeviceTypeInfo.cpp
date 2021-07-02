#include "stdafx.h"
#include "DeviceTypeInfo.h"
using namespace DFW2;

void DeviceTypeInfo::SetDeviceTypeMetrics(ptrdiff_t DeviceIdsCount, ptrdiff_t ParentIdsCount, ptrdiff_t DevicesCount)
{
	this->DeviceIdsCount = static_cast<int>(DeviceIdsCount);
	this->DevicesCount = static_cast<int>(DevicesCount);
	this->DeviceParentIdsCount = static_cast<int>(ParentIdsCount);
	AllocateData();
}

void DeviceTypeInfo::AddDeviceTypeVariable(const std::string_view VariableName, ptrdiff_t UnitsId, double Multiplier)
{
	VariableTypeInfo VarTypeInfo;
	VarTypeInfo.eUnits = static_cast<int>(UnitsId);
	VarTypeInfo.Multiplier = Multiplier;
	VarTypeInfo.Name = VariableName;
	VarTypeInfo.nIndex = m_VarTypes.size();
	if (m_VarTypes.insert(VarTypeInfo).second)
		m_VarTypesList.push_back(VarTypeInfo);
	else
		throw dfw2error(fmt::format(CDFW2Messages::m_cszDuplicatedVariableName, VarTypeInfo.Name, strDevTypeName));
}

void DeviceTypeInfo::AddDevice(const std::string_view DeviceName,
	const std::vector<ptrdiff_t>& DeviceIds,
	const std::vector<ptrdiff_t>& ParentIds,
	const std::vector<ptrdiff_t>& ParentTypes)
{
	auto CurrentDevice = m_pDeviceInstances.get() + CurrentInstanceIndex;
	if (CurrentInstanceIndex < DevicesCount)
	{
		CurrentDevice->nIndex = static_cast<ptrdiff_t>(CurrentInstanceIndex);
		CurrentDevice->Name = DeviceName;


		for (int i = 0; i < (std::min)(DeviceIdsCount, static_cast<int>(DeviceIds.size())); i++)
			CurrentDevice->SetId(i, DeviceIds[i]);

		for (int i = 0; i < (std::min)(DeviceParentIdsCount, (std::min)(static_cast<int>(ParentIds.size()), static_cast<int>(ParentTypes.size()))); i++)
			CurrentDevice->SetParent(i, ParentTypes[i], ParentIds[i]);

		CurrentInstanceIndex++;
	}
	else
		throw dfw2error(fmt::format(CDFW2Messages::m_cszTooMuchDevicesInResult,
			CurrentInstanceIndex, DevicesCount));
}

void DeviceTypeInfo::IndexDevices()
{
	DeviceInstanceInfo* pb = m_pDeviceInstances.get();
	DeviceInstanceInfo* pe = pb + DevicesCount;

	while (pb < pe)
	{
		bool bInserted = m_DevSet.insert(pb).second;
		_ASSERTE(bInserted);
		pb++;
	}
}

void DeviceTypeInfo::AllocateData()
{
	if (DevicesCount)
	{
		_ASSERTE(!pLinks);
		if (DeviceParentIdsCount)
			pLinks = std::make_unique<DeviceLinkToParent[]>(DeviceParentIdsCount * DevicesCount);

		_ASSERTE(!pIds);
		pIds = std::make_unique<ptrdiff_t[]>(DeviceIdsCount * DevicesCount);

		_ASSERTE(!m_pDeviceInstances);
		m_pDeviceInstances = std::make_unique<DeviceInstanceInfo[]>(DevicesCount);

		DeviceInstanceInfo* pb = m_pDeviceInstances.get();
		DeviceInstanceInfo* pe = pb + DevicesCount;

		while (pb < pe)
		{
			pb->m_pDevType = this;
			pb++;
		}
	}
}