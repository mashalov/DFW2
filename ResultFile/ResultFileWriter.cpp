#include "stdafx.h"
#include "ResultFileWriter.h"
#include "memory.h"

using namespace DFW2;

void CResultFile::WriteLEB(unsigned __int64 Value)
{
	if (m_pFile)
	{
		do
		{
			unsigned char low = Value & 0x7f;
			Value >>= 7;
			if (Value != 0)
				low |= 0x80;

			if (fwrite(&low, sizeof(unsigned char), 1, m_pFile) != 1)
				throw CFileWriteException(m_pFile);
		} while (Value != 0);
	}
	else
		throw CFileWriteException(m_pFile);
}

void CResultFile::WriteString(const _TCHAR *cszString)
{
	if (cszString)
	{
		size_t nLen = _tcslen(cszString);
		WriteLEB(nLen);
		CUnicodeSCSU StringWriter(m_pFile);
		StringWriter.WriteSCSU(cszString);
	}
	else
		WriteLEB(0);
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
			CChannelEncoder *pEncoder = m_pEncoders;
			CChannelEncoder *pEncoderEnd = m_pEncoders + m_nChannelsCount - 2;

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
	TerminateWriterThread();

	for (size_t nChannel = 0; nChannel < m_nChannelsCount; nChannel++)
		FlushChannel(nChannel);

	struct DataDirectoryEntry de = { 0, 0 };

	de.m_Offset = _ftelli64(m_pFile);
	if(_fseeki64(m_pFile, m_DataDirectoryOffset, SEEK_SET))
		throw CFileWriteException(m_pFile);
	if (fwrite(&de, sizeof(struct DataDirectoryEntry), 1, m_pFile) != 1)
		throw CFileWriteException(m_pFile);
	if (_fseeki64(m_pFile, de.m_Offset, SEEK_SET))
		throw CFileWriteException(m_pFile);
	WriteLEB(m_nPointsCount);
	WriteLEB(m_nChannelsCount);

	CChannelEncoder *pEncoder = m_pEncoders;
	CChannelEncoder *pEncoderEnd = m_pEncoders + m_nChannelsCount - 2;

	while (pEncoder < pEncoderEnd)
	{
		WriteChannelHeader(pEncoder - m_pEncoders, 
						   pEncoder->m_nDeviceType, 
						   pEncoder->m_nDeviceId, 
						   pEncoder->m_nVariableIndex);
		pEncoder++;
	}

	WriteChannelHeader(m_nChannelsCount - 2, DEVTYPE_MODEL, 0, 0);
	WriteChannelHeader(m_nChannelsCount - 1, DEVTYPE_MODEL, 0, 1);

	
	// write slow variables
	de.m_DataType = 1;
	de.m_Offset = _ftelli64(m_pFile);
	if (_fseeki64(m_pFile, m_DataDirectoryOffset + sizeof(struct DataDirectoryEntry), SEEK_SET))
		throw CFileWriteException(m_pFile);

	if (fwrite(&de, sizeof(struct DataDirectoryEntry), 1, m_pFile) != 1)
		throw CFileWriteException(m_pFile);
	if (_fseeki64(m_pFile, de.m_Offset, SEEK_SET))
		throw CFileWriteException(m_pFile);

	WriteLEB(m_setSlowVariables.size());
	for (auto &di : m_setSlowVariables)
	{
		if (!di->m_Graph.empty())
		{
			WriteLEB(di->m_DeviceTypeId);
			WriteString(di->m_strVarName.c_str());
			WriteLEB(di->m_DeviceIds.size());

			for (auto &idi : di->m_DeviceIds)
				WriteLEB(idi);
			WriteLEB(di->m_Graph.size());

			for (auto &gi : di->m_Graph)
			{
				WriteDouble(gi.m_dTime);
				WriteDouble(gi.m_dValue);
				WriteString(gi.m_strDescription.c_str());
			}
		}
	}

	de.m_DataType = 2;
	de.m_Offset = _ftelli64(m_pFile);
	WriteString(_T(""));

	if (_fseeki64(m_pFile, m_DataDirectoryOffset + sizeof(struct DataDirectoryEntry) * 2, SEEK_SET))
		throw CFileWriteException(m_pFile);

	if (fwrite(&de, sizeof(struct DataDirectoryEntry), 1, m_pFile) != 1)
		throw CFileWriteException(m_pFile);

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
			__int64 nCurrentSeek = _ftelli64(m_pFile);
			size_t nCompressedSize(m_nBufferLength * sizeof(BITWORD));
			if (nIndex < static_cast<ptrdiff_t>(m_nChannelsCount - 2) &&
				EncodeRLE(Output.BytesBuffer(), Output.BytesWritten(), m_pCompressedBuffer, nCompressedSize))
			{	
				WriteLEB(0);						// type of block 0 - RLE data
				WriteLEB(Encoder.m_nCount);			// count of doubles
				WriteLEB(nCompressedSize);			// byte length of RLE data

				if (fwrite(m_pCompressedBuffer, sizeof(unsigned char), nCompressedSize, m_pFile) != nCompressedSize)
					throw CFileWriteException(m_pFile);
			}
			else
			{
				WriteLEB(1);						// type of block 1 - RAW compressed data
				WriteLEB(Encoder.m_nCount);			// count of doubles
				WriteLEB(Output.BytesWritten());	// byte length of block

				if (fwrite(Output.Buffer(), sizeof(unsigned char), Output.BytesWritten(), m_pFile) != Output.BytesWritten())
					throw CFileWriteException(m_pFile);
			}
			Encoder.m_nCount = 0;
			Output.Reset();
			WriteLEB(OffsetFromCurrent(Encoder.m_nPreviousSeek));
			Encoder.m_nPreviousSeek = nCurrentSeek;
		}
	}
	else
		throw CFileWriteException(m_pFile);
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
					throw CFileWriteException(m_pFile);
				else
					if (Result == FC_OK)
						Encoder.m_nCount++;
			}
			else
				if (Result == FC_ERROR)
					throw CFileWriteException(m_pFile);
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
					throw CFileWriteException(m_pFile);
				else
					if (Result == FC_OK)
						Encoder.m_nCount++;
			}
			else
				if (Result == FC_ERROR)
					throw CFileWriteException(m_pFile);
				else
					if (Result == FC_OK)
						Encoder.m_nCount++;
		}
	}
	else
		throw CFileWriteException(m_pFile);
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
		throw CFileWriteException(m_pFile);
}

