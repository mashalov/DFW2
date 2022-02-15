/*
* Модуль сжатия результатов расчета электромеханических переходных процессов
* (С) 2018 - Н.В. Евгений Машалов
* Свидетельство о государственной регистрации программы для ЭВМ №2021663231
* https://fips.ru/EGD/c7c3d928-893a-4f47-a218-51f3c5686860
*
* Transient stability simulation results compression library
* (C) 2018 - present Eugene Mashalov
* pat. RU №2021663231
*/


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
		int DataType;
		int64_t Offset;
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

		struct ChannelHeadersRange
		{
			CResultFileReader::ChannelHeaderInfo* begin = nullptr;
			CResultFileReader::ChannelHeaderInfo* end = nullptr;
		};

		struct ChannelHeaderComp
		{
			bool operator()(const ChannelHeaderInfo* lhs, const ChannelHeaderInfo* rhs) const
			{
				return std::tie(lhs->eDeviceType, lhs->DeviceId, lhs->DeviceVarIndex) < std::tie(rhs->eDeviceType, rhs->DeviceId, rhs->DeviceVarIndex);
			}
		};


		using CHANNELSET = std::set<ChannelHeaderInfo*, ChannelHeaderComp>;

	protected:

		std::unique_ptr<ChannelHeaderInfo[]> pChannelHeaders_;
		std::unique_ptr<DataDirectoryEntry[]> pDirectoryEntries_;
		std::unique_ptr<double[]> pTime_, pStep_;
		ChannelHeadersRange ChannelHeadersRange_;

		int Version_ = 0;
		double TimeCreated_ = 0.0;
		int DevTypesCount_ = 0;
		size_t DirectoryEntriesCount_ = 0;
		size_t PointsCount_;
		size_t ChannelsCount_;

		typedef std::list<int64_t> INT64LIST;
		int ReadBlockType();
		void GetBlocksOrder(INT64LIST& Offsets, uint64_t LastBlockOffset);
		void ReadModelData(std::unique_ptr<double[]>& pData, int nVarIndex);
		int64_t OffsetFromCurrent();
		std::string FilePath_;
		std::string Comment_;
		bool bHeaderLoaded_ = false;

		DEVTYPESET DevTypeSet_;
		CHANNELSET ChannelSet_;

		std::unique_ptr<DeviceTypeInfo[]> pDeviceTypeInfo_;

		VARNAMEMAP VarNameMap_;

#ifdef _MSC_VER
		CSlowVariablesSet SlowVariables_;
#endif		

		int64_t CommentOffset_;
		int64_t CommentDirectoryOffset_;
		int64_t DirectoryOffset_;

		std::string UserComment_;

		CRLECompressor RLECompressor_;
		double Ratio_ = 0.0;

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
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t eType, ptrdiff_t Id, std::string_view VarName);
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t eType, ptrdiff_t Id, ptrdiff_t VarIndex);
		std::unique_ptr<double[]> ReadChannel(ptrdiff_t Index);
		std::unique_ptr<double[]> GetTimeStep();
		ptrdiff_t GetChannelIndex(ptrdiff_t eType, ptrdiff_t Id, ptrdiff_t VarIndex);
		ptrdiff_t GetChannelIndex(ptrdiff_t eType, ptrdiff_t Id, std::string_view VarName);
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
		ChannelHeadersRange GetChannelHeadersRange() const;
#ifdef _MSC_VER		
		const CSlowVariablesSet& GetSlowVariables() { return SlowVariables_; }
#endif		
		const char* GetUserComment();
		void SetUserComment(std::string_view UserComment);
		double GetCompressionRatio();
	};
}
