#include "stdafx.h"
#include "ResultFileWriter.h"
#include "memory.h"

using namespace DFW2;

void CResultFile::WriteLEB(uint64_t Value)
{
	do
	{
		unsigned char low = Value & 0x7f;
		Value >>= 7;
		if (Value != 0)
			low |= 0x80;
		infile.write(&low, sizeof(unsigned char));
	} while (Value != 0);
}

void CResultFile::WriteString(std::string_view cszString)
{
	if (cszString.empty())
		WriteLEB(0);
	else
	{
		
#if DFW2_RESULTFILE_VERSION > 1
		WriteLEB(cszString.size());
		// for file versions > 1 write plain utf-8 strings
		infile.write(cszString.data(), cszString.size());
#else
		// for older versions write scsu
		std::wstring scsuToEncode = stringutils::utf8_decode(cszString);
		WriteLEB(scsuToEncode.size());
		CUnicodeSCSU StringWriter(infile);
		StringWriter.WriteSCSU(scsuToEncode);
#endif
	}
}

void CResultFileWriter::WriteTime(double Time, double Step)
{
	SetTime_ = Time; 
	SetStep_ = Step;
	bPredictorReset_ = false;

	if (PredictorOrder_ > 0)
	{
		bool bReset = false;
		for (int j = 0; j < PredictorOrder_; j++)
		{
			if (Consts::Equal(Time, ts[j]))
			{
				bReset = true;
				break;
			}
		}

		if (bReset)
		{
			CChannelEncoder* pEncoder{ pEncoders_.get() };
			const CChannelEncoder* const pEncoderEnd{ pEncoder + ChannelsCount_ - 2 };

			while (pEncoder < pEncoderEnd)
			{
				pEncoder->Compressor_.ResetPredictor(PredictorOrder_);
				pEncoder++;
			}

			// если новое время равно текущему, предиктор нужно сбросить
			// порядок предиктора обнуляем, но используем флаг m_bPredictorReset
			// который нужен для того, чтобы использовать для предиктора последнее
			// значение, а не ноль (см. Predict)

			PredictorOrder_ = 0;
			bPredictorReset_ = true;
		}
	}

	UpdateLagrangeCoefficients(Time);

	WriteChannel(ChannelsCount_ - 2, Time);
	WriteChannel(ChannelsCount_ - 1, Step);
	PointsCount_++;
}

void CResultFileWriter::FlushChannels()
{ 
	// если каналы уже были сброшены - выходим
	if (bChannelsFlushed_)
		return;
	TerminateWriterThread();

	for (size_t nChannel = 0; nChannel < ChannelsCount_; nChannel++)
	{
		FlushChannel(nChannel);
		// сбрасываем SuperRLE каналы, в которых на всех точках были одинаковые значения
		FlushSuperRLE(pEncoders_[nChannel]);
	}

	struct DataDirectoryEntry de = { 0, 0 };

	de.Offset = infile.tellg();
	infile.seekg(DataDirectoryOffset_, std::ios_base::beg);
	infile.write(&de, sizeof(struct DataDirectoryEntry));
	infile.seekg(de.Offset, std::ios_base::beg);
	WriteLEB(PointsCount_);
	WriteLEB(ChannelsCount_);

	CChannelEncoder* pEncoder{ pEncoders_.get() };
	const CChannelEncoder* const pEncoderEnd{ pEncoder + ChannelsCount_ - 2 };

	if (pEncoder)
	{
		while (pEncoder < pEncoderEnd)
		{
			WriteChannelHeader(pEncoder - pEncoders_.get(),
				pEncoder->DeviceType_,
				pEncoder->DeviceId_,
				pEncoder->VariableIndex_);
			pEncoder++;
		}
	}

	WriteChannelHeader(ChannelsCount_ - 2, DEVTYPE_MODEL, 0, 0);
	WriteChannelHeader(ChannelsCount_ - 1, DEVTYPE_MODEL, 0, 1);

	
	// write slow variables
	de.DataType = 1;
	de.Offset = infile.tellg();
	infile.seekg(DataDirectoryOffset_ + sizeof(struct DataDirectoryEntry), std::ios_base::beg);
	infile.write(&de, sizeof(struct DataDirectoryEntry));
	infile.seekg(de.Offset, std::ios_base::beg);

	WriteLEB(SlowVariables_.size());
	for (auto &di : SlowVariables_)
	{
		if (!di->Graph_.empty())
		{
			WriteLEB(di->DeviceTypeId_);
			WriteString(di->VarName_);
			WriteLEB(di->DeviceIds_.size());

			for (auto &idi : di->DeviceIds_)
				WriteLEB(idi);
			WriteLEB(di->Graph_.size());

			for (auto &gi : di->Graph_)
			{
				WriteDouble(gi.Time_);
				WriteDouble(gi.Value_);
				WriteString(gi.Description_);
			}
		}
	}

	de.DataType = 2;
	de.Offset = infile.tellg();
	WriteString("");

	infile.seekg(DataDirectoryOffset_ + sizeof(struct DataDirectoryEntry) * 2, std::ios_base::beg);
	infile.write(&de, sizeof(struct DataDirectoryEntry));

	// ставим признак сброса каналов
	bChannelsFlushed_ = true;
}

