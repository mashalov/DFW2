#pragma once

#ifdef _MSC_VER
#ifdef _DEBUG
#ifdef _WIN64
#include "x64\debug\resultfile.tlh"
#else
#include "debug\resultfile.tlh"
#endif
#else
#ifdef _WIN64
#include "x64\release\resultfile.tlh"
#else
#include "release\resultfile.tlh"
#endif
#endif
#endif

#include "../ResultFile/IResultWriterABI.h"
#include "DLLWrapper.h"

namespace DFW2
{
	class CDynaModel;
	class CDeviceContainer;


	class CResultsWriterBase
	{
	protected:
		CDynaModel& model;
	public:

		struct ResultsInfo
		{
			double NoChangeTolerance = 0.0;
			std::string Comment;
		};

		struct WriteResultsInfo
		{
			double Time = 0;
			double Step = 0;
		};

		class DeviceType
		{

		};

		CResultsWriterBase(CDynaModel& Model) : model(Model) {}
		virtual void WriteResults(const WriteResultsInfo& WriteInfo) = 0;
		virtual void FinishWriteResults() = 0;
		virtual void CreateFile(std::filesystem::path path, ResultsInfo& Info) = 0;
		virtual void AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName) = 0;
		virtual void AddDeviceType(const CDeviceContainer& Container) = 0;
		virtual void SetChannel(ptrdiff_t DeviceId, ptrdiff_t DeviceType, ptrdiff_t VarIndex, double* ValuePtr, ptrdiff_t ChannelIndex) = 0;
		virtual void FinishWriteHeader() = 0;
		virtual void AddSlowVariable(ptrdiff_t nDeviceType,
			const ResultIds& DeviceIds,
			const std::string_view VariableName,
			double Time,
			double Value,
			double PreviousValue,
			const std::string_view ChangeDescription) = 0;
	};

#ifdef _MSC_VER
	class CResultsWriterCOM : public CResultsWriterBase
	{
	protected:
		IResultWritePtr m_spResultWrite;
		IResultPtr spResults;
	public:
		using CResultsWriterBase::CResultsWriterBase;
		void WriteResults(const WriteResultsInfo& WriteInfo) override;
		void FinishWriteResults() override;
		void CreateFile(std::filesystem::path path, ResultsInfo& Info) override;
		void AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName) override;
		void AddDeviceType(const CDeviceContainer& Container) override;
		void SetChannel(ptrdiff_t DeviceId, ptrdiff_t DeviceType, ptrdiff_t VarIndex, double* ValuePtr, ptrdiff_t ChannelIndex) override;
		void FinishWriteHeader() override;
		virtual void AddSlowVariable(ptrdiff_t nDeviceType,
			const ResultIds& DeviceIds,
			const std::string_view VariableName,
			double Time,
			double Value,
			double PreviousValue,
			const std::string_view ChangeDescription) override;
	};
#endif

	class CResultsWriterABI : public CResultsWriterBase
	{
	protected:
		using WriterDLL = CDLLInstanceFactory<IResultWriterABI>;
		std::shared_ptr<WriterDLL> m_ABIWriterDLL;
		CDLLInstanceWrapper<WriterDLL> m_ABIWriter;
	public:
		using CResultsWriterBase::CResultsWriterBase;
		void WriteResults(const WriteResultsInfo& WriteInfo) override;
		void FinishWriteResults() override;
		void CreateFile(std::filesystem::path path, ResultsInfo& Info) override;
		void AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName) override;
		void AddDeviceType(const CDeviceContainer& Container) override;
		void SetChannel(ptrdiff_t DeviceId, ptrdiff_t DeviceType, ptrdiff_t VarIndex, double* ValuePtr, ptrdiff_t ChannelIndex) override;
		void FinishWriteHeader() override;
		virtual void AddSlowVariable(ptrdiff_t nDeviceType,
			const ResultIds& DeviceIds,
			const std::string_view VariableName,
			double Time,
			double Value,
			double PreviousValue,
			const std::string_view ChangeDescription) override;
	};

}