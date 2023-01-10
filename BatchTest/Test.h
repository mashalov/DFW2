#pragma once
#include <list>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <mutex>
#include "..\DFW2\dfw2exception.h"


class CTestTimer
{
protected:
	std::chrono::time_point<std::chrono::high_resolution_clock> ClockStart_;
public:
	CTestTimer() : ClockStart_(std::chrono::high_resolution_clock::now())
	{

	}
	double Duration()
	{
		return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - ClockStart_).count()) * 1e-3;
	}
};

class CBatchTest
{
protected:
	using FilesT = std::list<std::filesystem::path>;
	FilesT CaseFiles_, ContingencyFiles_;
	struct Options
	{
		bool EmsMode = false;
		double Duration = 15;
		double RaidenAtol = 1e-4;
		double RaidenRtol = 1e-4;
		double RUSTabAtol = 1e-6;
		double RUSTabHmin = 1e-2;
		double RUSTabZeroBranch = 4e-6;
		std::filesystem::path ResultPath;
		long SelectedRun = 0;
		bool RaidenStopOnOOS = false;
		long Threads = 1;
	};

	struct Input
	{
		const Options& Opts;
		std::filesystem::path CaseFile;
		std::filesystem::path ContingencyFile;
		std::filesystem::path rstPath;
		std::filesystem::path scnPath;
		std::filesystem::path dfwPath;
		std::filesystem::path macroPath;
		std::filesystem::path astraPath;
		std::filesystem::path currentPath;
		long CaseId = 0;
		size_t TotalRuns = 0;
		inline static std::mutex mtx_;
	};

	struct Output
	{
		std::stringstream Report;
		std::stringstream BriefReport;
		double TimeRaiden = 0.0;
		double TimeRustab = 0.0;
	};

	static void TestPair(const Input& Input, Output& Output);
	std::ofstream briefreport, fullreport;
	std::filesystem::path parametersPath_;
	void ReadParameters();
	Options GlobalOptions;
public:
	CBatchTest(const std::filesystem::path& parametersPath) :parametersPath_(parametersPath) {}
	void Run();
	void AddCase(std::filesystem::path CaseFile);
	void AddContingency(std::filesystem::path ContingencyFile);
};

