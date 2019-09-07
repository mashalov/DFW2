#include "stdafx.h"
#include "ResultFileReader.h"


using namespace DFW2;

CResultFileReader::CResultFileReader()
{
	m_pDirectoryEntries = NULL;
	m_nVersion = 0;
	m_dTimeCreated = 0.0;
	m_pChannelHeaders = NULL;
	m_pTime = NULL;
	m_pStep = NULL;
	m_bHeaderLoaded = false;
	m_pFile = NULL;
	m_pDeviceTypeInfo = NULL;
	m_dRatio = -1.0;
}

CResultFileReader::~CResultFileReader()
{
	Close();
}

void CResultFileReader::ReadHeader(int& Version)
{
	if (m_pFile)
	{
		unsigned __int64 Version64;
		ReadLEB(Version64);
		Version = static_cast<int>(Version64);
		if (Version64 > DFW2_RESULTFILE_VERSION)
			return throw CFileReadException(m_pFile, Cex(CDFW2Messages::m_cszResultFileHasNewerVersion, Version, DFW2_RESULTFILE_VERSION));
	}
	else
		return throw CFileReadException(NULL);
}

double *CResultFileReader::ReadChannel(ptrdiff_t nIndex) const
{
	double *pResultBuffer = NULL;

	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_ChannelsCount))
	{
		ChannelHeaderInfo *pChannel = m_pChannelHeaders + nIndex;
		CCompressorSingle comp;
		size_t nTimeIndex = 0;
		double dValue = 0.0;
		BITWORD *pBuffer(nullptr);

		try
		{
			pResultBuffer = new double[m_PointsCount];

			if (!pResultBuffer)
				throw CFileReadException(m_pFile, CDFW2Messages::m_cszNoMemory);

			INT64LIST Offsets;
			GetBlocksOrder(Offsets, pChannel->LastBlockOffset);
			INT64LIST::reverse_iterator it = Offsets.rbegin();

			while (it != Offsets.rend())
			{
				if (_fseeki64(m_pFile, *it, SEEK_SET))
					throw CFileReadException(m_pFile);

				int BlockType = ReadLEBInt();
				int PointsCount = ReadLEBInt();
				int BytesCount = ReadLEBInt();

				pBuffer = nullptr;

				if (BlockType == 0)
				{
					unsigned char *pReadBuffer = new unsigned char[BytesCount + 1]();
					if (fread(pReadBuffer, 1, BytesCount, m_pFile) != BytesCount)
					{
						delete pReadBuffer;
						throw CFileReadException(m_pFile);
					}
					CRLECompressor rle;
					// наихудший результат предиктивного сжатия - по байту на каждый double
					size_t nDecomprSize(PointsCount * (sizeof(double) + 1));
					pBuffer = new BITWORD[nDecomprSize / sizeof(BITWORD) + 1]();
					bool bRes = rle.Decompress(pReadBuffer, BytesCount, static_cast<unsigned char*>(static_cast<void*>(pBuffer)), nDecomprSize);
					delete pReadBuffer;
					if (!bRes)
						throw CFileReadException(m_pFile);
					BytesCount = static_cast<int>(nDecomprSize);

				}
				else
				{
					pBuffer = new BITWORD[BytesCount / sizeof(BITWORD) + 1]();
					if (fread(pBuffer, 1, BytesCount, m_pFile) != BytesCount)
						throw CFileReadException(m_pFile);
				}

				CBitStream Input(pBuffer, pBuffer + BytesCount / sizeof(BITWORD) + 1, 0);

				for (int nPoint = 0; nPoint < PointsCount; nPoint++, nTimeIndex++)
				{
					double pred = comp.Predict(m_pTime[nTimeIndex]);
					dValue = 0.0;
					comp.ReadDouble(dValue, pred, Input);
					pResultBuffer[nTimeIndex] = dValue;
					comp.UpdatePredictor(dValue);
				}

				if(pBuffer)
					delete pBuffer;

				pBuffer = NULL;
				it++;
			}
		}
		catch (CFileReadException&)
		{
			if (pBuffer)
				delete pBuffer;
			if (pResultBuffer)
			{
				delete pResultBuffer;
				pResultBuffer = NULL;
			}
		}
	}
	return pResultBuffer;
}


