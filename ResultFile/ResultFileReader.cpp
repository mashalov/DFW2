#include "stdafx.h"
#include "ResultFileReader.h"


using namespace DFW2;

CResultFileReader::~CResultFileReader()
{
	Close();
}

void CResultFileReader::ReadHeader(int& Version)
{
	uint64_t Version64;
	ReadLEB(Version64);
	Version = static_cast<int>(Version64);
	if (Version64 > DFW2_RESULTFILE_VERSION)
		return throw CFileReadException(infile, fmt::format(CDFW2Messages::m_cszResultFileHasNewerVersion, Version, DFW2_RESULTFILE_VERSION).c_str());
}

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t nIndex)
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
			throw CFileReadException(infile, CDFW2Messages::m_cszNoMemory);

		// получаем список блоков данных канала в порядке от конца к началу
		INT64LIST Offsets;
		GetBlocksOrder(Offsets, pChannel->LastBlockOffset);

		// проходим блоки в обратном порядке от начала к концу
		INT64LIST::reverse_iterator it = Offsets.rbegin();
		while (it != Offsets.rend())
		{
			infile.seekg(*it, std::ios_base::beg);

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
				infile.read(pReadBuffer.get(), BytesCount);

				CRLECompressor rle;
				// наихудший результат предиктивного сжатия - по байту на каждый double
				size_t nDecomprSize(PointsCount * (sizeof(double) + 1));
				pBuffer = std::make_unique<BITWORD[]>(nDecomprSize / sizeof(BITWORD) + 1);
				bool bRes = rle.Decompress(pReadBuffer.get(), BytesCount, static_cast<unsigned char*>(static_cast<void*>(pBuffer.get())), nDecomprSize);
				if (!bRes)
					throw CFileReadException();
				BytesCount = static_cast<int>(nDecomprSize);
			}
				break;
			case 1:		// блок без сжатия
				// читаем несжатые данные
				BytesCount = ReadLEBInt();
				pBuffer = std::make_unique<BITWORD[]>(BytesCount / sizeof(BITWORD) + 1);
				infile.read(pBuffer.get(), BytesCount);
				break;
			case 2:		// блок SuperRLE
				pBuffer = std::make_unique<BITWORD[]>(BytesCount / sizeof(BITWORD) + 1);
				// читаем 1 байт сжатых предиктивным методом данных
				infile.read(pBuffer.get(), BytesCount);
				break;
			default:
				throw CFileReadException(infile);
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
			throw CFileReadException(infile, fmt::format(CDFW2Messages::m_cszResultFilePointsCountMismatch, nIndex, nTimeIndex, m_PointsCount).c_str());
	}
	return pResultBuffer;
}


ptrdiff_t CResultFileReader::GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, std::string_view VarName)
{

	ptrdiff_t nVarIndex = -1;

	DeviceTypeInfo findType;
	findType.eDeviceType = static_cast<int>(eType);
	DEVTYPEITRCONST it = m_DevTypeSet.find(&findType);
	if (it != m_DevTypeSet.end())
	{
		VariableTypeInfo findVar; 
		findVar.Name = VarName;
		VARTYPEITRCONST vit = (*it)->m_VarTypes.find(findVar);
		if (vit != (*it)->m_VarTypes.end())
			return GetChannelIndex(eType, nId, vit->nIndex);
	}
	return -1;
}


ptrdiff_t CResultFileReader::GetChannelIndex(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex)
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

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex)
{
	return ReadChannel(GetChannelIndex(eType,nId,nVarIndex));
}

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t eType, ptrdiff_t nId, std::string_view VarName)
{
	return ReadChannel(GetChannelIndex(eType, nId, VarName));
}

// строит список блоков данных канала от конца к началу
void CResultFileReader::GetBlocksOrder(INT64LIST& Offsets, uint64_t LastBlockOffset)
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
		infile.seekg(LastBlockOffset, std::ios_base::beg);

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
		infile.seekg(Bytes, std::ios_base::cur);

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
			infile.seekg(*it, std::ios_base::beg);
			ReadBlockType(); // Block Type
			int PointsCount = ReadLEBInt();
			ReadLEBInt(); // Bytes
			for (int nPoint = 0; nPoint < PointsCount; nPoint++)
			{
				ReadDouble(pData[nTimeIndex]);
				if (++nTimeIndex > m_PointsCount)
					throw CFileReadException(infile);
			}
			it++;
		}
	}
	else
		throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);
}