void CResultFileWriter::FlushSuperRLE(CChannelEncoder& Encoder)
{
	if (Encoder.UnwrittenSuperRLECount_)
	{
		int64_t CurrentSeek (infile.tellg());
		WriteLEB(2);
		WriteLEB(Encoder.UnwrittenSuperRLECount_);
		infile.write(&Encoder.SuperRLEByte_, sizeof(unsigned char));
		WriteLEB(OffsetFromCurrent(Encoder.PreviousSeek_));
		Encoder.PreviousSeek_ = CurrentSeek;
		Encoder.UnwrittenSuperRLECount_ = 0;
	}
}

void CResultFileWriter::FlushChannel(ptrdiff_t Index)
{
	if (Index >= 0 && Index < static_cast<ptrdiff_t>(ChannelsCount_))
	{
		CChannelEncoder& Encoder{ pEncoders_[Index] };
		CCompressorParallel& FloatCompressor{ Encoder.Compressor_ };
		CBitStream& Output{ Encoder.Output_ };

		if (Encoder.Count_)
		{
			size_t CompressedSize(BufferLength_ * sizeof(BITWORD));
			bool bAllBytesEqual{ false };
			if (Index < static_cast<ptrdiff_t>(ChannelsCount_ - 2) &&
				EncodeRLE(Output.BytesBuffer(), Output.BytesWritten(), pCompressedBuffer_.get(), CompressedSize, bAllBytesEqual))
			{	
				if (bAllBytesEqual)
				{
					// все байты во входном буфере RLE одинаковые
					if (Encoder.UnwrittenSuperRLECount_ > 0)
					{
						// уже есть блок данных с одинаковыми байтами
						// проверяем, это были те же байты, что пришли сейчас ?
						if(Encoder.SuperRLEByte_ == *Output.BytesBuffer())
							Encoder.UnwrittenSuperRLECount_ += Encoder.Count_;	// если да, просто увеличиваем счетчик байтов
						else
						{
							// накопленные байты отличаются от тех, что пришли,
							// поэтому сбрасываем старый RLE блок и начинаем записывать новый
							FlushSuperRLE(Encoder);
							Encoder.PreviousSeek_ = infile.tellg();
							Encoder.UnwrittenSuperRLECount_ = Encoder.Count_;
							Encoder.SuperRLEByte_ = *Output.BytesBuffer();
						}
					}
					else
					{
						// блока с одинаковыми данными не было, начинаем его записывать
						Encoder.UnwrittenSuperRLECount_ = Encoder.Count_;
						Encoder.SuperRLEByte_ = *Output.BytesBuffer();
					}
				}
				else
				{
					// сбрасываем блок SuperRLE если был
					FlushSuperRLE(Encoder);
					int64_t CurrentSeek (infile.tellg());
					WriteLEB(0);						// type of block 0 - RLE data
					WriteLEB(Encoder.Count_);			// count of doubles
					WriteLEB(CompressedSize);			// byte length of RLE data
					infile.write(pCompressedBuffer_.get(), CompressedSize);
					WriteLEB(OffsetFromCurrent(Encoder.PreviousSeek_));
					Encoder.PreviousSeek_ = CurrentSeek;
				}
			}
			else
			{
				// сбрасываем блок SuperRLE если был
				FlushSuperRLE(Encoder);
				int64_t CurrentSeek (infile.tellg());
				WriteLEB(1);						// type of block 1 - RAW compressed data
				WriteLEB(Encoder.Count_);			// count of doubles
				WriteLEB(Output.BytesWritten());	// byte length of block
				infile.write(Output.Buffer(), Output.BytesWritten());
				WriteLEB(OffsetFromCurrent(Encoder.PreviousSeek_));
				Encoder.PreviousSeek_ = CurrentSeek;
			}
			Encoder.Count_ = 0;
			Output.Reset();
		}
	}
	else
		throw CFileWriteException(infile);
}

