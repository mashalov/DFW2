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
		static constexpr const char *m_cszUnknownUnits = "???";
	};

	struct VariableTypeInfo
	{
		std::string Name;
		double Multiplier;
		int eUnits;
		ptrdiff_t nIndex;
		bool operator<(const VariableTypeInfo& other) const
		{
			return Name < other.Name;
		}
	};

	using VARTYPESET = std::set<VariableTypeInfo>;
	using VARTYPEITR = VARTYPESET::iterator;
	using VARTYPEITRCONST = VARTYPESET::const_iterator;
	using VARTYPELIST = std::list<VariableTypeInfo>;
	using VARTYPELISTITRCONST = VARTYPELIST::const_iterator;

	struct DeviceLinkToParent
	{
		ptrdiff_t m_eParentType;
		ptrdiff_t m_nId;
	};

	struct DeviceTypeInfo;

	struct DeviceInstanceInfoBase
	{
		ptrdiff_t nIndex;
		virtual ptrdiff_t GetId(ptrdiff_t nIdIndex) const
		{
			return nIndex;
		}
	};

	struct DeviceInstanceInfo : public DeviceInstanceInfoBase
	{
		DeviceTypeInfo* m_pDevType;
		std::string Name;
		DeviceInstanceInfo(DeviceTypeInfo* pDevTypeInfo);
		DeviceInstanceInfo() : m_pDevType(nullptr) {}
		void SetId(ptrdiff_t nIdIndex, ptrdiff_t nId);
		virtual ptrdiff_t GetId(ptrdiff_t nIdIndex) const;
		void SetParent(ptrdiff_t nParentIndex, ptrdiff_t eParentType, ptrdiff_t nParentId);
		const struct DeviceLinkToParent* GetParent(ptrdiff_t nParentIndex) const;
	};

	struct DeviceInstanceCompare
	{
		bool operator()(const DeviceInstanceInfoBase* lhs, const DeviceInstanceInfoBase* rhs) const
		{
			return lhs->GetId(0) < rhs->GetId(0);
		}
	};

	using DEVICESSET = std::set<DeviceInstanceInfoBase*, DeviceInstanceCompare>;
	using DEVIDITR = DEVICESSET::iterator;
	using DEVIDITRCONST = DEVICESSET::const_iterator;

	class CResultFileReader;

	struct DeviceTypeInfo : public IDeviceTypeABI
	{
		int eDeviceType;
		int DeviceIdsCount;
		int DeviceParentIdsCount;
		int DevicesCount;
		int VariablesByDeviceCount;
		VARTYPESET m_VarTypes;
		VARTYPELIST m_VarTypesList;
		std::unique_ptr<ptrdiff_t[]> pIds;
		std::unique_ptr<DeviceLinkToParent[]> pLinks;
		std::unique_ptr<DeviceInstanceInfo[]> m_pDeviceInstances;
		size_t CurrentInstanceIndex = 0;
		CResultFileReader* m_pFileReader = nullptr;
		DEVICESSET m_DevSet;
		std::string strDevTypeName;

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
	using DEVTYPEITR = DEVTYPESET::iterator;
	using DEVTYPEITRCONST = DEVTYPESET::const_iterator;

}