void CResultFileReader::ReadDirectoryEntries()
{
	uint64_t nDirEntries;
	ReadLEB(nDirEntries);
	m_nDirectoryEntriesCount = static_cast<size_t>(nDirEntries);
	m_pDirectoryEntries = std::make_unique<DataDirectoryEntry[]>(m_nDirectoryEntriesCount);
	m_DirectoryOffset = infile.tellg();
	infile.read(m_pDirectoryEntries.get(), m_nDirectoryEntriesCount * sizeof(struct DataDirectoryEntry));
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

void CResultFileReader::OpenFile(std::string_view FilePath)
{
	m_dRatio = -1.0;

	m_strFilePath = FilePath;

	infile.open(FilePath, std::ios_base::in|std::ios_base::binary|std::ios_base::out);

	size_t nCountSignature = sizeof(m_cszSignature);
	char SignatureBuffer[sizeof(m_cszSignature)];
	infile.read(SignatureBuffer, nCountSignature);
	if (memcmp(m_cszSignature, SignatureBuffer, nCountSignature))
		throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);
	ReadHeader(m_nVersion);
	ReadDouble(m_dTimeCreated);
	VARIANT vt;
	VariantInit(&vt);
	vt.vt = VT_DATE;
	vt.date = m_dTimeCreated;

	if(!SUCCEEDED(VariantChangeType(&vt, &vt, 0, VT_BSTR)))
		throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);

	std::string strDateTime(stringutils::utf8_encode(vt.bstrVal));
	printf("\nCreated %s ", strDateTime.c_str());

	ReadString(m_strComment);
	ReadDirectoryEntries();

	int VarNameCount = ReadLEBInt();
	m_VarNameMap.clear();

	// читаем типы и названия единиц измерения
	for (int varname = 0; varname < VarNameCount; varname++)
	{
		int VarType = ReadLEBInt();		// тип
		std::string strVarName;
		ReadString(strVarName);			// название
		// тип и название вводим в карту
		if (!m_VarNameMap.insert(make_pair(VarType,strVarName)).second)
			throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);
	}

	m_DevTypesCount = ReadLEBInt();

	m_pDeviceTypeInfo = std::make_unique<DeviceTypeInfo[]>(m_DevTypesCount);
	DeviceTypeInfo  *pDevTypeInfo = m_pDeviceTypeInfo.get();

	for (int i = 0; i < m_DevTypesCount; i++, pDevTypeInfo++)
	{
		pDevTypeInfo->m_pFileReader = this;
		pDevTypeInfo->eDeviceType = ReadLEBInt();
		ReadString(pDevTypeInfo->strDevTypeName);
		printf("\nType %d ", pDevTypeInfo->eDeviceType);
		pDevTypeInfo->DeviceIdsCount = ReadLEBInt();
		printf("IdsCount %d ", pDevTypeInfo->DeviceIdsCount);
		pDevTypeInfo->DeviceParentIdsCount = ReadLEBInt();
		printf("ParentIdsCount %d ", pDevTypeInfo->DeviceParentIdsCount);
		pDevTypeInfo->DevicesCount = ReadLEBInt();
		printf("DevCount %d ", pDevTypeInfo->DevicesCount);
		pDevTypeInfo->VariablesByDeviceCount = ReadLEBInt();
		printf("VarsCount %d", pDevTypeInfo->VariablesByDeviceCount);


		VariableTypeInfo VarTypeInfo;

		for (int iVar = 0; iVar < pDevTypeInfo->VariablesByDeviceCount; iVar++)
		{
			ReadString(VarTypeInfo.Name);
			printf("\nVar %s ", VarTypeInfo.Name.c_str());
			VarTypeInfo.eUnits = ReadLEBInt();
			printf("Units %d ", VarTypeInfo.eUnits);
			int nBitFlags = ReadLEBInt();
			if (nBitFlags & 0x01)
			{
				ReadDouble(VarTypeInfo.Multiplier);
				printf("Mult %g ", VarTypeInfo.Multiplier);
			}
			else
				VarTypeInfo.Multiplier = 1.0;

			VarTypeInfo.nIndex = iVar;

			if (!pDevTypeInfo->m_VarTypes.insert(VarTypeInfo).second)
				throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);
		}

		pDevTypeInfo->AllocateData();

		if (!m_DevTypeSet.insert(pDevTypeInfo).second) 
			throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);

		DeviceInstanceInfo *pDevInst = pDevTypeInfo->m_pDeviceInstances.get();

		for (int iDev = 0; iDev < pDevTypeInfo->DevicesCount; iDev++, pDevInst++)
		{
			pDevInst->nIndex = iDev;

			printf("\n");
			for (int Ids = 0; Ids < pDevTypeInfo->DeviceIdsCount; Ids++)
			{
				uint64_t ReadInt64;
				ReadLEB(ReadInt64);
				printf("Id[%d] %d ", Ids, static_cast<int>(ReadInt64));
				pDevInst->SetId(Ids, static_cast<int>(ReadInt64));
			}

			ReadString(pDevInst->Name);

			for (int Ids = 0; Ids < pDevTypeInfo->DeviceParentIdsCount; Ids++)
			{
				uint64_t nId = 0;
				uint64_t eType = 0;
				ReadLEB(eType);
				ReadLEB(nId);
				printf("Link to %d of type %d ", static_cast<int>(nId), static_cast<int>(eType));
				pDevInst->SetParent(Ids, static_cast<int>(eType), static_cast<int>(nId));
			}
		}
		pDevTypeInfo->IndexDevices();
	}

	Reparent();

	uint64_t nChannelHeadersOffset = -1;
	uint64_t nSlowVarsOffset = -1;
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

	uint64_t nCompressedDataOffset = infile.tellg();

	if (nChannelHeadersOffset < 0)
		throw CFileReadException(infile);
	infile.seekg(nChannelHeadersOffset, std::ios_base::beg);

	uint64_t ReadInt64;
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
		throw CFileReadException(infile);
	infile.seekg(nSlowVarsOffset, std::ios_base::beg);
		
	m_dRatio = static_cast<double>(nSlowVarsOffset - nCompressedDataOffset) / sizeof(double) / m_ChannelsCount / m_PointsCount;

	uint64_t nSlowVarsCount = 0;
	ReadLEB(nSlowVarsCount);
	while (nSlowVarsCount)
	{
		uint64_t nDeviceType = 0;
		uint64_t nKeysSize = 0;
		std::string strVarName;

		ReadLEB(nDeviceType);
		ReadString(strVarName);
		ReadLEB(nKeysSize);

		LONGVECTOR DeviceIds;
		while (nKeysSize)
		{
			uint64_t nId;
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
		infile.seekg(m_nCommentOffset, std::ios_base::beg);
		ReadString(m_strUserComment);
	}

	m_bHeaderLoaded = true;

}