void CResultFileWriter::WriteChannel(ptrdiff_t Index, double Value)
{
	if (Index >= 0 && Index < static_cast<ptrdiff_t>(ChannelsCount_))
	{
		CChannelEncoder &Encoder = pEncoders_[Index];
		CCompressorParallel& FloatCompressor = Encoder.Compressor_;
		CBitStream& Output = Encoder.Output_;

		eFCResult Result{ eFCResult::FC_OK };

		///*
		if (static_cast<ptrdiff_t>(ChannelsCount_ - 2) <= Index)
		{
			
			Result = Output.WriteDouble(Value);
			if (Result == eFCResult::FC_BUFFEROVERFLOW)
			{
				FlushChannel(Index);
				Result = Output.WriteDouble(Value);
				if (Result == eFCResult::FC_ERROR)
					throw CFileWriteException(infile);
				else
					if (Result == eFCResult::FC_OK)
						Encoder.Count_++;
			}
			else
				if (Result == eFCResult::FC_ERROR)
					throw CFileWriteException(infile);
				else
					if (Result == eFCResult::FC_OK)
						Encoder.Count_++;

		}
		else
		//*/
		{

			double pred{ 0.0 };
			/*
			if (static_cast<ptrdiff_t>(m_nChannelsCount - 2) <= nIndex)
			{
				pred = FloatCompressor.Predict(m_dSetTime, false, 1, ls);
				FloatCompressor.UpdatePredictor(Value, 0, 0);
				Result = FloatCompressor.WriteDouble(Value, pred, Output);
			}
			else
			*/
			{
				pred = FloatCompressor.Predict(SetTime_, bPredictorReset_, PredictorOrder_, ls);
				FloatCompressor.UpdatePredictor(Value, PredictorOrder_, NoChangeTolerance_);
				Result = FloatCompressor.WriteDouble(Value, pred, Output);
			}

			if (Result == eFCResult::FC_BUFFEROVERFLOW)
			{
				FlushChannel(Index);
				Result = FloatCompressor.WriteDouble(Value, pred, Output);
				if (Result == eFCResult::FC_ERROR)
					throw CFileWriteException(infile);
				else
					if (Result == eFCResult::FC_OK)
						Encoder.Count_++;
			}
			else
				if (Result == eFCResult::FC_ERROR)
					throw CFileWriteException(infile);
				else
					if (Result == eFCResult::FC_OK)
						Encoder.Count_++;
		}
	}
	else
		throw CFileWriteException(infile);
}

void CResultFileWriter::WriteChannelHeader(ptrdiff_t Index, ptrdiff_t Type, ptrdiff_t Id, ptrdiff_t nVarIndex)
{
	if (Index >= 0 && Index < static_cast<ptrdiff_t>(ChannelsCount_))
	{
		WriteLEB(Type);
		WriteLEB(Id);
		WriteLEB(nVarIndex);
		WriteLEB(OffsetFromCurrent(pEncoders_[Index].PreviousSeek_));
	}
	else
		throw CFileWriteException(infile);
}

void CResultFileWriter::PrepareChannelCompressor(size_t nChannelsCount)
{
	pEncoders_ = std::make_unique<CChannelEncoder[]>(ChannelsCount_ = nChannelsCount + 2);
	BufferLength_ *= sizeof(double) / sizeof(BITWORD);
	pCompressedBuffer_ = std::make_unique<unsigned char[]>(BufferLength_ * sizeof(BITWORD));
	
	size_t BufferGroup{ BufferGroup_ };
	size_t Seek{ 0 };
	
	BITWORD *pBuffer(nullptr);

	for (size_t nChannel = 0; nChannel < ChannelsCount_; nChannel++)
	{
		if (BufferGroup == BufferGroup_)
		{
			size_t nAllocSize{ BufferGroup_ };
			if (ChannelsCount_ - nChannel < BufferGroup_)
				nAllocSize = ChannelsCount_ - nChannel;

			pBuffer = new BITWORD[BufferLength_ * nAllocSize]();
			BufferBegin_.push_back(pBuffer);
			Seek = 0;
		}
		pEncoders_[nChannel].Output_.Init(pBuffer + Seek, pBuffer + Seek + BufferLength_, 0);
		pEncoders_[nChannel].Output_.Reset();
		BufferGroup--;
		if (!BufferGroup)
			BufferGroup = BufferGroup_;
		Seek += BufferLength_;
	}

	pEncoders_[ChannelsCount_ - 1].Compressor_.UpdatePredictor(0, 0);
	pEncoders_[ChannelsCount_ - 2].Compressor_.UpdatePredictor(0, 0);
}

