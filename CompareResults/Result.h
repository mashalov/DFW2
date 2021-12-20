#pragma once
#import "progid:ResultFile.Raiden.1" named_guids
#include "Plot.h"
#include <filesystem>
#include <list>

class CResult
{
	using DEVICELIST = std::list<ResultFileLib::IDevicePtr>;
protected:
	CLog& m_Log;
	ResultFileLib::IResultReadPtr m_ResultRead;;
	void Log(std::string_view log)
	{
		m_Log.Log(log);
	}
public:
	CResult(CLog& Log) : m_Log(Log) {}
	void Analyze(std::filesystem::path InputFile);
	CPlot ConstructFromPlot(_variant_t Input);
	DEVICELIST GetDevices(ptrdiff_t DeviceType);
};

