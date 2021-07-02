#pragma once

#include "../DFW2/Header.h"
#include "../DFW2/Messages.h"
#include "Compressor.h"
#include "vector"
#include "UnicodeSCSU.h"
#ifdef _MSC_VER
#include <io.h>
#include <Process.h>
#endif
#include "SlowVariableItem.h"
#include "RLECompressor.h"
#include "IResultWriterABI.h"

namespace DFW2
{
	class CResultFile
	{
	protected:
		CStreamedFile infile;
	public:
		void WriteLEB(uint64_t nValue);
		void WriteString(std::string_view cszString);
		static const char m_cszSignature[6];
	};

#pragma pack(push,1)
	struct DataDirectoryEntry
	{
		int m_DataType;
		int64_t m_Offset;
	};
#pragma pack(pop)


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
			bool operator()(const DeviceInstanceInfoBase *lhs, const DeviceInstanceInfoBase *rhs) const
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
			CResultFileReader *m_pFileReader = nullptr;
			DEVICESSET m_DevSet;
			std::string strDevTypeName;

			void AllocateData();
			void IndexDevices();

			void SetDeviceTypeMetrics(ptrdiff_t DeviceIdsCount, ptrdiff_t ParentIdsCount, ptrdiff_t DevicesCount) override;
			void AddDeviceTypeVariable(const std::string_view VariableName, ptrdiff_t UnitsId, double Multiplier) override;
			void AddDevice(const std::string_view DeviceName,
				const std::vector<ptrdiff_t>& DeviceIds,
				const std::vector<ptrdiff_t>& ParentIds,
				const std::vector<ptrdiff_t>& ParentTypes) override;
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
		using DEVTYPEITRCONST= DEVTYPESET::const_iterator;

	class CResultFileReader : public CResultFile
	{

	public:

		struct ChannelHeaderInfo
		{
			int eDeviceType;
			uint64_t DeviceId;
			int DeviceVarIndex;
			uint64_t LastBlockOffset;
		};

		struct ChannelHeaderComp
		{
			bool operator()(const ChannelHeaderInfo* lhs, const ChannelHeaderInfo* rhs) const
			{
				int64_t DevTypeDiff = lhs->eDeviceType - rhs->eDeviceType;
				if (DevTypeDiff > 0) 	return true;
				if (DevTypeDiff < 0) 	return false;
				DevTypeDiff = lhs->DeviceId - rhs->DeviceId;
				if (DevTypeDiff > 0) 	return true;
				if (DevTypeDiff < 0) 	return false;
				return lhs->DeviceVarIndex < rhs->DeviceVarIndex;
			}
		};


		using CHANNELSET = std::set<ChannelHeaderInfo*, ChannelHeaderComp>;
		using CHANNELSETITR = CHANNELSET::iterator;
		using CHANNELSETITRCONST = CHANNELSET::const_iterator;

	protected:

		std::unique_ptr<ChannelHeaderInfo[]> m_pChannelHeaders;
		std::unique_ptr<DataDirectoryEntry[]> m_pDirectoryEntries;
		std::unique_ptr<double[]> m_pTime, m_pStep;

		int m_nVersion = 0;
		double m_dTimeCreated = 0.0;
		int m_DevTypesCount;
		size_t m_nDirectoryEntriesCount;
		

		size_t m_PointsCount;
		size_t m_ChannelsCount;
		typedef std::list<int64_t> INT64LIST;
		int ReadBlockType();
		void GetBlocksOrder(INT64LIST& Offsets, uint64_t LastBlockOffset);
		void ReadModelData(std::unique_ptr<double[]>& pData, int nVarIndex);
		int64_t OffsetFromCurrent();
		std::string m_strFilePath;
		std::string m_strComment;
		bool m_bHeaderLoaded = false;

		DEVTYPESET m_DevTypeSet;
		CHANNELSET m_ChannelSet;

		std::unique_ptr<DeviceTypeInfo[]> m_pDeviceTypeInfo;

		VARNAMEMAP m_VarNameMap;

		CSlowVariablesSet m_setSlowVariables;

		int64_t m_nCommentOffset;
		int64_t m_nCommentDirectoryOffset;
		int64_t m_DirectoryOffset;

		std::string m_strUserComment;

		CRLECompressor m_RLECompressor;
		double m_dRatio = -1.0;

	public:
		virtual ~CResultFileReader();
		void OpenFile(std::string_view FilePath);
		void Reparent();
		void ReadHeader(int& Version);
		void ReadLEB(uint64_t& nValue);
		int ReadLEBInt();
		void ReadString(std::string& String);
		void ReadDouble(double& Value);
		void ReadDirectoryEntries();
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t eType, ptrdiff_t nId, std::string_view VarName);
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex);
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t nIndex);
		std::unique_ptr<double[]> GetTimeStep();
		ptrdiff_t GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex);
		ptrdiff_t GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, std::string_view VarName);
#ifdef _MSC_VER		
		SAFEARRAY* CreateSafeArray(std::unique_ptr<double[]>&& pChannelData);
#endif		
		double GetFileTime();
		const char* GetFilePath();
		const char* GetComment();
		size_t GetPointsCount();
		size_t GetChannelsCount();
		void GetTimeScale(double *pTimeBuffer, size_t nPointsCount);
		void GetStep(double *pStepBuffer, size_t nPointsCount);
		void Close();
		int GetVersion();
		const DEVTYPESET& GetTypesSet() const;
		const ChannelHeaderInfo* GetChannelHeaders();
		const char* GetUnitsName(ptrdiff_t eUnitsType);
		const CSlowVariablesSet& GetSlowVariables() { return m_setSlowVariables; }
		const char* GetUserComment();
		void SetUserComment(std::string_view UserComment);
		double GetCompressionRatio();
		static const char* m_cszUnknownUnits;
	};
}
