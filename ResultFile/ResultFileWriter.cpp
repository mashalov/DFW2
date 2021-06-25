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


void CResultFileWriter::WriteTime(double dTime, double dStep)
{
	m_dSetTime = dTime; 
	m_dSetStep = dStep;
	m_bPredictorReset = false;

	if (m_nPredictorOrder > 0)
	{
		bool bReset = false;
		for (int j = 0; j < m_nPredictorOrder; j++)
		{
			if (Equal(dTime, ts[j]))
			{
				bReset = true;
				break;
			}
		}

		if (bReset)
		{
			CChannelEncoder *pEncoder = m_pEncoders.get();
			CChannelEncoder *pEncoderEnd = pEncoder + m_nChannelsCount - 2;

			while (pEncoder < pEncoderEnd)
			{
				pEncoder->m_Compressor.ResetPredictor(m_nPredictorOrder);
				pEncoder++;
			}

			// если новое время равно текущему, предиктор нужно сбросить
			// порядок предиктора обнуляем, но используем флаг m_bPredictorReset
			// который нужен для того, чтобы использовать для предиктора последнее
			// значение, а не ноль (см. Predict)

			m_nPredictorOrder = 0;
			m_bPredictorReset = true;
		}
	}

	if (m_nPredictorOrder >= PREDICTOR_ORDER)
	{
		for (int j = 0; j < PREDICTOR_ORDER; j++)
		{
			ls[j] = 1;
			for (int m = 0; m < PREDICTOR_ORDER; m++)
			{
				if (m != j)
					ls[j] *= (dTime - ts[m]) / (ts[j] - ts[m]);
			}
			_CheckNumber(ls[j]);
		}
	}

	WriteChannel(m_nChannelsCount - 2, dTime);
	WriteChannel(m_nChannelsCount - 1, dStep);
	m_nPointsCount++;
}

void CResultFileWriter::FlushChannels()
{ 
	// если каналы уже были сброшены - выходим
	if (m_bChannelsFlushed)
		return;
	TerminateWriterThread();

	for (size_t nChannel = 0; nChannel < m_nChannelsCount; nChannel++)
	{
		FlushChannel(nChannel);
		// сбрасываем SuperRLE каналы, в которых на всех точках были одинаковые значения
		FlushSuperRLE(m_pEncoders[nChannel]);
	}

	struct DataDirectoryEntry de = { 0, 0 };

	de.m_Offset = infile.tellg();
	infile.seekg(m_DataDirectoryOffset, std::ios_base::beg);
	infile.write(&de, sizeof(struct DataDirectoryEntry));
	infile.seekg(de.m_Offset, std::ios_base::beg);
	WriteLEB(m_nPointsCount);
	WriteLEB(m_nChannelsCount);

	CChannelEncoder *pEncoder = m_pEncoders.get();
	CChannelEncoder *pEncoderEnd = pEncoder + m_nChannelsCount - 2;

	while (pEncoder < pEncoderEnd)
	{
		WriteChannelHeader(pEncoder - m_pEncoders.get(), 
						   pEncoder->m_nDeviceType, 
						   pEncoder->m_nDeviceId, 
						   pEncoder->m_nVariableIndex);
		pEncoder++;
	}

	WriteChannelHeader(m_nChannelsCount - 2, DEVTYPE_MODEL, 0, 0);
	WriteChannelHeader(m_nChannelsCount - 1, DEVTYPE_MODEL, 0, 1);

	
	// write slow variables
	de.m_DataType = 1;
	de.m_Offset = infile.tellg();
	infile.seekg(m_DataDirectoryOffset + sizeof(struct DataDirectoryEntry), std::ios_base::beg);
	infile.write(&de, sizeof(struct DataDirectoryEntry));
	infile.seekg(de.m_Offset, std::ios_base::beg);

	WriteLEB(m_setSlowVariables.size());
	for (auto &di : m_setSlowVariables)
	{
		if (!di->m_Graph.empty())
		{
			WriteLEB(di->m_DeviceTypeId);
			WriteString(di->m_strVarName);
			WriteLEB(di->m_DeviceIds.size());

			for (auto &idi : di->m_DeviceIds)
				WriteLEB(idi);
			WriteLEB(di->m_Graph.size());

			for (auto &gi : di->m_Graph)
			{
				WriteDouble(gi.m_dTime);
				WriteDouble(gi.m_dValue);
				WriteString(gi.m_strDescription);
			}
		}
	}

	de.m_DataType = 2;
	de.m_Offset = infile.tellg();
	WriteString("");

	infile.seekg(m_DataDirectoryOffset + sizeof(struct DataDirectoryEntry) * 2, std::ios_base::beg);
	infile.write(&de, sizeof(struct DataDirectoryEntry));

	// ставим признак сброса каналов
	m_bChannelsFlushed = true;
}

