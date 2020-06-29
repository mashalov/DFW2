#pragma once

#include "..\dfw2\Header.h"
#include "..\dfw2\Messages.h"
#include "Compressor.h"
#include "vector"
#include "UnicodeSCSU.h"
#include "Process.h"
#include "SlowVariableItem.h"
#include "io.h"
#include "RLECompressor.h"

namespace DFW2
{
	class CResultFile
	{
	protected:
		FILE *m_pFile = NULL;
	public:
		void WriteLEB(unsigned __int64 nValue);
		void WriteString(std::wstring_view cszString);
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
			std::wstring Name;
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
			std::wstring Name;
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
			const CResultFileReader *m_pFileReader = nullptr;
			DEVICESSET m_DevSet;
			std::wstring strDevTypeName;

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
		int ReadBlockType() const;
		void GetBlocksOrder(INT64LIST& Offsets, unsigned __int64 LastBlockOffset) const;
		void ReadModelData(std::unique_ptr<double[]>& pData, int nVarIndex);
		__int64 OffsetFromCurrent() const;
		std::wstring m_strFilePath;
		std::wstring m_strComment;
		bool m_bHeaderLoaded = false;

		DEVTYPESET m_DevTypeSet;
		CHANNELSET m_ChannelSet;

		std::unique_ptr<DeviceTypeInfo[]> m_pDeviceTypeInfo;

		VARNAMEMAP m_VarNameMap;

		CSlowVariablesSet m_setSlowVariables;

		__int64 m_nCommentOffset;
		__int64 m_nCommentDirectoryOffset;
		__int64 m_DirectoryOffset;

		std::wstring m_strUserComment;

		CRLECompressor m_RLECompressor;
		double m_dRatio = -1.0;

	public:
		virtual ~CResultFileReader();
		void OpenFile(const _TCHAR *cszFilePath);
		void Reparent();
		void ReadHeader(int& Version);
		void ReadLEB(unsigned __int64& nValue) const;
		int ReadLEBInt() const;
		void ReadString(std::wstring& String);
		void ReadDouble(double& Value);
		void ReadDirectoryEntries();
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR* cszVarName) const;
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex) const;
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t nIndex) const;
		std::unique_ptr<double[]> GetTimeStep();
		ptrdiff_t GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex) const;
		ptrdiff_t GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR *cszVarName) const;
		SAFEARRAY* CreateSafeArray(std::unique_ptr<double[]>& pChannelData) const;
		double GetFileTime();
		const _TCHAR* GetFilePath();
		const _TCHAR* GetComment();
		size_t GetPointsCount() const;
		size_t GetChannelsCount() const;
		void GetTimeScale(double *pTimeBuffer, size_t nPointsCount) const;
		void GetStep(double *pStepBuffer, size_t nPointsCount) const;
		void Close();
		int GetVersion();
		const DEVTYPESET& GetTypesSet() const;
		const ChannelHeaderInfo* GetChannelHeaders() const;
		const _TCHAR* GetUnitsName(ptrdiff_t eUnitsType) const;
		const CSlowVariablesSet& GetSlowVariables() { return m_setSlowVariables; }
		const _TCHAR* GetUserComment();
		void SetUserComment(const _TCHAR* cszUserComment);
		double GetCompressionRatio();
		static const _TCHAR* m_cszUnknownUnits;
	};
}
