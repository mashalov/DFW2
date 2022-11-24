#pragma once
#include <list>
#include <filesystem>
#include <fstream>
#include <chrono>
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
	std::filesystem::path rstPath_, dfwPath_, scnPath_, macroPath_;
	struct Options
	{
		bool EmsMode = false;
		long CaseId = 0;
		double Duration = 15;
		double RaidenAtol = 1e-4;
		double RUSTabHmin = 1e-2;
		std::filesystem::path ResultPath;
		long SelectedRun = 0;
	};
	void TestPair(const std::filesystem::path& CaseFile, const std::filesystem::path& ContingencyFile, const Options& Opts);
	std::ofstream report;
	std::filesystem::path parametersPath_;
	void ReadParameters();
	Options GlobalOptions;
public:
	CBatchTest(const std::filesystem::path& parametersPath) :parametersPath_(parametersPath) {}
	void Run();
	void AddCase(std::filesystem::path CaseFile);
	void AddContingency(std::filesystem::path ContingencyFile);
};