void CResultFileWriter::PrepareChannelCompressor(size_t nChannelsCount)
{
	m_pEncoders = new CChannelEncoder[m_nChannelsCount = nChannelsCount + 2];
	m_nBufferLength *= sizeof(double) / sizeof(BITWORD);
	m_pCompressedBuffer = new unsigned char[m_nBufferLength * sizeof(BITWORD)];
	
	size_t nBufferGroup = m_nBufferGroup;
	size_t nSeek = 0;
	
	BITWORD *pBuffer = NULL;

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


CResultFileWriter::CResultFileWriter()
{
	m_pFile = NULL;
	m_pEncoders = NULL;
	m_nBufferLength = 100;
	m_nBufferGroup = 100;
	m_nPointsCount = 0;
	m_hThread = NULL;
	m_hRunEvent = NULL;
	m_hDataMutex = NULL;
	m_bThreadRun = true;
	m_dNoChangeTolerance = 0.0;
	m_nPredictorOrder = 0;
	m_pCompressedBuffer = NULL;
}

void CResultFileWriter::SetNoChangeTolerance(double dTolerance)
{
	m_dNoChangeTolerance = dTolerance;
}


CResultFileWriter::~CResultFileWriter()
{
	CloseFile();
}

void CResultFileWriter::TerminateWriterThread()
{
	if (m_hThread)
	{
		DWORD dwExitCode = 0;
		if (GetExitCodeThread(m_hThread, &dwExitCode))
		{
			if (dwExitCode == STILL_ACTIVE)
			{
				m_bThreadRun = false;
				if (m_hRunEvent)
				{
					SetEvent(m_hRunEvent);
					WaitForSingleObject(m_hThread, INFINITE);
					CloseHandle(m_hThread);
					m_hThread = NULL;
				}
			}
		}
	}
}

void CResultFileWriter::CloseFile()
{
	TerminateWriterThread();

	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}

	if (m_pEncoders)
	{
		delete[] m_pEncoders;
		m_pEncoders = NULL;
	}

	if (m_pCompressedBuffer)
	{
		delete m_pCompressedBuffer;
		m_pCompressedBuffer = NULL;
	}

	for (BUFFERBEGINITERATOR it = m_BufferBegin.begin(); it != m_BufferBegin.end(); it++)
		delete *it;

	m_BufferBegin.clear();

	if (m_hRunEvent)
	{
		CloseHandle(m_hRunEvent);
		m_hRunEvent = NULL;
	}

	if (m_hDataMutex)
	{
		CloseHandle(m_hDataMutex);
		m_hDataMutex = NULL;
	}

}

void CResultFileWriter::CreateResultFile(const _TCHAR *cszFilePath)
{
	if (!_tfopen_s(&m_pFile, cszFilePath, _T("wb+,ccs=UNICODE")))
	{
		size_t nCountSignature = sizeof(m_cszSignature);
		if(fwrite(m_cszSignature, sizeof(m_cszSignature[0]), nCountSignature, m_pFile) != nCountSignature)
			throw CFileWriteException(m_pFile);
		WriteLEB(DFW2_RESULTFILE_VERSION);

		m_hRunEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (m_hRunEvent == NULL)
			throw CFileWriteException(m_pFile);

		m_hDataMutex = CreateMutex(NULL, FALSE, NULL);
		if (m_hDataMutex == NULL)
			throw CFileWriteException(m_pFile);
		
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, CResultFileWriter::WriterThread, this, 0, NULL);
		if (m_hThread == NULL)
			throw CFileWriteException(m_pFile);
	}
	else
		throw CFileWriteException(NULL,cszFilePath);
}


