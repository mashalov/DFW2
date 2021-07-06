#pragma once
#include "DeviceTypeInfo.h"
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
#pragma pack(push,1)
	struct DataDirectoryEntry
	{
		int m_DataType;
		int64_t m_Offset;
	};
#pragma pack(pop)

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
		int m_DevTypesCount = 0;
		size_t m_nDirectoryEntriesCount = 0;
		

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

#ifdef _MSC_VER
		CSlowVariablesSet m_setSlowVariables;
#endif		

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
#ifdef _MSC_VER		
		const CSlowVariablesSet& GetSlowVariables() { return m_setSlowVariables; }
#endif		
		const char* GetUserComment();
		void SetUserComment(std::string_view UserComment);
		double GetCompressionRatio();
	};
}
