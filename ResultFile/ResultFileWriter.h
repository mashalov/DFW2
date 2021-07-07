#pragma once
#include "ResultFileReader.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "IResultWriterABI.h"

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
		ptrdiff_t m_nVariableIndex = -1;					// индекс переменной канала
		const double *m_pVariable = nullptr;				// адрес переменной канала
		double m_dValue;									// значение для буферизации переданного на m_pVariable значения и записи в потоке
		ptrdiff_t m_nDeviceId;								// идентификатор устройства канала
		ptrdiff_t m_nDeviceType;							// тип устройства канала
		unsigned char m_SuperRLEByte;						// байт SuperRLE
	};

	using BUFFERBEGIN = std::vector<BITWORD*>;
	using BUFFERBEGINITERATOR = BUFFERBEGIN::iterator;

	class CResultFileWriter : public CResultFile, public IResultWriterABI
	{
	protected:
		std::unique_ptr<CChannelEncoder[]> m_pEncoders;
		size_t m_nChannelsCount = 0;
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
		std::mutex mutexRun;
		std::mutex mutexDone;
		std::atomic<int> portionSent, portionReceived;
		std::atomic_bool m_bThreadRun = true;
		std::condition_variable conditionDone;
		std::condition_variable conditionRun;

		void TerminateWriterThread();
		double m_dTimeToWrite;
		double m_dStepToWrite;
		bool WriteResultsThreaded();
		int64_t OffsetFromCurrent(int64_t AbsoluteOffset);
		double m_dNoChangeTolerance = 0.0;
		double ts[PREDICTOR_ORDER] = {};
		double ls[PREDICTOR_ORDER] = {};
		ptrdiff_t m_nPredictorOrder = 0;
		CSlowVariablesSet m_setSlowVariables;
		CRLECompressor	m_RLECompressor;
		std::unique_ptr<unsigned char[]> m_pCompressedBuffer;
		bool EncodeRLE(unsigned char* pBuffer, size_t nBufferSize, unsigned char* pCompressedBuffer, size_t& nCompressedSize, bool& bAllBytesEqual);
		void FlushSuperRLE(CChannelEncoder& Encoder);
		bool m_bChannelsFlushed = true;
		std::string m_strComment;
		VARNAMEMAP m_VarNameMap;
		DEVTYPESET m_DevTypeSet;
	public:
		virtual ~CResultFileWriter();

		void WriteDouble(const double &Value);
		void WriteTime(double dTime, double dStep);
		void WriteChannel(ptrdiff_t nIndex, double dValue);
		void FlushChannel(ptrdiff_t nIndex); 
		void FlushChannels();
		void PrepareChannelCompressor(size_t nChannelsCount);
		void WriteChannelHeader(ptrdiff_t nIndex, ptrdiff_t eType, ptrdiff_t nId, ptrdiff_t nVarIndex);
		void AddDirectoryEntries(size_t nDirectoryEntriesCount);
		static unsigned int WriterThread(void *pThis);
		CSlowVariablesSet& GetSlowVariables() { return m_setSlowVariables; }
		const std::string& GetComment() { return m_strComment; }

		// ABI Interface
		void Destroy() override { delete this; }
		void Close() override;
		void FinishWriteHeader() override;
		void CreateResultFile(const std::filesystem::path FilePath) override;
		void SetNoChangeTolerance(double Tolerance) override;
		void SetComment(const std::string_view Comment) override { m_strComment = Comment; }
		void WriteResults(double Time, double Step) override;
		void AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName) override;
		DeviceTypeInfo* AddDeviceType(ptrdiff_t nTypeId, std::string_view TypeName) override;
		void SetChannel(ptrdiff_t nDeviceId, ptrdiff_t nDeviceType, ptrdiff_t nDeviceVarIndex, const double* pVariable, ptrdiff_t nVariableIndex) override;
		virtual void AddSlowVariable(ptrdiff_t nDeviceType,
			const ResultIds& DeviceIds,
			const std::string_view VariableName,
			double Time,
			double Value,
			double PreviousValue,
			const std::string_view ChangeDescription) override;
	};
}