void CResultFileWriter::FlushSuperRLE(CChannelEncoder& Encoder)
{
	if (Encoder.m_nUnwrittenSuperRLECount)
	{
		int64_t nCurrentSeek = infile.tellg();
		WriteLEB(2);
		WriteLEB(Encoder.m_nUnwrittenSuperRLECount);
		infile.write(&Encoder.m_SuperRLEByte, sizeof(unsigned char));
		WriteLEB(OffsetFromCurrent(Encoder.m_nPreviousSeek));
		Encoder.m_nPreviousSeek = nCurrentSeek;
		Encoder.m_nUnwrittenSuperRLECount = 0;
	}
}

void CResultFileWriter::FlushChannel(ptrdiff_t nIndex)
{
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_nChannelsCount))
	{
		CChannelEncoder &Encoder = m_pEncoders[nIndex];
		CCompressorParallel& FloatCompressor = Encoder.m_Compressor;
		CBitStream& Output = Encoder.m_Output;

		if (Encoder.m_nCount)
		{
			size_t nCompressedSize(m_nBufferLength * sizeof(BITWORD));
			bool bAllBytesEqual(false);
			if (nIndex < static_cast<ptrdiff_t>(m_nChannelsCount - 2) &&
				EncodeRLE(Output.BytesBuffer(), Output.BytesWritten(), m_pCompressedBuffer.get(), nCompressedSize, bAllBytesEqual))
			{	
				if (bAllBytesEqual)
				{
					// все байты во входном буфере RLE одинаковые
					if (Encoder.m_nUnwrittenSuperRLECount > 0)
					{
						// уже есть блок данных с одинаковыми байтами
						// проверяем, это были те же байты, что пришли сейчас ?
						if(Encoder.m_SuperRLEByte == *Output.BytesBuffer())
							Encoder.m_nUnwrittenSuperRLECount += Encoder.m_nCount;	// если да, просто увеличиваем счетчик байтов
						else
						{
							// накопленные байты отличаются от тех, что пришли,
							// поэтому сбрасываем старый RLE блок и начинаем записывать новый
							FlushSuperRLE(Encoder);
							Encoder.m_nPreviousSeek = infile.tellg();
							Encoder.m_nUnwrittenSuperRLECount = Encoder.m_nCount;
							Encoder.m_SuperRLEByte = *Output.BytesBuffer();
						}
					}
					else
					{
						// блока с одинаковыми данными не было, начинаем его записывать
						Encoder.m_nUnwrittenSuperRLECount = Encoder.m_nCount;
						Encoder.m_SuperRLEByte = *Output.BytesBuffer();
					}
				}
				else
				{
					// сбрасываем блок SuperRLE если был
					FlushSuperRLE(Encoder);
					int64_t nCurrentSeek = infile.tellg();
					WriteLEB(0);						// type of block 0 - RLE data
					WriteLEB(Encoder.m_nCount);			// count of doubles
					WriteLEB(nCompressedSize);			// byte length of RLE data
					infile.write(m_pCompressedBuffer.get(), nCompressedSize);
					WriteLEB(OffsetFromCurrent(Encoder.m_nPreviousSeek));
					Encoder.m_nPreviousSeek = nCurrentSeek;

				}
			}
			else
			{
				// сбрасываем блок SuperRLE если был
				FlushSuperRLE(Encoder);
				int64_t nCurrentSeek = infile.tellg();
				WriteLEB(1);						// type of block 1 - RAW compressed data
				WriteLEB(Encoder.m_nCount);			// count of doubles
				WriteLEB(Output.BytesWritten());	// byte length of block
				infile.write(Output.Buffer(), Output.BytesWritten());
				WriteLEB(OffsetFromCurrent(Encoder.m_nPreviousSeek));
				Encoder.m_nPreviousSeek = nCurrentSeek;
			}
			Encoder.m_nCount = 0;
			Output.Reset();
		}
	}
	else
		throw CFileWriteException(infile);
}

