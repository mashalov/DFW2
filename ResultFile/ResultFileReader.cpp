#include "stdafx.h"
#include "ResultFileReader.h"


using namespace DFW2;

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
			return throw CFileReadException(m_pFile, fmt::format(CDFW2Messages::m_cszResultFileHasNewerVersion, Version, DFW2_RESULTFILE_VERSION).c_str());
	}
	else
		return throw CFileReadException(nullptr);
}

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t nIndex) const
{
	std::unique_ptr<double[]> pResultBuffer;

	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_ChannelsCount))
	{
		ChannelHeaderInfo *pChannel = m_pChannelHeaders.get() + nIndex;
		CCompressorSingle comp;
		size_t nTimeIndex = 0;
		double dValue = 0.0;
		std::unique_ptr<BITWORD[]> pBuffer;

		pResultBuffer = std::make_unique<double[]>(m_PointsCount);

		if (!pResultBuffer)
			throw CFileReadException(m_pFile, CDFW2Messages::m_cszNoMemory);

		// получаем список блоков данных канала в порядке от конца к началу
		INT64LIST Offsets;
		GetBlocksOrder(Offsets, pChannel->LastBlockOffset);

		// проходим блоки в обратном порядке от начала к концу
		INT64LIST::reverse_iterator it = Offsets.rbegin();
		while (it != Offsets.rend())
		{
			if (_fseeki64(m_pFile, *it, SEEK_SET))
				throw CFileReadException(m_pFile);

			int BlockType	= ReadBlockType();
			int PointsCount = ReadLEBInt();

			pBuffer = nullptr;
			int BytesCount(1);

			switch (BlockType)
			{
			case 0:		// RLE-блок
			{
				// читаем сжатые данные
				BytesCount = ReadLEBInt();
				std::unique_ptr<unsigned char[]> pReadBuffer = std::make_unique<unsigned char[]>(BytesCount + 1);
				if (fread(pReadBuffer.get(), 1, BytesCount, m_pFile) != BytesCount)
					throw CFileReadException(m_pFile);

				CRLECompressor rle;
				// наихудший результат предиктивного сжатия - по байту на каждый double
				size_t nDecomprSize(PointsCount * (sizeof(double) + 1));
				pBuffer = std::make_unique<BITWORD[]>(nDecomprSize / sizeof(BITWORD) + 1);
				bool bRes = rle.Decompress(pReadBuffer.get(), BytesCount, static_cast<unsigned char*>(static_cast<void*>(pBuffer.get())), nDecomprSize);
				if (!bRes)
					throw CFileReadException(m_pFile);
				BytesCount = static_cast<int>(nDecomprSize);
			}
				break;
			case 1:		// блок без сжатия
				// читаем несжатые данные
				BytesCount = ReadLEBInt();
				pBuffer = std::make_unique<BITWORD[]>(BytesCount / sizeof(BITWORD) + 1);
				if (fread(pBuffer.get(), 1, BytesCount, m_pFile) != BytesCount)
					throw CFileReadException(m_pFile);
				break;
			case 2:		// блок SuperRLE
				pBuffer = std::make_unique<BITWORD[]>(BytesCount / sizeof(BITWORD) + 1);
				// читаем 1 байт сжатых предиктивным методом данных
				if (fread(pBuffer.get(), 1, BytesCount, m_pFile) != BytesCount)
					throw CFileReadException(m_pFile);
				break;
			default:
				throw CFileReadException(m_pFile);
				break;
			}

			CBitStream Input(pBuffer.get(), pBuffer.get() + BytesCount / sizeof(BITWORD) + 1, 0);
			for (int nPoint = 0; nPoint < PointsCount; nPoint++, nTimeIndex++)
			{
				double pred = comp.Predict(m_pTime[nTimeIndex]);
				dValue = 0.0;
				comp.ReadDouble(dValue, pred, Input);
				pResultBuffer[nTimeIndex] = dValue;
				comp.UpdatePredictor(dValue);
				if (BlockType == 2)
					Input.Rewind();
			}
			it++;
		}

		if (nTimeIndex != m_PointsCount)
			throw CFileReadException(m_pFile, fmt::format(CDFW2Messages::m_cszResultFilePointsCountMismatch, nIndex, nTimeIndex, m_PointsCount).c_str());
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
		return *it - m_pChannelHeaders.get();
	else
		return -1;
}

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex) const
{
	return ReadChannel(GetChannelIndex(eType,nId,nVarIndex));
}

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR* cszVarName) const
{
	return ReadChannel(GetChannelIndex(eType, nId, cszVarName));
}