ptrdiff_t CResultFileReader::GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR *cszVarName) const
{

	ptrdiff_t nVarIndex = -1;

	DeviceTypeInfo findType;
	findType.eDeviceType = static_cast<int>(eType);
	DEVTYPEITRCONST it = m_DevTypeSet.find(&findType);
	if (it != m_DevTypeSet.end())
	{
		VariableTypeInfo findVar; 
		findVar.Name = cszVarName;
		VARTYPEITRCONST vit = (*it)->m_VarTypes.find(findVar);
		if (vit != (*it)->m_VarTypes.end())
			return GetChannelIndex(eType, nId, vit->nIndex);
	}
	return -1;
}


ptrdiff_t CResultFileReader::GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex) const
{
	
	ChannelHeaderInfo findChannel;
	findChannel.DeviceId = nId;
	findChannel.DeviceVarIndex = static_cast<int>(nVarIndex);
	findChannel.eDeviceType = static_cast<int>(eType);
	CHANNELSETITRCONST it = m_ChannelSet.find(&findChannel);
	if (it != m_ChannelSet.end())
		return *it - m_pChannelHeaders;
	else
		return -1;
}

double *CResultFileReader::ReadChannel(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex) const
{
	return ReadChannel(GetChannelIndex(eType,nId,nVarIndex));
}

double *CResultFileReader::ReadChannel(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR* cszVarName) const
{
	return ReadChannel(GetChannelIndex(eType, nId, cszVarName));
}


void CResultFileReader::GetBlocksOrder(INT64LIST& Offsets, unsigned __int64 LastBlockOffset) const
{
	Offsets.clear();

	while (LastBlockOffset)
	{
		Offsets.push_back(LastBlockOffset);

		if (_fseeki64(m_pFile, LastBlockOffset, SEEK_SET))
			throw CFileReadException(m_pFile);

		ReadLEBInt(); // Block Type
		ReadLEBInt(); // Count
		int Bytes = ReadLEBInt();
		if (_fseeki64(m_pFile, Bytes, SEEK_CUR))
			throw CFileReadException(m_pFile);
		LastBlockOffset = OffsetFromCurrent();
	}
}

void CResultFileReader::ReadModelData(double *pData, int nVarIndex)
{
	bool bRes = false;
	ChannelHeaderInfo *pChannel = m_pChannelHeaders;
	while (pChannel < m_pChannelHeaders + m_ChannelsCount)
	{
		if (pChannel->eDeviceType == DEVTYPE_MODEL &&
			pChannel->DeviceId == 0 &&
			pChannel->DeviceVarIndex == nVarIndex)
			break;
		pChannel++;
	}

	size_t nTimeIndex = 0;

	if (pChannel < m_pChannelHeaders + m_ChannelsCount)
	{
		INT64LIST Offsets;
		GetBlocksOrder(Offsets, pChannel->LastBlockOffset);
		INT64LIST::reverse_iterator it = Offsets.rbegin();

		while (it != Offsets.rend())
		{
			if (_fseeki64(m_pFile, *it, SEEK_SET))
				throw CFileReadException(m_pFile);
			ReadLEBInt(); // Block Type
			int PointsCount = ReadLEBInt();
			ReadLEBInt(); // Bytes

			for (int nPoint = 0; nPoint < PointsCount; nPoint++)
			{
				ReadDouble(pData[nTimeIndex]);
				if (++nTimeIndex > m_PointsCount)
					throw CFileReadException(m_pFile);
			}
			it++;
		}
	}
	else
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
}

void CResultFileReader::ReadDirectoryEntries()
{
	unsigned __int64 nDirEntries;
	ReadLEB(nDirEntries);
	m_nDirectoryEntriesCount = static_cast<size_t>(nDirEntries);
	m_pDirectoryEntries = new DataDirectoryEntry[m_nDirectoryEntriesCount];
	m_DirectoryOffset = _ftelli64(m_pFile);
	if (fread_s(m_pDirectoryEntries, m_nDirectoryEntriesCount * sizeof(struct DataDirectoryEntry), sizeof(struct DataDirectoryEntry), m_nDirectoryEntriesCount, m_pFile) != m_nDirectoryEntriesCount)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
}