void CResultFileWriter::WriteChannel(ptrdiff_t nIndex, double dValue)
{
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_nChannelsCount))
	{
		CChannelEncoder &Encoder = m_pEncoders[nIndex];
		CCompressorParallel& FloatCompressor = Encoder.m_Compressor;
		CBitStream& Output = Encoder.m_Output;

		eFCResult Result = FC_OK;

		///*
		if (static_cast<ptrdiff_t>(m_nChannelsCount - 2) <= nIndex)
		{
			
			Result = Output.WriteDouble(dValue);
			if (Result == FC_BUFFEROVERFLOW)
			{
				FlushChannel(nIndex);
				Result = Output.WriteDouble(dValue);
				if (Result == FC_ERROR)
					throw CFileWriteException(infile);
				else
					if (Result == FC_OK)
						Encoder.m_nCount++;
			}
			else
				if (Result == FC_ERROR)
					throw CFileWriteException(infile);
				else
					if (Result == FC_OK)
						Encoder.m_nCount++;

		}
		else
		//*/
		{

			double pred = 0.0;
			/*
			if (static_cast<ptrdiff_t>(m_nChannelsCount - 2) <= nIndex)
			{
				pred = FloatCompressor.Predict(m_dSetTime, false, 1, ls);
				FloatCompressor.UpdatePredictor(dValue, 0, 0);
				Result = FloatCompressor.WriteDouble(dValue, pred, Output);
			}
			else
			*/
			{
				pred = FloatCompressor.Predict(m_dSetTime, m_bPredictorReset, m_nPredictorOrder, ls);
				FloatCompressor.UpdatePredictor(dValue, m_nPredictorOrder, m_dNoChangeTolerance);
				Result = FloatCompressor.WriteDouble(dValue, pred, Output);
			}

			if (Result == FC_BUFFEROVERFLOW)
			{
				FlushChannel(nIndex);
				Result = FloatCompressor.WriteDouble(dValue, pred, Output);
				if (Result == FC_ERROR)
					throw CFileWriteException(infile);
				else
					if (Result == FC_OK)
						Encoder.m_nCount++;
			}
			else
				if (Result == FC_ERROR)
					throw CFileWriteException(infile);
				else
					if (Result == FC_OK)
						Encoder.m_nCount++;
		}
	}
	else
		throw CFileWriteException(infile);
}

void CResultFileWriter::WriteChannelHeader(ptrdiff_t nIndex, ptrdiff_t Type, ptrdiff_t nId, ptrdiff_t nVarIndex)
{
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_nChannelsCount))
	{
		WriteLEB(Type);
		WriteLEB(nId);
		WriteLEB(nVarIndex);
		WriteLEB(OffsetFromCurrent(m_pEncoders[nIndex].m_nPreviousSeek));
	}
	else
		throw CFileWriteException(infile);
}