void CResultFileWriter::WriteDouble(const double &Value)
{
	if(fwrite(&Value, sizeof(double), 1, m_pFile) != 1)
		throw CFileWriteException(m_pFile);
}

void CResultFileWriter::AddDirectoryEntries(size_t nDirectoryEntriesCount)
{
	WriteLEB(nDirectoryEntriesCount);
	struct DataDirectoryEntry DirEntry = { 0, 0LL };
	m_DataDirectoryOffset = _ftelli64(m_pFile);
	for (size_t i = 0; i < nDirectoryEntriesCount; i++)
	{
		if (fwrite(&DirEntry, sizeof(struct DataDirectoryEntry), 1, m_pFile) != 1)
			throw CFileWriteException(m_pFile);
	}
}

void CResultFileWriter::WriteResults(double dTime, double dStep)
{
	DWORD dwExitCode;
	if (GetExitCodeThread(m_hThread, &dwExitCode))
	{
		if (dwExitCode != STILL_ACTIVE)
			throw CFileWriteException(m_pFile);

		DWORD dwWaitRes = WaitForSingleObject(m_hDataMutex, INFINITE);

		if (dwWaitRes == WAIT_FAILED && dwWaitRes == WAIT_ABANDONED)
			throw CFileWriteException(m_pFile);

		if (dwWaitRes == WAIT_OBJECT_0)
		{
			__try
			{
				CChannelEncoder *pEncoder = m_pEncoders;
				CChannelEncoder *pEncoderEnd = m_pEncoders + m_nChannelsCount - 2;
				while (pEncoder < pEncoderEnd)
				{
					pEncoder->m_dValue = *pEncoder->m_pVariable;
					pEncoder++;
				}
				m_dTimeToWrite = dTime;
				m_dStepToWrite = dStep;
			}
			__finally
			{
				if (!ReleaseMutex(m_hDataMutex))
					throw CFileWriteException(m_pFile);
				if (!SetEvent(m_hRunEvent))
					throw CFileWriteException(m_pFile);
			}
		}
	}
	else
		throw CFileWriteException(m_pFile);
}

bool CResultFileWriter::WriteResultsThreaded()
{
	bool bRes = false;

	DWORD dwWaitRes = WaitForSingleObject(m_hDataMutex, INFINITE);

	if (dwWaitRes == WAIT_OBJECT_0)
	{
		__try
		{
			CChannelEncoder *pEncoder = m_pEncoders;
			CChannelEncoder *pEncoderEnd = m_pEncoders + m_nChannelsCount - 2;
			WriteTime(m_dTimeToWrite, m_dStepToWrite);
			pEncoder = m_pEncoders;
			while (pEncoder < pEncoderEnd)
			{
				WriteChannel(pEncoder - m_pEncoders, pEncoder->m_dValue);
				pEncoder++; 
			} 

			if (m_nPredictorOrder < PREDICTOR_ORDER)
			{
				ts[m_nPredictorOrder] = m_dSetTime;
				m_nPredictorOrder++;
			}
			else
			{
				memcpy(ts, ts + 1, sizeof(double) * (PREDICTOR_ORDER - 1));
				ts[PREDICTOR_ORDER - 1] = m_dSetTime;
			}

			bRes = true;
		}

		__finally
		{
			if (!ReleaseMutex(m_hDataMutex))
				bRes = false;
		}
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
		throw CFileWriteException(m_pFile);
}


unsigned int CResultFileWriter::WriterThread(void *pThis)
{
	CResultFileWriter *pthis = static_cast<CResultFileWriter*>(pThis);

	while (pthis->m_bThreadRun)
	{
		DWORD dwWaitRes = WaitForSingleObject(pthis->m_hRunEvent, INFINITE);

		if (!ResetEvent(pthis->m_hRunEvent))
			break;

		if (dwWaitRes == WAIT_OBJECT_0)
		{
			if (!pthis->m_bThreadRun)
				break;

			if (!pthis->WriteResultsThreaded())
				break;
		}
		else
			break;
	}

	return 0;
}


__int64 CResultFileWriter::OffsetFromCurrent(__int64 AbsoluteOffset)
{
	if (AbsoluteOffset)
	{
		_ASSERTE(m_pFile);
		__int64 nCurrentOffset = _ftelli64(m_pFile);

		if (AbsoluteOffset >= nCurrentOffset)
			throw CFileWriteException(m_pFile);

		AbsoluteOffset = nCurrentOffset - AbsoluteOffset;
	}
	return AbsoluteOffset;
}


bool CResultFileWriter::EncodeRLE(unsigned char* pBuffer, size_t nBufferSize, unsigned char* pCompressedBuffer, size_t& nCompressedSize)
{
	bool bRes = m_RLECompressor.Compress(pBuffer, nBufferSize, pCompressedBuffer, nCompressedSize);
	if (bRes)
		return nCompressedSize < nBufferSize;
	return bRes;
}

