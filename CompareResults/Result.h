#pragma once
#import "progid:ResultFile.Raiden.1" named_guids
#include "Plot.h"
#include <filesystem>
#include <list>

using DEVICELIST = std::list<ResultFileLib::IDevicePtr>;
class CResult
{
protected:
	CLog& m_Log;
	ResultFileLib::IResultReadPtr m_ResultRead;;
	void Log(std::string_view log) const
	{
		m_Log.Log(log);
	}
	void CompareDevices(const CResult& other, long DeviceType1, std::string_view PropName1, long DeviceType2, std::string_view PropName2, const CompareRange& range = {}) const;

public:
	CResult(CLog& Log) : m_Log(Log) {}
	void Load(std::filesystem::path InputFile);
	CPlot ConstructFromPlot(_variant_t Input, const CompareRange& range = {}) const;
	DEVICELIST GetDevices(ptrdiff_t DeviceType) const;
	void Compare(const CResult& other, const CompareRange& range = {}) const;

	static ResultFileLib::IVariablePtr GetVariable(ResultFileLib::IVariablesPtr& Variables, std::string_view VariableName);
	static ResultFileLib::IDevicePtr GetDevice(const DEVICELIST& Devices, LONG Id);

};

