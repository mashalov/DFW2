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
	if (Version64 > Consts::dfw_result_file_version)
		return throw CFileReadException(infile, fmt::format(CDFW2Messages::m_cszResultFileHasNewerVersion, Version, Consts::dfw_result_file_version).c_str());
}

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t Index)
{
	std::unique_ptr<double[]> pResultBuffer;

	if (Index >= 0 && Index < static_cast<ptrdiff_t>(ChannelsCount_))
	{
		ChannelHeaderInfo* pChannel{ pChannelHeaders_.get() + Index };
		CCompressorSingle comp;
		size_t TimeIndex{ 0 };
		double Value{ 0.0 };
		std::unique_ptr<BITWORD[]> pBuffer;

		pResultBuffer = std::make_unique<double[]>(PointsCount_);

		if (!pResultBuffer)
			throw CFileReadException(infile, CDFW2Messages::m_cszNoMemory);

		// получаем список блоков данных канала в порядке от конца к началу
		INT64LIST Offsets;
		GetBlocksOrder(Offsets, pChannel->LastBlockOffset);

		// проходим блоки в обратном порядке от начала к концу
		auto it{ Offsets.rbegin() };
		while (it != Offsets.rend())
		{
			infile.seekg(*it, std::ios_base::beg);

			int BlockType{ ReadBlockType() };
			int PointsCount{ ReadLEBInt() };

			pBuffer = nullptr;
			int BytesCount{ 1 };

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
				size_t DecomprSize{ PointsCount * (sizeof(double) + 1) };
				pBuffer = std::make_unique<BITWORD[]>(DecomprSize / sizeof(BITWORD) + 1);
				bool bRes{ rle.Decompress(pReadBuffer.get(), BytesCount, static_cast<unsigned char*>(static_cast<void*>(pBuffer.get())), DecomprSize) };
				if (!bRes)
					throw CFileReadException();
				BytesCount = static_cast<int>(DecomprSize);
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
			for (int nPoint = 0; nPoint < PointsCount; nPoint++, TimeIndex++)
			{
				double pred = comp.Predict(pTime_[TimeIndex]);
				Value = 0.0;
				comp.ReadDouble(Value, pred, Input);
				pResultBuffer[TimeIndex] = Value;
				comp.UpdatePredictor(Value);
				if (BlockType == 2)
					Input.Rewind();
			}
			it++;
		}

		if (TimeIndex != PointsCount_)
			throw CFileReadException(infile, fmt::format(CDFW2Messages::m_cszResultFilePointsCountMismatch, Index, TimeIndex, PointsCount_).c_str());
	}
	return pResultBuffer;
}


ptrdiff_t CResultFileReader::GetChannelIndex(ptrdiff_t eType, ptrdiff_t Id, std::string_view VarName)
{

	ptrdiff_t nVarIndex{ -1 };

	DeviceTypeInfo findType;
	findType.eDeviceType = static_cast<int>(eType);
	auto it{ DevTypeSet_.find(&findType) };
	if (it != DevTypeSet_.end())
	{
		VariableTypeInfo findVar; 
		findVar.Name = VarName;
		auto vit = (*it)->VarTypes_.find(findVar);
		if (vit != (*it)->VarTypes_.end())
			return GetChannelIndex(eType, Id, vit->Index);
	}
	return -1;
}


ptrdiff_t CResultFileReader::GetChannelIndex(ptrdiff_t eType, ptrdiff_t Id, ptrdiff_t nVarIndex)
{
	
	ChannelHeaderInfo findChannel;
	findChannel.DeviceId = Id;
	findChannel.DeviceVarIndex = static_cast<int>(nVarIndex);
	findChannel.eDeviceType = static_cast<int>(eType);
	auto it{ ChannelSet_.find(&findChannel) };
	if (it != ChannelSet_.end())
		return *it - pChannelHeaders_.get();
	else
		return -1;
}

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t eType, ptrdiff_t Id, ptrdiff_t nVarIndex)
{
	return ReadChannel(GetChannelIndex(eType,Id,nVarIndex));
}