void CResultFileWriter::SetNoChangeTolerance(double Tolerance)
{
	NoChangeTolerance_ = Tolerance;
}

CResultFileWriter::~CResultFileWriter()
{
	try
	{
		Close();
	}
	catch (...)  { }

	for (auto& it : DevTypeSet_)
		delete it;

	DevTypeSet_.clear();
}

// завершает и закрывает поток записи
void CResultFileWriter::TerminateWriterThread()
{
	// если поток записи еще работает
	if (threadWriter.joinable())
	{
		while(bThreadRun_)
		{
			// берем мьютекс доступа к данным
			std::unique_lock<std::mutex> dataGuard(mutexData);
			// убираем флаг работы потока
			bThreadRun_ = false;
			// снимаем поток с ожидания
			conditionRun.notify_all();
			conditionDone.wait_for(dataGuard, std::chrono::milliseconds(10), [this]() { return  this->portionSent == this->portionReceived; });
		}

		// ждем завершения функции потока
		if(threadWriter.joinable())
			threadWriter.join();
	}
}

// закрывает файл записываемых результатов
void CResultFileWriter::Close()
{
	// сбрасываем каналы, если еще не были сброшены
	FlushChannels();
	// сначала останавливаем поток записи
	TerminateWriterThread();

	infile.close();

	for (auto&& it : BufferBegin_)
		delete it;

	BufferBegin_.clear();
}

// создает файл результатов
void CResultFileWriter::CreateResultFile(std::filesystem::path FilePath)
{
	infile.open(FilePath.string(), std::ios_base::out|std::ios_base::binary);
	// запись сигнатуры
	size_t nCountSignature = sizeof(m_cszSignature);
	infile.write(m_cszSignature, nCountSignature);
	// запись версии (версия в define, соответствует исходнику)
	WriteLEB(Consts::dfw_result_file_version);

	// создаем поток для записи
	threadWriter = std::thread(CResultFileWriter::WriterThread, this); 
	if(!threadWriter.joinable())
		throw CFileWriteException(infile);

	// раз создали файл результатов - потребуется финализация
	bChannelsFlushed_ = false;
}

// записывает double без сжатия
void CResultFileWriter::WriteDouble(const double &Value)
{
	infile.write(&Value, sizeof(double));
}

// записывает заданное количество описателей разделов
void CResultFileWriter::AddDirectoryEntries(size_t nDirectoryEntriesCount)
{
	WriteLEB(nDirectoryEntriesCount);							// записываем количество разделов
	struct DataDirectoryEntry DirEntry = { 0, 0LL };			// создаем пустой раздел
	DataDirectoryOffset_ = infile.tellg();						//	запоминаем позицию начала разделов в файле
	// записываем заданное количество пустых разделов
	for (size_t i = 0; i < nDirectoryEntriesCount; i++)
		infile.write(&DirEntry, sizeof(struct DataDirectoryEntry));
}

