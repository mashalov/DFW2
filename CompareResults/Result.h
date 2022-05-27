#pragma once
#import "progid:ResultFile.Raiden.1" named_guids
#include "Plot.h"
#include "TimeSeries.h"
#include <list>

using DEVICELIST = std::list<ResultFileLib::IDevicePtr>;
class CResult
{
	using TimeSeries = timeseries::TimeSeriesData<double, double>;
	using TimeSeriesOptions = TimeSeries::Options;
protected:
	CLog& Log_;
	ResultFileLib::IResultReadPtr m_ResultRead;;
	void Log(std::string_view log) const
	{
		Log_.Log(log);
	}
	void CompareDevices(const CResult& other, long DeviceType1, std::string_view PropName1, long DeviceType2, std::string_view PropName2, const CompareRange& range = {}) const;

public:
	CResult(CLog& Log) : Log_(Log) {}
	void Load(std::filesystem::path InputFile);
	TimeSeries ConstructFromPlot(_variant_t Input, const CompareRange& range = {}) const;
	DEVICELIST GetDevices(ptrdiff_t DeviceType) const;
	void Compare(const CResult& other, const CompareRange& range = {}) const;
	TimeSeries GetPlot(ptrdiff_t DeviceType, ptrdiff_t DeviceId, std::string_view Variable);

	static ResultFileLib::IVariablePtr GetVariable(ResultFileLib::IVariablesPtr& Variables, std::string_view VariableName);
	static ResultFileLib::IDevicePtr GetDevice(const DEVICELIST& Devices, ptrdiff_t Id);
};