void CResultFileWriter::PrepareChannelCompressor(size_t nChannelsCount)
{
	m_pEncoders = std::make_unique<CChannelEncoder[]>(m_nChannelsCount = nChannelsCount + 2);
	m_nBufferLength *= sizeof(double) / sizeof(BITWORD);
	m_pCompressedBuffer = std::make_unique<unsigned char[]>(m_nBufferLength * sizeof(BITWORD));
	
	size_t nBufferGroup = m_nBufferGroup;
	size_t nSeek = 0;
	
	BITWORD *pBuffer(nullptr);

	for (size_t nChannel = 0; nChannel < m_nChannelsCount; nChannel++)
	{
		if (nBufferGroup == m_nBufferGroup)
		{
			size_t nAllocSize = m_nBufferGroup;
			if (m_nChannelsCount - nChannel < m_nBufferGroup)
				nAllocSize = m_nChannelsCount - nChannel;
			pBuffer = new BITWORD[m_nBufferLength * nAllocSize]();
			m_BufferBegin.push_back(pBuffer);
			nSeek = 0;
		}
		m_pEncoders[nChannel].m_Output.Init(pBuffer + nSeek, pBuffer + nSeek + m_nBufferLength, 0);
		m_pEncoders[nChannel].m_Output.Reset();
		nBufferGroup--;
		if (!nBufferGroup)
			nBufferGroup = m_nBufferGroup;
		nSeek += m_nBufferLength;
	}

	m_pEncoders[m_nChannelsCount - 1].m_Compressor.UpdatePredictor(0, 0);
	m_pEncoders[m_nChannelsCount - 2].m_Compressor.UpdatePredictor(0, 0);
}

void CResultFileWriter::SetNoChangeTolerance(double dTolerance)
{
	m_dNoChangeTolerance = dTolerance;
}

CResultFileWriter::~CResultFileWriter()
{
	CloseFile();
	//printf("\n%zd", m_nPointsCount);
}

// завершает и закрывает поток записи
void CResultFileWriter::TerminateWriterThread()
{
	// если поток записи еще работает
	if (threadWriter.joinable())
	{
		{
			// берем мьютекс доступа к данным
			std::unique_lock<std::mutex> dataGuard(mutexData);
			// убираем флаг работы потока
			m_bThreadRun = false;
			// снимаем поток с ожидания
			conditionRun.notify_all();
		}
		// ждем завершения функции потока
		threadWriter.join();
	}
}

// закрывает файл записываемых результатов
void CResultFileWriter::CloseFile()
{
	// сбрасываем каналы, если еще не были сброшены
	FlushChannels();
	// сначала останавливаем поток записи
	TerminateWriterThread();

	infile.close();

	for (auto&& it : m_BufferBegin)
		delete it;

	m_BufferBegin.clear();
}

// создает файл результатов
void CResultFileWriter::CreateResultFile(std::string_view FilePath)
{
	infile.open(FilePath, std::ios_base::out|std::ios_base::binary);
	// запись сигнатуры
	size_t nCountSignature = sizeof(m_cszSignature);
	infile.write(m_cszSignature, nCountSignature);
	// запись версии (версия в define, соответствует исходнику)
	WriteLEB(DFW2_RESULTFILE_VERSION);

	// создаем поток для записи
	threadWriter = std::thread(CResultFileWriter::WriterThread, this);
	if(!threadWriter.joinable())
		throw CFileWriteException(infile);

	// раз создали файл результатов - потребуется финализация
	m_bChannelsFlushed = false;
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
	m_DataDirectoryOffset = infile.tellg();						//	запоминаем позицию начала разделов в файле
	// записываем заданное количество пустых разделов
	for (size_t i = 0; i < nDirectoryEntriesCount; i++)
		infile.write(&DirEntry, sizeof(struct DataDirectoryEntry));
}

// запись результатов вне потока
void CResultFileWriter::WriteResults(double dTime, double dStep)
{
	// проверяем активен ли поток записи
	if (threadWriter.joinable())
	{
		// ждем и забираем мьютекс доступа к данным
		std::unique_lock<std::mutex> dataGuard(mutexData);

		try
		{
			// сохраняем результаты из указателей во внутрениий буфер
			CChannelEncoder *pEncoder = m_pEncoders.get();
			CChannelEncoder *pEncoderEnd = pEncoder + m_nChannelsCount - 2;
			while (pEncoder < pEncoderEnd)
			{
				pEncoder->m_dValue = *pEncoder->m_pVariable;
				pEncoder++;
			}

			m_dTimeToWrite = dTime;
			m_dStepToWrite = dStep;
		}
		catch (...)
		{
			throw CFileWriteException(infile);
		}

		// по завершению копирования результатов запускаем поток записи
		conditionRun.notify_all();
		// после освобождения мьютекса поток будет снят с wait
	}
	else
		throw CFileWriteException(infile);
}

