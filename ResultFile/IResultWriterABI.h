#pragma once
#include <filesystem>

using ResultIds = std::vector<ptrdiff_t>;

class IDeviceTypeABI
{
public:
	virtual void SetDeviceTypeMetrics(ptrdiff_t DeviceIdsCount, ptrdiff_t ParentIdsCount, ptrdiff_t DevicesCount) = 0;
	virtual void AddDeviceTypeVariable(const std::string_view VariableName, ptrdiff_t UnitsId, double Multiplier) = 0;
	virtual void AddDevice(const std::string_view DeviceName,
		const ResultIds& DeviceIds,
		const ResultIds& ParentIds,
		const ResultIds& ParentTypes) = 0;
};

class IResultWriterABI
{
public:
	virtual void Destroy() = 0;
	virtual void CreateResultFile(const std::filesystem::path FilePath) = 0;
	virtual void SetNoChangeTolerance(double Tolerance) = 0;
	virtual void SetComment(const std::string_view Comment) = 0;
	virtual void Close() = 0;
	virtual void FinishWriteHeader() = 0;
	virtual void WriteResults(double Time, double Step) = 0;
	virtual void AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName) = 0;
	virtual void SetChannel(ptrdiff_t nDeviceId, ptrdiff_t nDeviceType, ptrdiff_t nDeviceVarIndex, const double* pVariable, ptrdiff_t nVariableIndex) = 0;
	virtual IDeviceTypeABI* AddDeviceType(ptrdiff_t nDeviceType, const std::string_view DeviceTypeName) = 0;
	virtual void AddSlowVariable(ptrdiff_t nDeviceType,
		const ResultIds& DeviceIds,
		const std::string_view VariableName,
		double Time,
		double Value,
		double PreviousValue,
		const std::string_view ChangeDescription) = 0;
};
