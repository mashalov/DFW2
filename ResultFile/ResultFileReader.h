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

namespace DFW2
{
	class CResultFile
	{
	protected:
		CStreamedFile infile;
	public:
		void WriteLEB(unsigned __int64 nValue);
		void WriteString(std::string_view cszString);
		static const char m_cszSignature[6];
	};

#pragma pack(push,1)
	struct DataDirectoryEntry
	{
		int m_DataType;
		__int64 m_Offset;
	};
#pragma pack(pop)
	
	class CResultFileReader : public CResultFile
	{

	public:

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
			struct DeviceInstanceInfo(struct DeviceTypeInfo* pDevTypeInfo);
			struct DeviceInstanceInfo() : m_pDevType(nullptr) {}
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


		struct DeviceTypeInfo
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
			CResultFileReader *m_pFileReader = nullptr;
			DEVICESSET m_DevSet;
			std::string strDevTypeName;

			void AllocateData();
			void IndexDevices();
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

		struct ChannelHeaderInfo
		{
			int eDeviceType;
			unsigned __int64 DeviceId;
			int DeviceVarIndex;
			unsigned __int64 LastBlockOffset;
		};

		struct ChannelHeaderComp
		{
			bool operator()(const ChannelHeaderInfo* lhs, const ChannelHeaderInfo* rhs) const
			{
				__int64 DevTypeDiff = lhs->eDeviceType - rhs->eDeviceType;
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
		typedef std::list<__int64> INT64LIST;
		int ReadBlockType();
		void GetBlocksOrder(INT64LIST& Offsets, unsigned __int64 LastBlockOffset);
		void ReadModelData(std::unique_ptr<double[]>& pData, int nVarIndex);
		__int64 OffsetFromCurrent();
		std::string m_strFilePath;
		std::string m_strComment;
		bool m_bHeaderLoaded = false;

		DEVTYPESET m_DevTypeSet;
		CHANNELSET m_ChannelSet;

		std::unique_ptr<DeviceTypeInfo[]> m_pDeviceTypeInfo;

		VARNAMEMAP m_VarNameMap;

		CSlowVariablesSet m_setSlowVariables;

		__int64 m_nCommentOffset;
		__int64 m_nCommentDirectoryOffset;
		__int64 m_DirectoryOffset;

		std::string m_strUserComment;

		CRLECompressor m_RLECompressor;
		double m_dRatio = -1.0;

	public:
		virtual ~CResultFileReader();
		void OpenFile(std::string_view FilePath);
		void Reparent();
		void ReadHeader(int& Version);
		void ReadLEB(unsigned __int64& nValue);
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
		SAFEARRAY* CreateSafeArray(std::unique_ptr<double[]>&& pChannelData);
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
