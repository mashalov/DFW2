#pragma once

#include "../DFW2/Header.h"
#include "../DFW2/Messages.h"
#include "IResultWriterABI.h"
#include "Filewrapper.h"

namespace DFW2
{
	class CResultFile
	{
	protected:
		CStreamedFile infile;
	public:
		void WriteLEB(uint64_t nValue);
		void WriteString(std::string_view cszString);
		static constexpr const char m_cszSignature[6] = { 'R', 'a', 'i', 'd', 'e', 'n' };
	};

	struct VariableTypeInfo
	{
		std::string Name;
		double Multiplier = 0.0;
		int eUnits = 0;
		ptrdiff_t Index = 0;
		bool operator<(const VariableTypeInfo& other) const
		{
			return Name < other.Name;
		}
	};

	using VARTYPESET = std::set<VariableTypeInfo>;
	using VARTYPELIST = std::list<VariableTypeInfo>;

	struct DeviceLinkToParent
	{
		ptrdiff_t eParentType;
		ptrdiff_t Id;
	};

	struct DeviceTypeInfo;

	struct DeviceInstanceInfoBase
	{
		ptrdiff_t Index_ = 0;
		virtual ptrdiff_t GetId(ptrdiff_t IdIndex) const
		{
			return Index_;
		}
	};

	struct DeviceInstanceInfo : public DeviceInstanceInfoBase
	{
		DeviceTypeInfo* pDevType_;
		std::string Name;
		DeviceInstanceInfo(DeviceTypeInfo* pDevTypeInfo);
		DeviceInstanceInfo() : pDevType_(nullptr) {}
		void SetId(ptrdiff_t IdIndex, ptrdiff_t Id);
		virtual ptrdiff_t GetId(ptrdiff_t IdIndex) const;
		void SetParent(ptrdiff_t ParentIndex, ptrdiff_t eParentType, ptrdiff_t ParentId);
		const struct DeviceLinkToParent* GetParent(ptrdiff_t ParentIndex) const;
	};

	struct DeviceInstanceCompare
	{
		bool operator()(const DeviceInstanceInfoBase* lhs, const DeviceInstanceInfoBase* rhs) const
		{
			return lhs->GetId(0) < rhs->GetId(0);
		}
	};

	using DEVICESSET = std::set<DeviceInstanceInfoBase*, DeviceInstanceCompare>;

	class CResultFileReader;

	struct DeviceTypeInfo : public IDeviceTypeABI
	{
		int eDeviceType = 0;
		int DeviceIdsCount = 0;
		int DeviceParentIdsCount = 0;
		int DevicesCount = 0;
		int VariablesByDeviceCount = 0;
		VARTYPESET VarTypes_;
		VARTYPELIST VarTypesList_;
		std::unique_ptr<ptrdiff_t[]> pIds;
		std::unique_ptr<DeviceLinkToParent[]> pLinks;
		std::unique_ptr<DeviceInstanceInfo[]> pDeviceInstances_;
		size_t CurrentInstanceIndex = 0;
		CResultFileReader* pFileReader_ = nullptr;
		DEVICESSET DevSet_;
		std::string DevTypeName_;

		void AllocateData();
		void IndexDevices();

		void SetDeviceTypeMetrics(ptrdiff_t DeviceIdsCount, ptrdiff_t ParentIdsCount, ptrdiff_t DevicesCount) override;
		void AddDeviceTypeVariable(const std::string_view VariableName, ptrdiff_t UnitsId, double Multiplier) override;
		void AddDevice(const std::string_view DeviceName,
			const ResultIds& DeviceIds,
			const ResultIds& ParentIds,
			const ResultIds& ParentTypes) override;
	};

	struct DeviceTypesComp
	{
		bool operator()(const DeviceTypeInfo* lhs, const DeviceTypeInfo* rhs) const
		{
			return lhs->eDeviceType < rhs->eDeviceType;
		}
	};

	using DEVTYPESET = std::set<DeviceTypeInfo*, DeviceTypesComp>;
}