// строит список блоков данных канала от конца к началу
void CResultFileReader::GetBlocksOrder(INT64LIST& Offsets, unsigned __int64 LastBlockOffset) const
{
	Offsets.clear();

	// начинаем с заданного смещения последнего блока
	// и работаем, пока не обнаружим смещение предыдущего
	// блока равное нулю

	while (LastBlockOffset)
	{
		// запоминаем смещение блока
		Offsets.push_back(LastBlockOffset);
		// встаем на смещение блока и читаем заголовок
		if (_fseeki64(m_pFile, LastBlockOffset, SEEK_SET))
			throw CFileReadException(m_pFile);

		int nBlockType = ReadBlockType();			// Block Type
		ReadLEBInt();								// doubles count
		// если блок имеет тип 2 - (SuperRLE) то размер блока 1 байт. Размер не записывается для такого блока
		// поэтому принимаем его по умолчанию
		int Bytes(1);
		if(nBlockType < 2)
			Bytes = ReadLEBInt();					// если блоки типа 0 или 1 - читаем размер блока в байтах

		
		/*
			Если нужно найти канал по смещению в файле - смещение задаем здесь,
			последовательно читаем все каналы и ждем пока не вылетит исключение.
			Индекс канала вызывашего исключение использовать для поиска девайса и переменной

			unsigned __int64 addr = 7907939;
			if (addr >= LastBlockOffset && addr < LastBlockOffset + Bytes)
				throw CFileReadException(m_pFile);
		*/
		

		// перемещаемся на запись смещения предыдущего блока, используя известный размер блока
		if (_fseeki64(m_pFile, Bytes, SEEK_CUR))
			throw CFileReadException(m_pFile);

		// читаем смещение предыдущего блока
		LastBlockOffset = OffsetFromCurrent();
	}
}

// чтение несжатых данных модели
void CResultFileReader::ReadModelData(std::unique_ptr<double[]>& pData, int nVarIndex)
{
	bool bRes = false;
	ChannelHeaderInfo *pChannel = m_pChannelHeaders.get();
	while (pChannel < m_pChannelHeaders.get() + m_ChannelsCount)
	{
		if (pChannel->eDeviceType == DEVTYPE_MODEL &&
			pChannel->DeviceId == 0 &&
			pChannel->DeviceVarIndex == nVarIndex)
			break;
		pChannel++;
	}

	size_t nTimeIndex = 0;

	if (pChannel < m_pChannelHeaders.get() + m_ChannelsCount)
	{
		// получаем список блоков данных канала в порядке от конца к началу
		INT64LIST Offsets;
		GetBlocksOrder(Offsets, pChannel->LastBlockOffset);

		// проходим блоки в обратном порядке от начала к концу
		INT64LIST::reverse_iterator it = Offsets.rbegin();
		while (it != Offsets.rend())
		{
			if (_fseeki64(m_pFile, *it, SEEK_SET))
				throw CFileReadException(m_pFile);
			ReadBlockType(); // Block Type
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
	m_pDirectoryEntries = std::make_unique<DataDirectoryEntry[]>(m_nDirectoryEntriesCount);
	m_DirectoryOffset = _ftelli64(m_pFile);
	if (fread_s(m_pDirectoryEntries.get(), m_nDirectoryEntriesCount * sizeof(struct DataDirectoryEntry), sizeof(struct DataDirectoryEntry), m_nDirectoryEntriesCount, m_pFile) != m_nDirectoryEntriesCount)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszWrongResultFile);
}

void CResultFileReader::Reparent()
{
	for (DEVTYPEITRCONST it = m_DevTypeSet.begin(); it != m_DevTypeSet.end(); it++)
	{
		DeviceTypeInfo *pDevType = *it;

		if (pDevType->DeviceParentIdsCount == 0) continue;

		DeviceInstanceInfo *pb = pDevType->m_pDeviceInstances.get();
		DeviceInstanceInfo *pe = pb + pDevType->DevicesCount;

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
	VariantInit(&vt);
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

	m_pDeviceTypeInfo = std::make_unique<DeviceTypeInfo[]>(m_DevTypesCount);
	DeviceTypeInfo  *pDevTypeInfo = m_pDeviceTypeInfo.get();

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

		DeviceInstanceInfo *pDevInst = pDevTypeInfo->m_pDeviceInstances.get();

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

	m_pChannelHeaders = std::make_unique<ChannelHeaderInfo[]>(m_ChannelsCount);
	ChannelHeaderInfo *pChannel = m_pChannelHeaders.get();

	for (size_t nChannel = 0; nChannel < m_ChannelsCount; nChannel++, pChannel++)
	{
		pChannel->eDeviceType = ReadLEBInt();
		ReadLEB(pChannel->DeviceId);
		pChannel->DeviceVarIndex = ReadLEBInt();
		pChannel->LastBlockOffset = OffsetFromCurrent();
		bool bInserted = m_ChannelSet.insert(pChannel).second;
		_ASSERTE(bInserted);
	}


	m_pTime = std::make_unique<double[]>(m_PointsCount);
	m_pStep = std::make_unique<double[]>(m_PointsCount);
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
		std::wstring strVarName;

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

		CSlowVariableItem *pItm = new CSlowVariableItem(static_cast<long>(nDeviceType), DeviceIds, strVarName.c_str());

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
		m_pFile = NULL;
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

std::unique_ptr<double[]> CResultFileReader::GetTimeStep()
{
	std::unique_ptr<double[]> pResultBuffer = std::make_unique<double[]>(m_PointsCount);
	if (!pResultBuffer)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszNoMemory);
	GetStep(pResultBuffer.get(), m_PointsCount);
	return pResultBuffer;
}

void CResultFileReader::GetTimeScale(double *pTimeBuffer, size_t nPointsCount) const
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	if (nPointsCount <= m_PointsCount && nPointsCount >= 0)
		std::copy(m_pTime.get(), m_pTime.get()+ nPointsCount, pTimeBuffer);
	else
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszNoMemory);
}