// запись результатов вне потока
void CResultFileWriter::WriteResults(double Time, double Step)
{
	// проверяем активен ли поток записи
	if (threadWriter.joinable())
	{
		{
			// если число записей в энкодере меньше, чем число вызовов записи результатов
			while (portionSent > portionReceived)
			{
				// скорее всего поток записи не запустился при прошлом вызове
				// запускаем его
				std::unique_lock<std::mutex> doneGuard(mutexDone);
				conditionRun.notify_all();
				// и ждем возврата 10мс, после чего снова проверяем равенство записей (самый надежный и кондовый способ синхронизации)
				conditionDone.wait_for(doneGuard, std::chrono::milliseconds(10), [this]() { return  this->portionSent == this->portionReceived; });
			}


			// ждем освобождения данных
			std::lock_guard<std::mutex> dataGuard(mutexData);

			try
			{
				// сохраняем результаты из указателей во внутрениий буфер
				CChannelEncoder* pEncoder{ pEncoders_.get() };
				const CChannelEncoder* const pEncoderEnd{ pEncoder + ChannelsCount_ - 2 };
				while (pEncoder < pEncoderEnd)
				{
					pEncoder->Value_ = *pEncoder->pVariable_;
					pEncoder++;
				}

				TimeToWrite_ = Time;
				StepToWrite_ = Step;
			}
			catch (...)
			{
				throw CFileWriteException(infile);
			}
			// по завершению копирования результатов запускаем поток записи
		}

		// запускаем поток
		conditionRun.notify_all();
		// считаем количество отданных в энкодер записей
		portionSent++;
	}
	else
		throw CFileWriteException(infile);
}

// поток записи результатов
unsigned int CResultFileWriter::WriterThread(void* pThis)
{
	try
	{
		// получаем объект из параметра функции потока
		CResultFileWriter* pthis = static_cast<CResultFileWriter*>(pThis);

		if (!pthis)
			throw CFileWriteException("Threading problem");
;
		while (pthis->bThreadRun_)
		{
			{
				// ждем команды на запуск записи или останов
				std::unique_lock<std::mutex> runGuard(pthis->mutexRun);
				pthis->conditionRun.wait(runGuard);
				std::lock_guard<std::mutex> dataGuard(pthis->mutexData);

				// если останов - выходим
				if (!pthis->bThreadRun_)
					break;

				// записываем очередной блок результатов

				if (!pthis->WriteResultsThreaded())
					throw CFileWriteException(pthis->infile);

			}
			// считаем количество записей
			pthis->portionReceived++; 
			// выдаем ивент что закончили работу
			pthis->conditionDone.notify_all();
		}
	}
	catch (CFileWriteException&)
	{
#ifdef _MSC_VER		
		MessageBox(NULL, L"Result Write Error", L"ResultWriter", MB_OK);
#endif		
	}


	return 0;
}


// запись результатов в потоке
bool CResultFileWriter::WriteResultsThreaded()
{
	bool bRes = false;

	try
	{
		CChannelEncoder* pEncoder{ pEncoders_.get() };
		const CChannelEncoder * const pEncoderEnd{ pEncoder + ChannelsCount_ - 2 };
		// записываем время и шаг
		WriteTime(TimeToWrite_, StepToWrite_);
		// записываем текущий блок с помощью кодеков каналов
		pEncoder = pEncoders_.get();
		while (pEncoder < pEncoderEnd)
		{
			// WriteChannel сам решает - продолжать писать в буфер или сбрасывать на диск, если 
			// буфер закончился
			WriteChannel(pEncoder - pEncoders_.get(), pEncoder->Value_);
			pEncoder++;
		}

		if (PredictorOrder_ < PREDICTOR_ORDER)
		{
			// если не набрали заданный порядок предиктора
			// добавляем текущее время и увеличиваем порядок
			ts[PredictorOrder_] = SetTime_;
			PredictorOrder_++;
		}
		else
		{
			// если предиктор работает с заданным порядком,
			// сдвигаем буфер точек времени на 1 влево и добавляем текущее время
			std::copy(ts + 1, ts + PREDICTOR_ORDER, ts);
			//memcpy(ts, ts + 1, sizeof(double) * (PREDICTOR_ORDER - 1));
			ts[PREDICTOR_ORDER - 1] = SetTime_;
		}

		bRes = true;
	}
	catch(...)
	{
		bRes = false;
	}

	return bRes;
}

void CResultFileWriter::SetChannel(ptrdiff_t DeviceId, ptrdiff_t DeviceType, ptrdiff_t DeviceVarIndex, const double* pVariable, ptrdiff_t VariableIndex)
{
	if (VariableIndex >= 0 && VariableIndex < static_cast<ptrdiff_t>(ChannelsCount_ - 2) && pVariable)
	{
		CChannelEncoder& Encoder{ pEncoders_[VariableIndex] };
		Encoder.DeviceId_		= DeviceId;
		Encoder.DeviceType_		= DeviceType;
		Encoder.pVariable_		= pVariable;
		Encoder.VariableIndex_	= DeviceVarIndex;
	}
	else
		throw CFileWriteException(infile);
}