void CResultFileReader::ReadString(std::string& String)
{
	uint64_t nLen64 = 0;
	ReadLEB(nLen64);
	if (nLen64 < 0xffff)
	{
		if (m_nVersion > 1)
		{
			// if file version 2 and above read plain utf-8
			String.resize(static_cast<int>(nLen64), '\x0');
			infile.read(String.data(), static_cast<int>(nLen64));
		}
		else
		{
			// for 1st version read scsu
			std::wstring scsuToDecode;
			CUnicodeSCSU StringWriter(infile);
			StringWriter.ReadSCSU(scsuToDecode, static_cast<int>(nLen64));
			String = stringutils::utf8_encode(scsuToDecode);
		}
	}
}

void CResultFileReader::ReadDouble(double& Value)
{
	infile.read(&Value, sizeof(double));
}

int CResultFileReader::ReadLEBInt()
{
	uint64_t IntToRead;
	ReadLEB(IntToRead);
	if (IntToRead > 0x7fffffff)	// для LEB разрешаем только 31-битное число 
		throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);
	return static_cast<int>(IntToRead);
}
void CResultFileReader::ReadLEB(uint64_t& nValue)
{
	nValue = 0;
	ptrdiff_t shift = 0;
	unsigned char low;
	do
	{
		infile.read(&low, sizeof(low));
		nValue |= ((static_cast<uint64_t>(low)& 0x7f) << shift);
		shift += 7;
		if (shift > 64)
			break;
	} while (low & 0x80);
}


int64_t CResultFileReader::OffsetFromCurrent()
{
	int64_t nCurrentOffset = infile.tellg();
	uint64_t nRelativeOffset = 0;
	ReadLEB(nRelativeOffset);

	if (static_cast<int64_t>(nRelativeOffset) > nCurrentOffset)
		throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);

	if (nRelativeOffset)
		nRelativeOffset = nCurrentOffset - nRelativeOffset;
	
	return nRelativeOffset;
}

void CResultFileReader::Close()
{
	m_strFilePath.clear();
	m_bHeaderLoaded = false;

	infile.close();

	m_DevTypeSet.clear();
	m_dRatio = -1.0;

}

double CResultFileReader::GetFileTime()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_dTimeCreated;
}

const char* CResultFileReader::GetFilePath()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_strFilePath.c_str();
}