void CResultFileReader::Reparent()
{
	for (DEVTYPEITRCONST it = m_DevTypeSet.begin(); it != m_DevTypeSet.end(); it++)
	{
		DeviceTypeInfo *pDevType = *it;

		if (pDevType->DeviceParentIdsCount == 0) continue;

		DeviceInstanceInfo *pb = pDevType->m_pDeviceInstances;
		DeviceInstanceInfo *pe = pDevType->m_pDeviceInstances + pDevType->DevicesCount;

		while (pb < pe)
		{
			const DeviceLinkToParent *pLink = pb->GetParent(0);
			if (pLink && pLink->m_eParentType != 0)
			{
				DeviceTypeInfo findType;
				findType.eDeviceType = static_cast<int>(pLink->m_eParentType);
				DEVTYPEITRCONST ftypeit = m_DevTypeSet.find(&findType);
				if (ftypeit != m_DevTypeSet.end())
				{
					DeviceTypeInfo *pParentTypeInfo = *ftypeit;

					DeviceInstanceInfoBase findInstance;
					findInstance.nIndex = pLink->m_nId;

					if (pParentTypeInfo->m_DevSet.find(&findInstance) == pParentTypeInfo->m_DevSet.end())
						pb->SetParent(0, 0, 0);
				}
				else
					pb->SetParent(0, 0, 0);
			}
			pb++;
		}
	}
}