// возвращает смещение относительно текущей позиции в файле
// для относительного смещения нужно значительно меньше разрядов
int64_t CResultFileWriter::OffsetFromCurrent(int64_t AbsoluteOffset)
{
	if (AbsoluteOffset)
	{
		int64_t nCurrentOffset = infile.tellg();

		if (AbsoluteOffset >= nCurrentOffset)
			throw CFileWriteException(infile);

		AbsoluteOffset = nCurrentOffset - AbsoluteOffset;
	}
	return AbsoluteOffset;
}

// выполняет кодирование заданного буфера с помошью RLE
bool CResultFileWriter::EncodeRLE(unsigned char* pBuffer, size_t BufferSize, unsigned char* pCompressedBuffer, size_t& CompressedSize, bool& bAllBytesEqual)
{
	bool bRes{ RLECompressor_.Compress(pBuffer, BufferSize, pCompressedBuffer, CompressedSize, bAllBytesEqual) };
	if (bRes)
	{
		// если удалось сжать, или все байты одинаковые и размер сжатого равен размеру исходного - возвращаем 
		// тип блока - RLE
		if (CompressedSize < BufferSize || (bAllBytesEqual && CompressedSize <= BufferSize) )
		{
			// debug RLE
			/* 
			unsigned char *pDecoBuf = new unsigned char[2000];
			size_t nDecoSize(2000);
			m_RLECompressor.Decompress(pCompressedBuffer, nCompressedSize, pDecoBuf, nDecoSize);
			_ASSERTE(nBufferSize == nDecoSize);
			_ASSERTE(!memcmp(pBuffer, pDecoBuf, nDecoSize));
			*/
			return true; // блок RLE
		}
		return false; // блок не RLE
	}
	return bRes;
}

void CResultFileWriter::FinishWriteHeader()
{
	// в начало заголовка записываем время создания файла результатов
	double dCurrentDate = 0;
#ifdef _MSC_VER	
	SYSTEMTIME Now;
	GetLocalTime(&Now);
	if (!SystemTimeToVariantTime(&Now, &dCurrentDate))
		throw CFileWriteException();
#else
	// задаем дату 01.01.2000
	std::tm tm
	{
		0,  // seconds after the minute - [0, 60] including leap second
		0,  // minutes after the hour - [0, 59]
		0,  // hours since midnight - [0, 23]
		1,  // day of the month - [1, 31]
		0,  // months since January - [0, 11]
	    100,  // years since 1900 
		0,  // days since Sunday - [0, 6]
		0,  // days since January 1 - [0, 365]
	    0   // daylight savings time flag
	};
	// рассчитываем разницу текущего времени и 01.01.2000 в секундах и 
	// добавляем секунды от 30.12.1899  + 14  дней (1 день текущий + 13 дней Грегорианский -> Юлианский)
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now() - 
		std::chrono::system_clock::from_time_t(std::mktime(&tm))) +
		std::chrono::seconds(3155846400);
	// считаем разность в днях
	auto days = std::chrono::duration_cast<std::chrono::duration<int,std::ratio<86400>>>(seconds);
	dCurrentDate = days.count();
	// считаем долю оставшихся после вычитания дней секунд от числа секунд в сутках
	seconds -= std::chrono::duration_cast<std::chrono::seconds>(days);
	dCurrentDate += seconds.count() / 86400.0;
