#pragma once
#include "ResultFileReader.h"
#include "..\dfw2\HRTimer.h"
#include <thread>
#include <mutex>
#include <condition_variable>

namespace DFW2
{
	class CChannelEncoder
	{
	public:
		CCompressorParallel m_Compressor;					// экземпляр предиктивного кодера
		CBitStream m_Output;								// поток для записи битового потока
		uint64_t m_nPreviousSeek = 0;						// смещение последнего записанного предыдущего блока
		size_t m_nCount = 0;								// количество сжатых double
		size_t m_nUnwrittenSuperRLECount = 0;				// количество подсчитанных, но не записанных блоков SuperRLE
		ptrdiff_t m_nVariableIndex;							// индекс переменной канала
		const double *m_pVariable = nullptr;				// адрес переменной канала
		double m_dValue;									// значение для буферизации переданного на m_pVariable значения и записи в потоке
		ptrdiff_t m_nDeviceId;								// идентификатор устройства канала
		ptrdiff_t m_nDeviceType;							// тип устройства канала
		unsigned char m_SuperRLEByte;						// байт SuperRLE
	};

	using BUFFERBEGIN = std::vector<BITWORD*>;
	using BUFFERBEGINITERATOR = BUFFERBEGIN::iterator;

	class CResultFileWriter : public CResultFile
	{
	protected:
		std::unique_ptr<CChannelEncoder[]> m_pEncoders;
		size_t m_nChannelsCount;
		BUFFERBEGIN m_BufferBegin;
		double m_dSetTime;
		double m_dSetStep;
		bool m_bPredictorReset;
		size_t m_nBufferLength = 100;
		size_t m_nBufferGroup = 100;
		size_t m_nPointsCount = 0;
		int64_t m_DataDirectoryOffset;

		/// std threading stuff

		std::thread threadWriter;
		std::mutex mutexData;
		std::condition_variable conditionRun;

		void TerminateWriterThread();
		bool m_bThreadRun = true;
		double m_dTimeToWrite;
		double m_dStepToWrite;
		bool WriteResultsThreaded();
		int64_t OffsetFromCurrent(int64_t AbsoluteOffset);
		double m_dNoChangeTolerance = 0.0;
		double ts[PREDICTOR_ORDER];
		double ls[PREDICTOR_ORDER];
		ptrdiff_t m_nPredictorOrder = 0;
		CSlowVariablesSet m_setSlowVariables;
		CRLECompressor	m_RLECompressor;
		std::unique_ptr<unsigned char[]> m_pCompressedBuffer;
		bool EncodeRLE(unsigned char* pBuffer, size_t nBufferSize, unsigned char* pCompressedBuffer, size_t& nCompressedSize, bool& bAllBytesEqual);
		void FlushSuperRLE(CChannelEncoder& Encoder);
		bool m_bChannelsFlushed = true;
	public:
		virtual ~CResultFileWriter();
		void CreateResultFile(std::string_view FilePath);
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