void CResultFileReader::OpenFile(const _TCHAR *cszFilePath)
{
	m_dRatio = -1.0;

	m_strFilePath = cszFilePath;

	if (_tfopen_s(&m_pFile, cszFilePath, _T("rb+,ccs=UNICODE")))
		throw CFileReadException(NULL, cszFilePath);

	size_t nCountSignature = sizeof(m_cszSignature);
	char SignatureBuffer[sizeof(m_cszSignature)];

	if (fread_s(SignatureBuffer, nCountSignature, sizeof(m_cszSignature[0]), nCountSignature, m_pFile) != nCountSignature)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);

	if (memcmp(m_cszSignature, SignatureBuffer, nCountSignature))
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
	ReadHeader(m_nVersion);
	ReadDouble(m_dTimeCreated);
	VARIANT vt;
	VariantClear(&vt);
	vt.vt = VT_DATE;
	vt.date = m_dTimeCreated;

	if(!SUCCEEDED(VariantChangeType(&vt, &vt, 0, VT_BSTR)))
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);

	std::wstring strDateTime(vt.bstrVal, SysStringLen(vt.bstrVal));
	_tcprintf(_T("\nCreated %s "), strDateTime.c_str());

	ReadString(m_strComment);
	ReadDirectoryEntries();

	int VarNameCount = ReadLEBInt();
	m_VarNameMap.clear();

	// читаем типы и названия единиц измерения
	for (int varname = 0; varname < VarNameCount; varname++)
	{
		int VarType = ReadLEBInt();		// тип
		std::wstring strVarName;
		ReadString(strVarName);			// название
		// тип и название вводим в карту
		if (!m_VarNameMap.insert(make_pair(VarType,strVarName)).second)
			throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
	}

	m_DevTypesCount = ReadLEBInt();

	m_pDeviceTypeInfo = new DeviceTypeInfo[m_DevTypesCount];
	DeviceTypeInfo  *pDevTypeInfo = m_pDeviceTypeInfo;

	for (int i = 0; i < m_DevTypesCount; i++, pDevTypeInfo++)
	{
		pDevTypeInfo->m_pFileReader = this;
		pDevTypeInfo->eDeviceType = ReadLEBInt();
		ReadString(pDevTypeInfo->strDevTypeName);
		_tcprintf(_T("\nType %d "), pDevTypeInfo->eDeviceType);
		pDevTypeInfo->DeviceIdsCount = ReadLEBInt();
		_tcprintf(_T("IdsCount %d "), pDevTypeInfo->DeviceIdsCount);
		pDevTypeInfo->DeviceParentIdsCount = ReadLEBInt();
		_tcprintf(_T("ParentIdsCount %d "), pDevTypeInfo->DeviceParentIdsCount);
		pDevTypeInfo->DevicesCount = ReadLEBInt();
		_tcprintf(_T("DevCount %d "), pDevTypeInfo->DevicesCount);
		pDevTypeInfo->VariablesByDeviceCount = ReadLEBInt();
		_tcprintf(_T("VarsCount %d"), pDevTypeInfo->VariablesByDeviceCount);


		VariableTypeInfo VarTypeInfo;

		for (int iVar = 0; iVar < pDevTypeInfo->VariablesByDeviceCount; iVar++)
		{
			ReadString(VarTypeInfo.Name);
			_tcprintf(_T("\nVar %s "), VarTypeInfo.Name.c_str());
			VarTypeInfo.eUnits = ReadLEBInt();
			_tcprintf(_T("Units %d "), VarTypeInfo.eUnits);
			int nBitFlags = ReadLEBInt();
			if (nBitFlags & 0x01)
			{
				ReadDouble(VarTypeInfo.Multiplier);
				_tcprintf(_T("Mult %g "), VarTypeInfo.Multiplier);
			}
			else
				VarTypeInfo.Multiplier = 1.0;

			VarTypeInfo.nIndex = iVar;

			if (!pDevTypeInfo->m_VarTypes.insert(VarTypeInfo).second)
				throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
		}

		pDevTypeInfo->AllocateData();

		if (!m_DevTypeSet.insert(pDevTypeInfo).second) 
			throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);

		DeviceInstanceInfo *pDevInst = pDevTypeInfo->m_pDeviceInstances;

		for (int iDev = 0; iDev < pDevTypeInfo->DevicesCount; iDev++, pDevInst++)
		{
			pDevInst->nIndex = iDev;

			_tcprintf(_T("\n"));
			for (int Ids = 0; Ids < pDevTypeInfo->DeviceIdsCount; Ids++)
			{
				unsigned __int64 ReadInt64;
				ReadLEB(ReadInt64);
				_tcprintf(_T("Id[%d] %d "), Ids, static_cast<int>(ReadInt64));
				pDevInst->SetId(Ids, static_cast<int>(ReadInt64));
			}

			ReadString(pDevInst->Name);
			_tcprintf(_T("Name %s "), pDevInst->Name.c_str());

			for (int Ids = 0; Ids < pDevTypeInfo->DeviceParentIdsCount; Ids++)
			{
				unsigned __int64 nId = 0;
				unsigned __int64 eType = 0;
				ReadLEB(eType);
				ReadLEB(nId);
				_tcprintf(_T("Link to %d of type %d "), static_cast<int>(nId), static_cast<int>(eType));
				pDevInst->SetParent(Ids, static_cast<int>(eType), static_cast<int>(nId));
			}
		}
		pDevTypeInfo->IndexDevices();
	}

	Reparent();

	__int64 nChannelHeadersOffset = -1;
	__int64 nSlowVarsOffset = -1;
	m_nCommentOffset = m_nCommentDirectoryOffset = -1;

	for (size_t i = 0; i < m_nDirectoryEntriesCount; i++)
	{
		switch (m_pDirectoryEntries[i].m_DataType)
		{
		case 0:
			if (nChannelHeadersOffset == -1)
				nChannelHeadersOffset = m_pDirectoryEntries[i].m_Offset;
			break;
		case 1:
			if (nSlowVarsOffset == -1)
				nSlowVarsOffset = m_pDirectoryEntries[i].m_Offset;
			break;
		case 2:
			if (m_nCommentOffset == -1)
			{
				m_nCommentOffset = m_pDirectoryEntries[i].m_Offset;
				m_nCommentDirectoryOffset = m_DirectoryOffset + i * sizeof(DataDirectoryEntry);
			}
			break;
		}
	}

	__int64 nCompressedDataOffset = _ftelli64(m_pFile);

	if (nChannelHeadersOffset < 0)
		throw CFileReadException(m_pFile);
	if (_fseeki64(m_pFile, nChannelHeadersOffset, SEEK_SET))
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
	unsigned __int64 ReadInt64;
	ReadLEB(ReadInt64);
	m_PointsCount = static_cast<size_t>(ReadInt64);
	ReadLEB(ReadInt64);
	m_ChannelsCount = static_cast<size_t>(ReadInt64);

	m_pChannelHeaders = new ChannelHeaderInfo[m_ChannelsCount];
	ChannelHeaderInfo *pChannel = m_pChannelHeaders;

	for (size_t nChannel = 0; nChannel < m_ChannelsCount; nChannel++, pChannel++)
	{
		pChannel->eDeviceType = ReadLEBInt();
		ReadLEB(pChannel->DeviceId);
		pChannel->DeviceVarIndex = ReadLEBInt();
		pChannel->LastBlockOffset = OffsetFromCurrent();
		bool bInserted = m_ChannelSet.insert(pChannel).second;
		_ASSERTE(bInserted);
	}


	m_pTime = new double[m_PointsCount];
	m_pStep = new double[m_PointsCount];
	ReadModelData(m_pTime,0);
	ReadModelData(m_pStep,1);


	if (nSlowVarsOffset < 0)
		throw CFileReadException(m_pFile);
	if (_fseeki64(m_pFile, nSlowVarsOffset, SEEK_SET))
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);

	m_dRatio = static_cast<double>(nSlowVarsOffset - nCompressedDataOffset) / sizeof(double) / m_ChannelsCount / m_PointsCount;

	unsigned __int64 nSlowVarsCount = 0;
	ReadLEB(nSlowVarsCount);
	while (nSlowVarsCount)
	{
		unsigned __int64 nDeviceType = 0;
		unsigned __int64 nKeysSize = 0;
		wstring strVarName;

		ReadLEB(nDeviceType);
		ReadString(strVarName);
		ReadLEB(nKeysSize);

		LONGVECTOR DeviceIds;
		while (nKeysSize)
		{
			unsigned __int64 nId;
			ReadLEB(nId);
			DeviceIds.push_back(static_cast<long>(nId));
			nKeysSize--;
		}

		CSlowVariableItem *pItm = new  CSlowVariableItem(static_cast<long>(nDeviceType), DeviceIds, strVarName.c_str());

		ReadLEB(nKeysSize); // graph size now

		while (nKeysSize)
		{
			double dTime = 0.0;
			double dValue = 0.0;
			ReadDouble(dTime);
			ReadDouble(dValue);
			ReadString(strVarName);
			CSlowVariableGraphItem itg(dTime, dValue, strVarName.c_str());
			pItm->m_Graph.insert(itg);
			nKeysSize--;
		}

		m_setSlowVariables.insert(pItm);

		nSlowVarsCount--;
	}

	if (m_nCommentOffset > 0)
	{
		if (_fseeki64(m_pFile, m_nCommentOffset, SEEK_SET))
			throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
		ReadString(m_strUserComment);
	}

	m_bHeaderLoaded = true;

}