const char* CResultFileReader::GetComment()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_strComment.c_str();
}

size_t CResultFileReader::GetPointsCount()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_PointsCount;
}

size_t CResultFileReader::GetChannelsCount()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return m_ChannelsCount;
}

std::unique_ptr<double[]> CResultFileReader::GetTimeStep()
{
	std::unique_ptr<double[]> pResultBuffer = std::make_unique<double[]>(m_PointsCount);
	if (!pResultBuffer)
		throw CFileReadException(infile, CDFW2Messages::m_cszNoMemory);
	GetStep(pResultBuffer.get(), m_PointsCount);
	return pResultBuffer;
}

void CResultFileReader::GetTimeScale(double *pTimeBuffer, size_t nPointsCount)
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	if (nPointsCount <= m_PointsCount && nPointsCount >= 0)
		std::copy(m_pTime.get(), m_pTime.get()+ nPointsCount, pTimeBuffer);
	else
		throw CFileReadException(infile, CDFW2Messages::m_cszNoMemory);
}

void CResultFileReader::GetStep(double *pStepBuffer, size_t nPointsCount)
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	if (nPointsCount <= m_PointsCount && nPointsCount >= 0)
		std::copy(m_pStep.get(), m_pStep.get() + nPointsCount, pStepBuffer);
	else
		throw CFileReadException(infile, CDFW2Messages::m_cszNoMemory);
}

int CResultFileReader::GetVersion()
{
	if (!m_bHeaderLoaded)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);
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
		throw CFileReadException(CDFW2Messages::m_cszWrongResultFile);
}

ptrdiff_t CResultFileReader::DeviceInstanceInfo::GetId(ptrdiff_t nIdIndex) const
{
	if (nIdIndex >= 0 && nIdIndex < m_pDevType->DeviceIdsCount)
		return m_pDevType->pIds[nIndex * m_pDevType->DeviceIdsCount + nIdIndex];
	else
		throw CFileReadException(CDFW2Messages::m_cszWrongResultFile);
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
		throw CFileReadException(CDFW2Messages::m_cszWrongResultFile);
}

const CResultFileReader::DeviceLinkToParent* CResultFileReader::DeviceInstanceInfo::GetParent(ptrdiff_t nParentIndex) const
{
	if (m_pDevType->DeviceParentIdsCount)
	{
		if (nParentIndex >= 0 && nParentIndex < m_pDevType->DeviceParentIdsCount)
			return m_pDevType->pLinks.get() + nIndex * m_pDevType->DeviceParentIdsCount + nParentIndex;
		else
			throw CFileReadException(CDFW2Messages::m_cszWrongResultFile);
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

const CResultFileReader::ChannelHeaderInfo* CResultFileReader::GetChannelHeaders()
{
	return m_pChannelHeaders.get();
}

// возвращает название единиц измерения по заданному типу
const char* CResultFileReader::GetUnitsName(ptrdiff_t eUnitsType)
{
	VARNAMEITRCONST it = m_VarNameMap.find(eUnitsType);
	if (it != m_VarNameMap.end())
		return it->second.c_str();
	else
		return m_cszUnknownUnits;
}


SAFEARRAY* CResultFileReader::CreateSafeArray(std::unique_ptr<double[]>&& pChannelData)
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

const char* CResultFileReader::GetUserComment()
{
	return m_strUserComment.c_str();
}

void CResultFileReader::SetUserComment(std::string_view UserComment)
{
	if (m_nCommentOffset > 0 && m_nCommentDirectoryOffset > 0 && UserComment.length() > 0)
	{
		infile.seekg(m_nCommentOffset, std::ios_base::beg);
		WriteString(UserComment);
		infile.truncate();
		infile.seekg(m_nCommentDirectoryOffset, std::ios_base::beg);
		DataDirectoryEntry de = {2, m_nCommentOffset};
		infile.write(&de, sizeof(DataDirectoryEntry));
		m_strUserComment = UserComment;
	}
}

double CResultFileReader::GetCompressionRatio()
{
	return m_dRatio;
}

// чтение типа блока из файла с проверкой корректности типа
int CResultFileReader::ReadBlockType()
{
	int nBlockType = ReadLEBInt();
	if (nBlockType >= 0 && nBlockType <= 2)
		return nBlockType;
	else
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileWrongCompressedBlockType);
}


const char CResultFile::m_cszSignature[] = { 'R', 'a', 'i', 'd', 'e', 'n' };
const char* CResultFileReader::m_cszUnknownUnits = "???";
