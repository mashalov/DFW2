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
		CCompressorParallel Compressor_;					// экземпляр предиктивного кодера
		CBitStream Output_;									// поток для записи битового потока
		uint64_t PreviousSeek_ = 0;							// смещение последнего записанного предыдущего блока
		size_t Count_ = 0;									// количество сжатых double
		size_t UnwrittenSuperRLECount_ = 0;					// количество подсчитанных, но не записанных блоков SuperRLE
		ptrdiff_t VariableIndex_ = -1;						// индекс переменной канала
		const double *pVariable_ = nullptr;					// адрес переменной канала
		double Value_;										// значение для буферизации переданного на m_pVariable значения и записи в потоке
		ptrdiff_t DeviceId_;								// идентификатор устройства канала
		ptrdiff_t DeviceType_;								// тип устройства канала
		unsigned char SuperRLEByte_;						// байт SuperRLE
	};

	using BUFFERBEGIN = std::vector<BITWORD*>;

	class CResultFileWriter : public CResultFile, public IResultWriterABI
	{
	protected:
		std::unique_ptr<CChannelEncoder[]> pEncoders_;
		size_t ChannelsCount_ = 0;
		BUFFERBEGIN BufferBegin_;
		double SetTime_;
		double SetStep_;
		bool bPredictorReset_;
		size_t BufferLength_ = 100;
		size_t BufferGroup_ = 100;
		size_t PointsCount_ = 0;
		int64_t DataDirectoryOffset_;

		/// std threading stuff

		std::thread threadWriter;
		std::mutex mutexData;
		std::mutex mutexRun;
		std::mutex mutexDone;
		std::atomic<int> portionSent, portionReceived;
		std::atomic_bool bThreadRun_ = true;
		std::condition_variable conditionDone;
		std::condition_variable conditionRun;

		void TerminateWriterThread();
		double TimeToWrite_;
		double StepToWrite_;
		bool WriteResultsThreaded();
		int64_t OffsetFromCurrent(int64_t AbsoluteOffset);
		double NoChangeTolerance_ = 0.0;
		double ts[PREDICTOR_ORDER] = {};
		double ls[PREDICTOR_ORDER] = {};
		ptrdiff_t PredictorOrder_ = 0;
		CSlowVariablesSet SlowVariables_;
		CRLECompressor	RLECompressor_;
		std::unique_ptr<unsigned char[]> pCompressedBuffer_;
		bool EncodeRLE(unsigned char* pBuffer, size_t BufferSize, unsigned char* pCompressedBuffer, size_t& CompressedSize, bool& bAllBytesEqual);
		void FlushSuperRLE(CChannelEncoder& Encoder);
		bool bChannelsFlushed_ = true;
		std::string Comment_;
		VARNAMEMAP VarNameMap_;
		DEVTYPESET DevTypeSet_;
	public:
		virtual ~CResultFileWriter();

		void WriteDouble(const double &Value);
		void WriteTime(double Time, double Step);
		void UpdateLagrangeCoefficients(double Time);
		void WriteChannel(ptrdiff_t Index, double Value);
		void FlushChannel(ptrdiff_t Index); 
		void FlushChannels();
		void PrepareChannelCompressor(size_t ChannelsCount);
		void WriteChannelHeader(ptrdiff_t Index, ptrdiff_t eType, ptrdiff_t Id, ptrdiff_t VarIndex);
		void AddDirectoryEntries(size_t nDirectoryEntriesCount);
		static unsigned int WriterThread(void *pThis);
		CSlowVariablesSet& GetSlowVariables() { return SlowVariables_; }
		const std::string& GetComment() { return Comment_; }

		// ABI Interface
		void Destroy() override { delete this; }
		void Close() override;
		void FinishWriteHeader() override;
		void CreateResultFile(const std::filesystem::path FilePath) override;
		void SetNoChangeTolerance(double Tolerance) override;
		void SetComment(const std::string_view Comment) override { Comment_ = Comment; }
		void WriteResults(double Time, double Step) override;
		void AddVariableUnit(ptrdiff_t UnitType, const std::string_view UnitName) override;
		DeviceTypeInfo* AddDeviceType(ptrdiff_t TypeId, std::string_view TypeName) override;
		void SetChannel(ptrdiff_t DeviceId, ptrdiff_t DeviceType, ptrdiff_t DeviceVarIndex, const double* pVariable, ptrdiff_t VariableIndex) override;
		virtual void AddSlowVariable(ptrdiff_t DeviceType,
			const ResultIds& DeviceIds,
			const std::string_view VariableName,
			double Time,
			double Value,
			double PreviousValue,
			const std::string_view ChangeDescription) override;
	};
}
