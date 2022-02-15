#include "stdafx.h"
#include "DeviceTypeInfo.h"
#include "FileExceptions.h"

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
	VarTypeInfo.Index = VarTypes_.size();
	if (VarTypes_.insert(VarTypeInfo).second)
		VarTypesList_.push_back(VarTypeInfo);
	else
		throw dfw2error(fmt::format(CDFW2Messages::m_cszDuplicatedVariableName, VarTypeInfo.Name, DevTypeName_));
}

void DeviceTypeInfo::AddDevice(const std::string_view DeviceName,
	const ResultIds& DeviceIds,
	const ResultIds& ParentIds,
	const ResultIds& ParentTypes)
{
	auto CurrentDevice{ pDeviceInstances_.get() + CurrentInstanceIndex };
	if (CurrentInstanceIndex < static_cast<size_t>(DevicesCount))
	{
		CurrentDevice->Index_ = static_cast<ptrdiff_t>(CurrentInstanceIndex);
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
	DeviceInstanceInfo* pb{ pDeviceInstances_.get() };
	const DeviceInstanceInfo* const pe{ pb + DevicesCount };

	while (pb < pe)
	{
		bool bInserted = DevSet_.insert(pb).second;
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

		_ASSERTE(!pDeviceInstances_);
		pDeviceInstances_ = std::make_unique<DeviceInstanceInfo[]>(DevicesCount);

		DeviceInstanceInfo* pb{ pDeviceInstances_.get() };
		const DeviceInstanceInfo* const pe{ pb + DevicesCount };

		while (pb < pe)
		{
			pb->pDevType_= this;
			pb++;
		}
	}
}

DeviceInstanceInfo::DeviceInstanceInfo(struct DeviceTypeInfo* pDevTypeInfo) : pDevType_(pDevTypeInfo) {}

void DeviceInstanceInfo::SetId(ptrdiff_t IdIndex, ptrdiff_t Id)
{
	if (IdIndex >= 0 && IdIndex < pDevType_->DeviceIdsCount)
	{
		_ASSERTE(Index_ * pDevType_->DeviceIdsCount + IdIndex < pDevType_->DeviceIdsCount* pDevType_->DevicesCount);
		pDevType_->pIds[Index_ * pDevType_->DeviceIdsCount + IdIndex] = Id;
	}
	else
		throw CFileReadException(CDFW2Messages::m_cszWrongResultFile);
}

ptrdiff_t DeviceInstanceInfo::GetId(ptrdiff_t IdIndex) const
{
	if (IdIndex >= 0 && IdIndex < pDevType_->DeviceIdsCount)
		return pDevType_->pIds[Index_ * pDevType_->DeviceIdsCount + IdIndex];
	else
		throw CFileReadException(CDFW2Messages::m_cszWrongResultFile);
}

void DeviceInstanceInfo::SetParent(ptrdiff_t ParentIndex, ptrdiff_t eParentType, ptrdiff_t ParentId)
{
	if (ParentIndex >= 0 && ParentIndex < pDevType_->DeviceParentIdsCount)
	{
		DeviceLinkToParent* pLink{ pDevType_->pLinks.get() + Index_ * pDevType_->DeviceParentIdsCount + ParentIndex };
		_ASSERTE(pLink < pDevType_->pLinks.get() + pDevType_->DeviceParentIdsCount * pDevType_->DevicesCount);
		pLink->eParentType = eParentType;
		pLink->Id = ParentId;
	}
	else
		throw CFileReadException(CDFW2Messages::m_cszWrongResultFile);
}

const DeviceLinkToParent* DeviceInstanceInfo::GetParent(ptrdiff_t ParentIndex) const
{
	if (pDevType_->DeviceParentIdsCount)
	{
		if (ParentIndex >= 0 && ParentIndex < pDevType_->DeviceParentIdsCount)
			return pDevType_->pLinks.get() + Index_ * pDevType_->DeviceParentIdsCount + ParentIndex;
		else
			throw CFileReadException(CDFW2Messages::m_cszWrongResultFile);
	}
	else return nullptr;
}
