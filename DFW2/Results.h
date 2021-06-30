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

namespace DFW2
{
	class CDynaModel;

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

		CResultsWriterBase(CDynaModel& Model) : model(Model) {}
		virtual void WriteResultsHeader() = 0;
		virtual void WriteResults(double Time, double Step) = 0;
		virtual void FinishWriteResults() = 0;
		virtual void CreateFile(std::filesystem::path path, ResultsInfo& Info) = 0;
	};

	class CResultsWriterCOM : public CResultsWriterBase
	{
	protected:
		IResultWritePtr m_spResultWrite;
		IResultPtr spResults;
	public:
		using CResultsWriterBase::CResultsWriterBase;
		void WriteResultsHeader() override;
		void WriteResults(double Time, double Step) override;
		void FinishWriteResults() override;
		void CreateFile(std::filesystem::path path, ResultsInfo& Info) override;
	};

}