// запись результатов в потоке
bool CResultFileWriter::WriteResultsThreaded()
{
	bool bRes = false;

	try
	{
		CChannelEncoder *pEncoder = m_pEncoders.get();
		CChannelEncoder *pEncoderEnd = pEncoder + m_nChannelsCount - 2;
		// записываем время и шаг
		WriteTime(m_dTimeToWrite, m_dStepToWrite);
		// записываем текущий блок с помощью кодеков каналов
		pEncoder = m_pEncoders.get();
		while (pEncoder < pEncoderEnd)
		{
			// WriteChannel сам решает - продолжать писать в буфер или сбрасывать на диск, если 
			// буфер закончился
			WriteChannel(pEncoder - m_pEncoders.get(), pEncoder->m_dValue);
			pEncoder++;
		}

		if (m_nPredictorOrder < PREDICTOR_ORDER)
		{
			// если не набрали заданный порядок предиктора
			// добавляем текущее время и увеличиваем порядок
			ts[m_nPredictorOrder] = m_dSetTime;
			m_nPredictorOrder++;
		}
		else
		{
			// если предиктор работает с заданным порядком,
			// сдвигаем буфер точек времени на 1 влево и добавляем текущее время
			std::copy(ts + 1, ts + PREDICTOR_ORDER, ts);
			//memcpy(ts, ts + 1, sizeof(double) * (PREDICTOR_ORDER - 1));
			ts[PREDICTOR_ORDER - 1] = m_dSetTime;
		}

		bRes = true;
	}
	catch(...)
	{
		bRes = false;
	}

	return bRes;
}

void CResultFileWriter::SetChannel(ptrdiff_t nDeviceId, ptrdiff_t nDeviceType, ptrdiff_t nDeviceVarIndex, const double *pVariable, ptrdiff_t nVariableIndex)
{
	if (nVariableIndex >= 0 && nVariableIndex < static_cast<ptrdiff_t>(m_nChannelsCount - 2) && pVariable)
	{
		CChannelEncoder &Encoder = m_pEncoders[nVariableIndex];
		Encoder.m_nDeviceId   = nDeviceId;
		Encoder.m_nDeviceType = nDeviceType;
		Encoder.m_pVariable = pVariable;
		Encoder.m_nVariableIndex = nDeviceVarIndex;
	}
	else
		throw CFileWriteException(infile);
}

// поток записи результатов
unsigned int CResultFileWriter::WriterThread(void *pThis)
{
	try
	{
		// получаем объект из параметра функции потока
		CResultFileWriter *pthis = static_cast<CResultFileWriter*>(pThis);

		if (!pthis)
			throw CFileWriteException(nullptr);

		// поток работает пока не сброшен флаг работы в параметрах
		while (pthis->m_bThreadRun)
		{
			// ждем команды на запуск записи или останов
			{
				std::unique_lock<std::mutex> dataGuard(pthis->mutexData);
				pthis->conditionRun.wait(dataGuard);

				if (!pthis->m_bThreadRun)
					break;

				// записываем очередной блок результатов
				if (!pthis->WriteResultsThreaded())
					throw CFileWriteException(pthis->infile);
			}
		}
	}
	catch (CFileWriteException&)
	{
		MessageBox(NULL, L"Result Write Error", L"ResultWriter", MB_OK);
	}

	return 0;
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
bool CResultFileWriter::EncodeRLE(unsigned char* pBuffer, size_t nBufferSize, unsigned char* pCompressedBuffer, size_t& nCompressedSize, bool& bAllBytesEqual)
{
	bool bRes = m_RLECompressor.Compress(pBuffer, nBufferSize, pCompressedBuffer, nCompressedSize, bAllBytesEqual);
	if (bRes)
	{
		// если удалось сжать, или все байты одинаковые и размер сжатого равен размеру исходного - возвращаем 
		// тип блока - RLE
		if (nCompressedSize < nBufferSize || (bAllBytesEqual && nCompressedSize <= nBufferSize) )
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

