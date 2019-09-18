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
	using namespace std;

	class CResultFile
	{
	protected:
		FILE *m_pFile;
	public:
		void WriteLEB(unsigned __int64 nValue);
		void WriteString(const _TCHAR *cszString);
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
			wstring Name;
			double Multiplier;
			int eUnits;
			ptrdiff_t nIndex;
			bool operator<(const VariableTypeInfo& other) const
			{
				return Name < other.Name;
			}
		};

		typedef set<VariableTypeInfo> VARTYPESET;
		typedef VARTYPESET::iterator VARTYPEITR;
		typedef VARTYPESET::const_iterator VARTYPEITRCONST;
		typedef list<VariableTypeInfo> VARTYPELIST;
		typedef VARTYPELIST::const_iterator VARTYPELISTITRCONST;

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
			wstring Name;
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

		typedef set<DeviceInstanceInfoBase*, DeviceInstanceCompare> DEVICESSET;
		typedef DEVICESSET::iterator DEVIDITR;
		typedef DEVICESSET::const_iterator DEVIDITRCONST;


		struct DeviceTypeInfo
		{
			int eDeviceType;
			int DeviceIdsCount;
			int DeviceParentIdsCount;
			int DevicesCount;
			int VariablesByDeviceCount;
			VARTYPESET m_VarTypes;
			VARTYPELIST m_VarTypesList;
			ptrdiff_t *pIds;
			DeviceLinkToParent *pLinks;
			const CResultFileReader *m_pFileReader;
			DeviceInstanceInfo *m_pDeviceInstances;
			DEVICESSET m_DevSet;
			wstring strDevTypeName;

			DeviceTypeInfo() : pIds(nullptr),
							   pLinks(nullptr),
							   m_pFileReader(nullptr),
							   m_pDeviceInstances(nullptr)
			{

			}

			void AllocateData();
			void IndexDevices();

			~DeviceTypeInfo()
			{
				if (pIds)	delete pIds;
				if (pLinks)	delete pLinks;
				if (m_pDeviceInstances) delete [] m_pDeviceInstances;
			}

		};

		struct DeviceTypesComp
		{
			bool operator()(const DeviceTypeInfo* lhs, const DeviceTypeInfo* rhs) const
			{ 
				return lhs->eDeviceType < rhs->eDeviceType;
			}
		};

		typedef set<DeviceTypeInfo*, DeviceTypesComp> DEVTYPESET;
		typedef DEVTYPESET::iterator DEVTYPEITR;
		typedef DEVTYPESET::const_iterator DEVTYPEITRCONST;

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


		typedef set<ChannelHeaderInfo*, ChannelHeaderComp> CHANNELSET;
		typedef CHANNELSET::iterator CHANNELSETITR;
		typedef CHANNELSET::const_iterator CHANNELSETITRCONST;

	protected:

		ChannelHeaderInfo *m_pChannelHeaders;
		double *m_pTime;
		double *m_pStep;

		int m_nVersion;
		double m_dTimeCreated;
		int m_DevTypesCount;
		size_t m_nDirectoryEntriesCount;
		DataDirectoryEntry *m_pDirectoryEntries;

		size_t m_PointsCount;
		size_t m_ChannelsCount;
		typedef list<__int64> INT64LIST;
		int ReadBlockType() const;
		void GetBlocksOrder(INT64LIST& Offsets, unsigned __int64 LastBlockOffset) const;
		void ReadModelData(double *pData, int nVarIndex);
		__int64 OffsetFromCurrent() const;
		wstring m_strFilePath;
		wstring m_strComment;
		bool m_bHeaderLoaded;

		DEVTYPESET m_DevTypeSet;
		CHANNELSET m_ChannelSet;

		DeviceTypeInfo *m_pDeviceTypeInfo;

		VARNAMEMAP m_VarNameMap;

		CSlowVariablesSet m_setSlowVariables;

		__int64 m_nCommentOffset;
		__int64 m_nCommentDirectoryOffset;
		__int64 m_DirectoryOffset;

		wstring m_strUserComment;

		CRLECompressor m_RLECompressor;
		double m_dRatio;

	public:
		CResultFileReader();
		virtual ~CResultFileReader();
		void OpenFile(const _TCHAR *cszFilePath);
		void Reparent();
		void ReadHeader(int& Version);
		void ReadLEB(unsigned __int64& nValue) const;
		int ReadLEBInt() const;
		void ReadString(std::wstring& String);
		void ReadDouble(double& Value);
		void ReadDirectoryEntries();
		double *ReadChannel(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR* cszVarName) const;
		double *ReadChannel(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex) const;
		double *ReadChannel(ptrdiff_t nIndex) const;
		double *GetTimeStep();
		ptrdiff_t GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex) const;
		ptrdiff_t GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR *cszVarName) const;
		SAFEARRAY* CreateSafeArray(double *pChannelData) const;
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