std::unique_ptr<double[]> CResultFileReader::ReadChannel(ptrdiff_t eType, ptrdiff_t Id, std::string_view VarName)
{
	return ReadChannel(GetChannelIndex(eType, Id, VarName));
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

		int nBlockType{ ReadBlockType() };			// Block Type
		ReadLEBInt();								// doubles count
		// если блок имеет тип 2 - (SuperRLE) то размер блока 1 байт. Размер не записывается для такого блока
		// поэтому принимаем его по умолчанию
		int Bytes{ 1 };
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
	bool bRes{ false };
	ChannelHeaderInfo *pChannel = pChannelHeaders_.get();
	while (pChannel < pChannelHeaders_.get() + ChannelsCount_)
	{
		if (pChannel->eDeviceType == DEVTYPE_MODEL &&
			pChannel->DeviceId == 0 &&
			pChannel->DeviceVarIndex == nVarIndex)
			break;
		pChannel++;
	}

	size_t TimeIndex{ 0 };

	if (pChannel < pChannelHeaders_.get() + ChannelsCount_)
	{
		// получаем список блоков данных канала в порядке от конца к началу
		INT64LIST Offsets;
		GetBlocksOrder(Offsets, pChannel->LastBlockOffset);

		// проходим блоки в обратном порядке от начала к концу
		auto it{ Offsets.rbegin() };
		while (it != Offsets.rend())
		{
			infile.seekg(*it, std::ios_base::beg);
			ReadBlockType(); // Block Type
			int PointsCount{ ReadLEBInt() };
			ReadLEBInt(); // Bytes
			for (int nPoint = 0; nPoint < PointsCount; nPoint++)
			{
				ReadDouble(pData[TimeIndex]);
				if (++TimeIndex > PointsCount_)
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
	DirectoryEntriesCount_ = static_cast<size_t>(nDirEntries);
	pDirectoryEntries_ = std::make_unique<DataDirectoryEntry[]>(DirectoryEntriesCount_);
	DirectoryOffset_ = infile.tellg();
	infile.read(pDirectoryEntries_.get(), DirectoryEntriesCount_ * sizeof(struct DataDirectoryEntry));
}

void CResultFileReader::Reparent()
{
	for (const auto& it : DevTypeSet_)
	{
		DeviceTypeInfo* pDevType{ it };

		if (pDevType->DeviceParentIdsCount == 0) continue;

		DeviceInstanceInfo* pb{ pDevType->pDeviceInstances_.get() };
		const DeviceInstanceInfo* const pe{ pb + pDevType->DevicesCount };

		while (pb < pe)
		{
			const DeviceLinkToParent *pLink = pb->GetParent(0);
			if (pLink && pLink->eParentType != 0)
			{
				DeviceTypeInfo findType;
				findType.eDeviceType = static_cast<int>(pLink->eParentType);
				if (auto ftypeit{ DevTypeSet_.find(&findType) }; ftypeit != DevTypeSet_.end())
				{
					DeviceTypeInfo* pParentTypeInfo{ *ftypeit };
					DeviceInstanceInfoBase findInstance;
					findInstance.Index_ = pLink->Id;

					if (pParentTypeInfo->DevSet_.find(&findInstance) == pParentTypeInfo->DevSet_.end())
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
	Ratio_ = 0.0;
	FilePath_ = FilePath;

	infile.open(FilePath, std::ios_base::in|std::ios_base::binary|std::ios_base::out);

	size_t CountSignature{ sizeof(m_cszSignature) };
	char SignatureBuffer[sizeof(m_cszSignature)];
	infile.read(SignatureBuffer, CountSignature);
	if (memcmp(m_cszSignature, SignatureBuffer, CountSignature))
		throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);
	ReadHeader(Version_);
	ReadDouble(TimeCreated_);
	VARIANT vt;
	VariantInit(&vt);
	vt.vt = VT_DATE;
	vt.date = TimeCreated_;

	if(!SUCCEEDED(VariantChangeType(&vt, &vt, 0, VT_BSTR)))
		throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);

	std::string strDateTime(stringutils::utf8_encode(vt.bstrVal));
	//printf("\nCreated %s ", strDateTime.c_str());

	ReadString(Comment_);
	ReadDirectoryEntries();

	int VarNameCount{ ReadLEBInt() };
	VarNameMap_.clear();

	// читаем типы и названия единиц измерения
	for (int varname = 0; varname < VarNameCount; varname++)
	{
		int VarType = ReadLEBInt();		// тип
		std::string strVarName;
		ReadString(strVarName);			// название
		// тип и название вводим в карту
		if (!VarNameMap_.insert(make_pair(VarType,strVarName)).second)
			throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);
	}

	DevTypesCount_ = ReadLEBInt();

	pDeviceTypeInfo_ = std::make_unique<DeviceTypeInfo[]>(DevTypesCount_);
	DeviceTypeInfo* pDevTypeInfo{ pDeviceTypeInfo_.get() };

	for (int i = 0; i < DevTypesCount_; i++, pDevTypeInfo++)
	{
		pDevTypeInfo->pFileReader_= this;
		pDevTypeInfo->eDeviceType = ReadLEBInt();
		ReadString(pDevTypeInfo->DevTypeName_);
		//printf("\nType %d ", pDevTypeInfo->eDeviceType);
		pDevTypeInfo->DeviceIdsCount = ReadLEBInt();
		//printf("IdsCount %d ", pDevTypeInfo->DeviceIdsCount);
		pDevTypeInfo->DeviceParentIdsCount = ReadLEBInt();
		//printf("ParentIdsCount %d ", pDevTypeInfo->DeviceParentIdsCount);
		pDevTypeInfo->DevicesCount = ReadLEBInt();
		//printf("DevCount %d ", pDevTypeInfo->DevicesCount);
		pDevTypeInfo->VariablesByDeviceCount = ReadLEBInt();
		//printf("VarsCount %d", pDevTypeInfo->VariablesByDeviceCount);


		VariableTypeInfo VarTypeInfo;

		for (int iVar = 0; iVar < pDevTypeInfo->VariablesByDeviceCount; iVar++)
		{
			ReadString(VarTypeInfo.Name);
			//printf("\nVar %s ", VarTypeInfo.Name.c_str());
			VarTypeInfo.eUnits = ReadLEBInt();
			//printf("Units %d ", VarTypeInfo.eUnits);
			int nBitFlags = ReadLEBInt();
			if (nBitFlags & 0x01)
			{
				ReadDouble(VarTypeInfo.Multiplier);
				//printf("Mult %g ", VarTypeInfo.Multiplier);
			}
			else
				VarTypeInfo.Multiplier = 1.0;

			VarTypeInfo.Index = iVar;

			if (!pDevTypeInfo->VarTypes_.insert(VarTypeInfo).second)
				throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);
		}

		pDevTypeInfo->AllocateData();

		if (!DevTypeSet_.insert(pDevTypeInfo).second) 
			throw CFileReadException(infile, CDFW2Messages::m_cszWrongResultFile);

		DeviceInstanceInfo* pDevInst{ pDevTypeInfo->pDeviceInstances_.get() };

		for (int iDev = 0; iDev < pDevTypeInfo->DevicesCount; iDev++, pDevInst++)
		{
			pDevInst->Index_ = iDev;

			//printf("\n");
			for (int Ids = 0; Ids < pDevTypeInfo->DeviceIdsCount; Ids++)
			{
				uint64_t ReadInt64;
				ReadLEB(ReadInt64);
				//printf("Id[%d] %d ", Ids, static_cast<int>(ReadInt64));
				pDevInst->SetId(Ids, static_cast<int>(ReadInt64));
			}

			ReadString(pDevInst->Name);

			for (int Ids = 0; Ids < pDevTypeInfo->DeviceParentIdsCount; Ids++)
			{
				uint64_t Id = 0;
				uint64_t eType = 0;
				ReadLEB(eType);
				ReadLEB(Id);
				//printf("Link to %d of type %d ", static_cast<int>(Id), static_cast<int>(eType));
				pDevInst->SetParent(Ids, static_cast<int>(eType), static_cast<int>(Id));
			}
		}
		pDevTypeInfo->IndexDevices();
	}

	Reparent();

	uint64_t ChannelHeadersOffset(-1);
	uint64_t SlowVarsOffset(-1);

	CommentOffset_ = CommentDirectoryOffset_ = -1;

	for (size_t i = 0; i < DirectoryEntriesCount_; i++)
	{
		switch (pDirectoryEntries_[i].DataType)
		{
		case 0:
			if (ChannelHeadersOffset == -1)
				ChannelHeadersOffset = pDirectoryEntries_[i].Offset;
			break;
		case 1:
			if (SlowVarsOffset == -1)
				SlowVarsOffset = pDirectoryEntries_[i].Offset;
			break;
		case 2:
			if (CommentOffset_ == -1)
			{
				CommentOffset_ = pDirectoryEntries_[i].Offset;
				CommentDirectoryOffset_ = DirectoryOffset_ + i * sizeof(DataDirectoryEntry);
			}
			break;
		}
	}

	uint64_t CompressedDataOffset(infile.tellg());

	if (ChannelHeadersOffset < 0)
		throw CFileReadException(infile);
	infile.seekg(ChannelHeadersOffset, std::ios_base::beg);

	uint64_t ReadInt64;
	ReadLEB(ReadInt64);
	PointsCount_ = static_cast<size_t>(ReadInt64);
	ReadLEB(ReadInt64);
	ChannelsCount_ = static_cast<size_t>(ReadInt64);

	pChannelHeaders_ = std::make_unique<ChannelHeaderInfo[]>(ChannelsCount_);
	ChannelHeaderInfo *pChannel = pChannelHeaders_.get();

	ChannelHeadersRange_ = { pChannel, pChannel + ChannelsCount_ };

	for (size_t nChannel = 0; nChannel < ChannelsCount_; nChannel++, pChannel++)
	{
		pChannel->eDeviceType = ReadLEBInt();
		ReadLEB(pChannel->DeviceId);
		pChannel->DeviceVarIndex = ReadLEBInt();
		pChannel->LastBlockOffset = OffsetFromCurrent();
		bool bInserted{ ChannelSet_.insert(pChannel).second };
		_ASSERTE(bInserted);
	}


	pTime_ = std::make_unique<double[]>(PointsCount_);
	pStep_ = std::make_unique<double[]>(PointsCount_);
	ReadModelData(pTime_,0);
	ReadModelData(pStep_,1);


	if (SlowVarsOffset < 0)
		throw CFileReadException(infile);
	infile.seekg(SlowVarsOffset, std::ios_base::beg);
		
	// рассчитываем коэффициент сжатия если исходный размер ненулевой
	Ratio_ = static_cast<double>(ChannelsCount_ * PointsCount_ * sizeof(double));
	if(Ratio_ > 0.0)
		Ratio_ = static_cast<double>(SlowVarsOffset - CompressedDataOffset) / Ratio_;

	uint64_t SlowVarsCount{ 0 };
	ReadLEB(SlowVarsCount);
	while (SlowVarsCount)
	{
		uint64_t DeviceType{ 0 };
		uint64_t KeysSize{ 0 };
		std::string VarName;

		ReadLEB(DeviceType);
		ReadString(VarName);
		ReadLEB(KeysSize);

		ResultIds DeviceIds;

		while (KeysSize)
		{
			uint64_t Id;
			ReadLEB(Id);
			DeviceIds.push_back(static_cast<long>(Id));
			KeysSize--;
		}

		CSlowVariableItem *pItm = new CSlowVariableItem(static_cast<long>(DeviceType), DeviceIds, VarName.c_str());

		ReadLEB(KeysSize); // graph size now

		while (KeysSize)
		{
			double Time{ 0.0 }, Value{ 0.0 };
			ReadDouble(Time);
			ReadDouble(Value);
			ReadString(VarName);
			CSlowVariableGraphItem itg(Time, Value, VarName.c_str());
			pItm->Graph_.insert(itg);
			KeysSize--;
		}

		SlowVariables_.insert(pItm);
		SlowVarsCount--;
	}

	if (CommentOffset_ > 0)
	{
		infile.seekg(CommentOffset_, std::ios_base::beg);
		ReadString(UserComment_);
	}

	bHeaderLoaded_ = true;

}


void CResultFileReader::ReadString(std::string& String)
{
	uint64_t nLen64 = 0;
	ReadLEB(nLen64);
	if (nLen64 < 0xffff)
	{
		if (Version_ > 1)
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
	FilePath_.clear();
	bHeaderLoaded_ = false;

	infile.close();

	DevTypeSet_.clear();
	Ratio_ = 0.0;

}

double CResultFileReader::GetFileTime()
{
	if (!bHeaderLoaded_)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return TimeCreated_;
}

const char* CResultFileReader::GetFilePath()
{
	if (!bHeaderLoaded_)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return FilePath_.c_str();
}

const char* CResultFileReader::GetComment()
{
	if (!bHeaderLoaded_)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return Comment_.c_str();
}

size_t CResultFileReader::GetPointsCount()
{
	if (!bHeaderLoaded_)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return PointsCount_;
}

size_t CResultFileReader::GetChannelsCount()
{
	if (!bHeaderLoaded_)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	return ChannelsCount_;
}

std::unique_ptr<double[]> CResultFileReader::GetTimeStep()
{
	std::unique_ptr<double[]> pResultBuffer = std::make_unique<double[]>(PointsCount_);
	if (!pResultBuffer)
		throw CFileReadException(infile, CDFW2Messages::m_cszNoMemory);
	GetStep(pResultBuffer.get(), PointsCount_);
	return pResultBuffer;
}

void CResultFileReader::GetTimeScale(double *pTimeBuffer, size_t nPointsCount)
{
	if (!bHeaderLoaded_)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	if (nPointsCount <= PointsCount_ && nPointsCount >= 0)
		std::copy(pTime_.get(), pTime_.get() + nPointsCount, pTimeBuffer);
	else
		throw CFileReadException(infile, CDFW2Messages::m_cszNoMemory);
}

void CResultFileReader::GetStep(double *pStepBuffer, size_t nPointsCount)
{
	if (!bHeaderLoaded_)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);

	if (nPointsCount <= PointsCount_ && nPointsCount >= 0)
		std::copy(pStep_.get(), pStep_.get() + nPointsCount, pStepBuffer);
	else
		throw CFileReadException(infile, CDFW2Messages::m_cszNoMemory);
}

int CResultFileReader::GetVersion()
{
	if (!bHeaderLoaded_)
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileNotLoadedProperly);
	return Version_;
}

const DEVTYPESET& CResultFileReader::GetTypesSet() const
{
	return DevTypeSet_;
}

const CResultFileReader::ChannelHeaderInfo* CResultFileReader::GetChannelHeaders()
{
	return pChannelHeaders_.get();
}

CResultFileReader::ChannelHeadersRange CResultFileReader::GetChannelHeadersRange() const
{
	return ChannelHeadersRange_;
}

// возвращает название единиц измерения по заданному типу
const char* CResultFileReader::GetUnitsName(ptrdiff_t eUnitsType)
{
	return VarNameMap_.VerbalUnits(eUnitsType).c_str();
}


SAFEARRAY* CResultFileReader::CreateSafeArray(std::unique_ptr<double[]>&& pChannelData)
{
	SAFEARRAY* pSA{ nullptr };
	try
	{
		const size_t nPointsCount{ GetPointsCount() };
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
					HRESULT hRes{ SafeArrayUnaccessData(pSA) };
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
	return UserComment_.c_str();
}

void CResultFileReader::SetUserComment(std::string_view UserComment)
{
	if (CommentOffset_ > 0 && CommentDirectoryOffset_ > 0 && UserComment.length() > 0)
	{
		infile.seekg(CommentOffset_, std::ios_base::beg);
		WriteString(UserComment);
		infile.truncate();
		infile.seekg(CommentDirectoryOffset_, std::ios_base::beg);
		DataDirectoryEntry de {2, CommentOffset_ };
		infile.write(&de, sizeof(DataDirectoryEntry));
		UserComment_ = UserComment;
	}
}

double CResultFileReader::GetCompressionRatio()
{
	return Ratio_;
}

// чтение типа блока из файла с проверкой корректности типа
int CResultFileReader::ReadBlockType()
{
	int nBlockType{ ReadLEBInt() };
	if (nBlockType >= 0 && nBlockType <= 2)
		return nBlockType;
	else
		throw CFileReadException(infile, CDFW2Messages::m_cszResultFileWrongCompressedBlockType);
}