void CResultFileReader::ReadString(std::wstring& String)
{
	if (m_pFile)
	{
		unsigned __int64 nLen64 = 0;
		ReadLEB(nLen64);
		if (nLen64 < 0xffff)
		{
			CUnicodeSCSU StringWriter(m_pFile);
			StringWriter.ReadSCSU(String, static_cast<int>(nLen64));
		}
	}
	else
		throw CFileReadException(NULL);
}

void CResultFileReader::ReadDouble(double& Value)
{
	if (m_pFile)
	{
		if (fread_s(&Value, sizeof(double), sizeof(double), 1, m_pFile) != 1)
			throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
	}
	else
		throw CFileReadException(NULL);
}

int CResultFileReader::ReadLEBInt() const
{
	unsigned __int64 IntToRead;
	ReadLEB(IntToRead);
	if (IntToRead > 0x7fffffff)	// для LEB разрешаем только 31-битное число 
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
	return static_cast<int>(IntToRead);
}
void CResultFileReader::ReadLEB(unsigned __int64 & nValue) const
{
	if (m_pFile)
	{
		nValue = 0;
		ptrdiff_t shift = 0;
		unsigned char low;
		do
		{
			if (fread_s(&low, sizeof(low), sizeof(low), 1, m_pFile) != 1)
				throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);

			nValue |= ((static_cast<unsigned __int64>(low)& 0x7f) << shift);
			shift += 7;
			if (shift > 64)
				break;
		} while (low & 0x80);
	}
	else
		throw CFileReadException(NULL);
}


__int64 CResultFileReader::OffsetFromCurrent() const
{
	__int64 nCurrentOffset = _ftelli64(m_pFile);
	unsigned __int64 nRelativeOffset = 0;
	ReadLEB(nRelativeOffset);

	if (static_cast<__int64>(nRelativeOffset) > nCurrentOffset)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);

	if (nRelativeOffset)
		nRelativeOffset = nCurrentOffset - nRelativeOffset;
	
	return nRelativeOffset;
}