void CResultFileReader::GetStep(double *pStepBuffer, size_t nPointsCount) const
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	if (nPointsCount <= m_PointsCount && nPointsCount >= 0)
		std::copy(m_pStep.get(), m_pStep.get() + nPointsCount, pStepBuffer);
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
		DeviceLinkToParent *pLink = m_pDevType->pLinks.get() + nIndex * m_pDevType->DeviceParentIdsCount + nParentIndex;
		_ASSERTE(pLink < m_pDevType->pLinks.get() + m_pDevType->DeviceParentIdsCount * m_pDevType->DevicesCount);
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
			return m_pDevType->pLinks.get() + nIndex * m_pDevType->DeviceParentIdsCount + nParentIndex;
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
	DeviceInstanceInfo *pb = m_pDeviceInstances.get();
	DeviceInstanceInfo *pe = pb + DevicesCount;

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
			pLinks = std::make_unique<DeviceLinkToParent[]>(DeviceParentIdsCount * DevicesCount);

		_ASSERTE(!pIds);
		pIds = std::make_unique<ptrdiff_t[]>(DeviceIdsCount  * DevicesCount);

		_ASSERTE(!m_pDeviceInstances);
		m_pDeviceInstances = std::make_unique<DeviceInstanceInfo[]>(DevicesCount);

		DeviceInstanceInfo *pb = m_pDeviceInstances.get();
		DeviceInstanceInfo *pe = pb + DevicesCount;

		while (pb < pe)
		{
			pb->m_pDevType = this;
			pb++;
		}
	}
}

const CResultFileReader::ChannelHeaderInfo* CResultFileReader::GetChannelHeaders() const
{
	return m_pChannelHeaders.get();
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


SAFEARRAY* CResultFileReader::CreateSafeArray(std::unique_ptr<double[]>& pChannelData) const
{
	SAFEARRAY *pSA = nullptr;
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
					std::copy(pChannelData.get(), pChannelData.get() + nPointsCount, static_cast<double*>(pData));
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
		pSA = nullptr;
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

// чтение типа блока из файла с проверкой корректности типа
int CResultFileReader::ReadBlockType() const
{
	int nBlockType = ReadLEBInt();
	if (nBlockType >= 0 && nBlockType <= 2)
		return nBlockType;
	else
		throw CFileReadException(m_pFile, CDFW2Messages::m_cszResultFileWrongCompressedBlockType);
}


const char CResultFile::m_cszSignature[] = { 'R', 'a', 'i', 'd', 'e', 'n' };
const _TCHAR* CResultFileReader::m_cszUnknownUnits = _T("???");