#endif

	WriteDouble(dCurrentDate);			// записываем время
	WriteString(GetComment());			// записываем строку комментария
	AddDirectoryEntries(3);				// описатели разделов
	WriteLEB(VarNameMap_.size());		// записываем количество единиц измерения

	// записываем последовательность типов и названий единиц измерения переменных
	for (auto&& vnmit : VarNameMap_)
	{
		WriteLEB(vnmit.first);
		WriteString(vnmit.second);
	}
	// записываем количество типов устройств
	WriteLEB(DevTypeSet_.size());

	long nChannelsCount = 0;

	// записываем устройства
	for (auto& di : DevTypeSet_)
	{
		// для каждого типа устройств
		WriteLEB(di->eDeviceType);							// идентификатор типа устройства
		WriteString(di->DevTypeName_);						// название типа устройства
		WriteLEB(di->DeviceIdsCount);						// количество идентификаторов устройства (90% - 1, для ветвей, например - 3)
		WriteLEB(di->DeviceParentIdsCount);					// количество родительских устройств
		WriteLEB(di->DevicesCount);							// количество устройств данного типа
		WriteLEB(di->VarTypes_.size());						// количество переменных устройства

		long nChannelsByDevice = 0;

		// записываем описания переменных типа устройства
		for (auto& vi : di->VarTypesList_)
		{
			WriteString(vi.Name);							// имя переменной
			WriteLEB(vi.eUnits);							// единицы измерения переменной
			unsigned char BitFlags = 0x0;
			// если у переменной есть множитель -
			// добавляем битовый флаг
			if (!Consts::Equal(vi.Multiplier, 1.0))
				BitFlags |= 0x1;
			WriteLEB(BitFlags);								// битовые флаги переменной
			// если есть множитель
			if (BitFlags & 0x1)
				WriteDouble(vi.Multiplier);					// множитель переменной

			// считаем количество каналов для данного типа устройства
			nChannelsByDevice++;
		}

		// записываем описания устройств
		for (DeviceInstanceInfo* pDev = di->pDeviceInstances_.get(); pDev < di->pDeviceInstances_.get() + di->DevicesCount; pDev++)
		{
			// для каждого устройства
			// записываем последовательность идентификаторов
			for (int i = 0; i < di->DeviceIdsCount; i++)
				WriteLEB(pDev->GetId(i));
			// записываем имя устройства
			WriteString(pDev->Name);
			// для каждого из родительских устройств
			for (int i = 0; i < di->DeviceParentIdsCount; i++)
			{
				const DeviceLinkToParent* pDevLink = pDev->GetParent(i);
				// записываем тип родительского устройства
				WriteLEB(pDevLink->eParentType);
				// и его идентификатор
				WriteLEB(pDevLink->Id);
			}
			// считаем общее количество каналов
			nChannelsCount += nChannelsByDevice;
		}
	}
	// готовим компрессоры для рассчитанного количества каналов
	PrepareChannelCompressor(nChannelsCount);
}


void CResultFileWriter::AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName)
{
	if (!VarNameMap_.insert(std::make_pair(nUnitType, UnitName)).second)
		throw dfw2error(fmt::format(CDFW2Messages::m_cszDuplicatedVariableUnit, nUnitType));
}

DeviceTypeInfo* CResultFileWriter::AddDeviceType(ptrdiff_t nTypeId, std::string_view TypeName)
{
	auto DeviceType = std::make_unique<DeviceTypeInfo>();
	DeviceType->eDeviceType = static_cast<int>(nTypeId);
	DeviceType->DevTypeName_= TypeName;
	DeviceType->DeviceParentIdsCount = DeviceType->DeviceIdsCount = 1;
	DeviceType->VariablesByDeviceCount = DeviceType->DevicesCount = 0;
	if (!DevTypeSet_.insert(DeviceType.get()).second)
		throw dfw2error(fmt::format(CDFW2Messages::m_cszDuplicatedDeviceType, nTypeId));
	return DeviceType.release();
}


void CResultFileWriter::AddSlowVariable(ptrdiff_t nDeviceType,
	const ResultIds& DeviceIds,
	const std::string_view VariableName,
	double Time,
	double Value,
	double PreviousValue,
	const std::string_view ChangeDescription)
{

	GetSlowVariables().Add(nDeviceType,
		DeviceIds,
		VariableName,
		Time,
		Value,
		PreviousValue,
		ChangeDescription);
}


#ifdef __GNUC__
#pragma GCC push options
#pragma GCC optimize ("O0")
#endif

#ifdef _MSC_VER
#pragma optimize("", off)
#endif

void CResultFileWriter::UpdateLagrangeCoefficients(double Time)
{
	if (PredictorOrder_ >= PREDICTOR_ORDER)
	{
		for (int j = 0; j < PREDICTOR_ORDER; j++)
		{
			ls[j] = 1;
			for (int m = 0; m < PREDICTOR_ORDER; m++)
			{
				if (m != j)
					ls[j] *= (Time - ts[m]) / (ts[j] - ts[m]);
			}
			_CheckNumber(ls[j]);
		}
	}
}

#ifdef __GNUC__
#pragma GCC pop options
#endif

#ifdef _MSC_VER
#pragma optimize("", on)
#endif