void CResultFileReader::Close()
{
	m_strFilePath.clear();
	m_bHeaderLoaded = false;

	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile =NULL;
	}

	if (m_pDirectoryEntries)
	{
		delete m_pDirectoryEntries;
		m_pDirectoryEntries = NULL;
	}

	if (m_pChannelHeaders)
	{
		delete m_pChannelHeaders;
		m_pChannelHeaders = NULL;
	}

	if (m_pTime)
	{
		delete m_pTime;
		m_pTime = NULL;
	}

	if (m_pStep)
	{
		delete m_pStep;
		m_pStep = NULL;
	}

	if (m_pDeviceTypeInfo)
	{
		delete[] m_pDeviceTypeInfo;
		m_pDeviceTypeInfo = NULL;
	}

	m_DevTypeSet.clear();
	m_dRatio = -1.0;

}

double CResultFileReader::GetFileTime()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_dTimeCreated;
}

const _TCHAR* CResultFileReader::GetFilePath()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_strFilePath.c_str();
}

const _TCHAR* CResultFileReader::GetComment()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_strComment.c_str();
}

size_t CResultFileReader::GetPointsCount() const
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_PointsCount;
}

size_t CResultFileReader::GetChannelsCount() const
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_ChannelsCount;
}

double* CResultFileReader::GetTimeStep()
{
	double *pResultBuffer = new double[m_PointsCount];
	if (!pResultBuffer)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszNoMemory);
	GetStep(pResultBuffer, m_PointsCount);
	return pResultBuffer;
}

void CResultFileReader::GetTimeScale(double *pTimeBuffer, size_t nPointsCount) const
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	if (nPointsCount <= m_PointsCount && nPointsCount >= 0)
		memcpy(pTimeBuffer, m_pTime, sizeof(double) * nPointsCount);
	else
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszNoMemory);
}

void CResultFileReader::GetStep(double *pStepBuffer, size_t nPointsCount) const
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	if (nPointsCount <= m_PointsCount && nPointsCount >= 0)
		memcpy(pStepBuffer, m_pStep, sizeof(double) * nPointsCount);
	else
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszNoMemory);
}

int CResultFileReader::GetVersion()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);
	return m_nVersion;
}

CResultFileReader::DeviceInstanceInfo::DeviceInstanceInfo(struct DeviceTypeInfo* pDevTypeInfo) : m_pDevType(pDevTypeInfo) {}

void CResultFileReader::DeviceInstanceInfo::SetId(ptrdiff_t nIdIndex, ptrdiff_t nId)
{
	if (nIdIndex >= 0 && nIdIndex < m_pDevType->DeviceIdsCount)
	{
		_ASSERTE(nIndex * m_pDevType->DeviceIdsCount + nIdIndex < m_pDevType->DeviceIdsCount * m_pDevType->DevicesCount);
		m_pDevType->pIds[nIndex * m_pDevType->DeviceIdsCount + nIdIndex] = nId;
	}
	else
		throw CFileReadException(NULL, CDFW2Messages::m_cszWrongResultFile);
}

ptrdiff_t CResultFileReader::DeviceInstanceInfo::GetId(ptrdiff_t nIdIndex) const
{
	if (nIdIndex >= 0 && nIdIndex < m_pDevType->DeviceIdsCount)
		return m_pDevType->pIds[nIndex * m_pDevType->DeviceIdsCount + nIdIndex];
	else
		throw CFileReadException(NULL, CDFW2Messages::m_cszWrongResultFile);
}

void CResultFileReader::DeviceInstanceInfo::SetParent(ptrdiff_t nParentIndex, ptrdiff_t eParentType, ptrdiff_t nParentId) 
{
	if (nParentIndex >= 0 && nParentIndex < m_pDevType->DeviceParentIdsCount)
	{
		DeviceLinkToParent *pLink = m_pDevType->pLinks + nIndex * m_pDevType->DeviceParentIdsCount + nParentIndex;

		_ASSERTE(pLink < m_pDevType->pLinks + m_pDevType->DeviceParentIdsCount * m_pDevType->DevicesCount);

		pLink->m_eParentType = eParentType;
		pLink->m_nId = nParentId;
	}
	else
		throw CFileReadException(NULL, CDFW2Messages::m_cszWrongResultFile);
}

const CResultFileReader::DeviceLinkToParent* CResultFileReader::DeviceInstanceInfo::GetParent(ptrdiff_t nParentIndex) const
{
	if (m_pDevType->DeviceParentIdsCount)
	{
		if (nParentIndex >= 0 && nParentIndex < m_pDevType->DeviceParentIdsCount)
			return m_pDevType->pLinks + nIndex * m_pDevType->DeviceParentIdsCount + nParentIndex;
		else
			throw CFileReadException(NULL, CDFW2Messages::m_cszWrongResultFile);
	}
	else return nullptr;
}

