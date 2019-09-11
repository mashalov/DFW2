#pragma once
#include "ResultFileReader.h"
#include "..\dfw2\HRTimer.h"

namespace DFW2
{
	using namespace std;

	class CChannelEncoder
	{
	public:
		CCompressorParallel m_Compressor;
		CBitStream m_Output;
		unsigned __int64 m_nPreviousSeek;
		size_t m_nCount;
		ptrdiff_t m_nVariableIndex;
		const double *m_pVariable;
		double m_dValue;
		ptrdiff_t m_nDeviceId;
		ptrdiff_t m_nDeviceType;

		CChannelEncoder() : m_nPreviousSeek(0), m_nCount(0),
							m_pVariable(NULL)
		{

		}
	};

	typedef vector<BITWORD*> BUFFERBEGIN;
	typedef BUFFERBEGIN::iterator BUFFERBEGINITERATOR;

	class CResultFileWriter : public CResultFile
	{
	protected:
		CChannelEncoder *m_pEncoders;
		size_t m_nChannelsCount;
		BUFFERBEGIN m_BufferBegin;
		double m_dSetTime;
		double m_dSetStep;
		bool m_bPredictorReset;
		size_t m_nBufferLength;
		size_t m_nBufferGroup;
		size_t m_nPointsCount;
		__int64 m_DataDirectoryOffset;
		HANDLE m_hThread;
		HANDLE m_hRunEvent;
		HANDLE m_hRunningEvent;
		HANDLE m_hDataMutex;
		void TerminateWriterThread();
		bool m_bThreadRun;
		double m_dTimeToWrite;
		double m_dStepToWrite;
		bool CResultFileWriter::WriteResultsThreaded();
		__int64 OffsetFromCurrent(__int64 AbsoluteOffset);
		double m_dNoChangeTolerance;
		double ts[PREDICTOR_ORDER];
		double ls[PREDICTOR_ORDER];
		ptrdiff_t m_nPredictorOrder;
		CSlowVariablesSet m_setSlowVariables;
		CRLECompressor	m_RLECompressor;
		unsigned char *m_pCompressedBuffer;
		bool EncodeRLE(unsigned char* pBuffer, size_t nBufferSize, unsigned char* pCompressedBuffer, size_t& nCompressedSize, bool& bAllBytesEqual);
	public:
		CResultFileWriter();
		virtual ~CResultFileWriter();
		void CreateResultFile(const _TCHAR *cszFilePath);
		void CloseFile();
		void WriteDouble(const double &Value);
		void WriteTime(double dTime, double dStep);
		void WriteChannel(ptrdiff_t nIndex, double dValue);
		void FlushChannel(ptrdiff_t nIndex); 
		void FlushChannels();
		void PrepareChannelCompressor(size_t nChannelsCount);
		void WriteChannelHeader(ptrdiff_t nIndex, ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex);
		void AddDirectoryEntries(size_t nDirectoryEntriesCount);
		void SetChannel(ptrdiff_t nDeviceId, ptrdiff_t nDeviceType, ptrdiff_t nDeviceVarIndex, const double *pVariable, ptrdiff_t nVariableIndex);
		void WriteResults(double dTime, double dStep);
		void SetNoChangeTolerance(double dTolerance);
		static unsigned int __stdcall WriterThread(void *pThis);
		CSlowVariablesSet& GetSlowVariables() { return m_setSlowVariables; }
	};
}