const CResultFileReader::DEVTYPESET& CResultFileReader::GetTypesSet() const
{
	return m_DevTypeSet;
}

void CResultFileReader::DeviceTypeInfo::IndexDevices()
{
	DeviceInstanceInfo *pb = m_pDeviceInstances;
	DeviceInstanceInfo *pe = m_pDeviceInstances + DevicesCount;

	while (pb < pe)
	{
		bool bInserted = m_DevSet.insert(pb).second;
		_ASSERTE(bInserted);
		pb++;
	}
}

void CResultFileReader::DeviceTypeInfo::AllocateData()
{
	if (DevicesCount)
	{
		_ASSERTE(!pLinks);
		if (DeviceParentIdsCount)
			pLinks = new DeviceLinkToParent[DeviceParentIdsCount * DevicesCount];

		_ASSERTE(!pIds);
		pIds = new ptrdiff_t[DeviceIdsCount  * DevicesCount];

		_ASSERTE(!m_pDeviceInstances);
		m_pDeviceInstances = new DeviceInstanceInfo[DevicesCount];

		DeviceInstanceInfo *pb = m_pDeviceInstances;
		DeviceInstanceInfo *pe = m_pDeviceInstances + DevicesCount;

		while (pb < pe)
		{
			pb->m_pDevType = this;
			pb++;
		}
	}
}

const CResultFileReader::ChannelHeaderInfo* CResultFileReader::GetChannelHeaders() const
{
	return m_pChannelHeaders;
}

// возвращает название единиц измерения по заданному типу
const _TCHAR* CResultFileReader::GetUnitsName(ptrdiff_t eUnitsType) const
{
	VARNAMEITRCONST it = m_VarNameMap.find(eUnitsType);
	if (it != m_VarNameMap.end())
		return it->second.c_str();
	else
		return m_cszUnknownUnits;
}


SAFEARRAY* CResultFileReader::CreateSafeArray(double *pChannelData) const
{
	SAFEARRAY *pSA = NULL;
	try
	{
		size_t nPointsCount = GetPointsCount();
		if (pChannelData)
		{
			SAFEARRAYBOUND sabounds[2] = { { static_cast<ULONG>(nPointsCount), 0 }, { 2, 0 } };
			pSA = SafeArrayCreate(VT_R8, 2, sabounds);
			if (pSA)
			{
				void *pData;
				if (SUCCEEDED(SafeArrayAccessData(pSA, &pData)))
				{
					GetTimeScale(static_cast<double*>(pData)+nPointsCount, nPointsCount);
					memcpy(static_cast<double*>(pData), pChannelData, sizeof(double) * nPointsCount);
					HRESULT hRes = SafeArrayUnaccessData(pSA);
					_ASSERTE(SUCCEEDED(hRes));
				}
			}
		}
	}
	catch (...)
	{
		if (pSA)
			SafeArrayDestroy(pSA);
		pSA = NULL; 
	}
	return pSA;
}

const _TCHAR* CResultFileReader::GetUserComment()
{
	return m_strUserComment.c_str();
}

void CResultFileReader::SetUserComment(const _TCHAR* cszUserComment)
{
	if (m_nCommentOffset > 0 && m_nCommentDirectoryOffset > 0 && cszUserComment)
	{
		if (_fseeki64(m_pFile, m_nCommentOffset, SEEK_SET))
			throw CFileWriteException(m_pFile);
		WriteString(cszUserComment);

		if(_chsize_s(_fileno(m_pFile), _ftelli64(m_pFile)))
			throw CFileWriteException(m_pFile);

		if (_fseeki64(m_pFile, m_nCommentDirectoryOffset, SEEK_SET))
			throw CFileWriteException(m_pFile);
		
		DataDirectoryEntry de = {2, m_nCommentOffset};

		if (fwrite(&de, sizeof(DataDirectoryEntry), 1, m_pFile) != 1)
			throw CFileWriteException(m_pFile);

		m_strUserComment = cszUserComment;
	}
}

double CResultFileReader::GetCompressionRatio()
{
	return m_dRatio;
}


const char CResultFile::m_cszSignature[] = { 'R', 'a', 'i', 'd', 'e', 'n' };
const _TCHAR* CResultFileReader::m_cszUnknownUnits = _T("???");
