#include "Result.h"
#include <stack>
#include <set>
#include <map>

std::string GetHRESULT(HRESULT hr)
{
	return fmt::format("HRESULT = {:#0x}", static_cast<unsigned>(hr));
}

void CResult::Load(std::filesystem::path InputFile)
{
	try
	{
		ResultFileLib::IResultPtr result;
		HRESULT hr{ result.CreateInstance(ResultFileLib::CLSID_Result) };
		if (FAILED(hr))
			throw dfw2error(fmt::format("Failed to create Result File Class {}", GetHRESULT(hr)));
		m_ResultRead = result->Load(InputFile.c_str());
		if (m_ResultRead->GetChannels() == 0)
			return;
		// выбираем нулевой канал чтобы узнать диапазон времени, доступный в файле
		// (может неправильно работать, если нулевой канал медленная переменная, более 
		// надежно взять GetTimeScale
		auto testPlot{ ConstructFromPlot(m_ResultRead->GetPlotByIndex(0)) };
		Log(fmt::format("Loaded file {}, Channels {}, Points {}, Ratio {:3.2}",
			InputFile.filename().string(),
			m_ResultRead->GetChannels(),
			m_ResultRead->GetPoints(),
			m_ResultRead->GetCompressionRatio()));
	}
	catch (_com_error& er)
	{
		throw std::runtime_error(er.Description());
	}
}

DEVICELIST CResult::GetDevices(ptrdiff_t DeviceType) const
{
	DEVICELIST devices;
	std::stack<ResultFileLib::IDevicePtr> stack;
	std::set<ResultFileLib::IDevicePtr> visited;
	stack.push(m_ResultRead->GetRoot());
	while (!stack.empty())
	{
		ResultFileLib::IDevicePtr device = stack.top();
		stack.pop();
		if (device->GetType() == DeviceType)
			devices.push_back(device);
		if (visited.find(device) == visited.end())
		{
			visited.insert(device);
			ResultFileLib::IDevicesPtr children{ device->GetChildren() };
			for(long i = 0 ; i < children->GetCount() ; i++)
				stack.push(children->Item(i));
		}
	}
	Log(fmt::format("{} device(s) of type {} found", devices.size(), DeviceType));
	return devices;
}

CResult::TimeSeries CResult::ConstructFromPlot(_variant_t Input, const CompareRange& range) const
{
	if (Input.vt != (VT_ARRAY | VT_R8))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - array has wrong type {}", Input.vt));
	if (auto nDims{ SafeArrayGetDim(Input.parray) } ; nDims != 2)
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - array must have 2 dimensions {}", nDims));
	LONG lBound{ 0 }, uBound{ 0 };
	HRESULT hr{ SafeArrayGetLBound(Input.parray, 1, &lBound) };
	if (FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - cannot get low bound {}", GetHRESULT(hr)));
	hr = SafeArrayGetUBound(Input.parray, 1, &uBound);
	if (FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - cannot get high bound {}", GetHRESULT(hr)));

	double* pData{ nullptr };

	uBound -= lBound - 1;

	hr = SafeArrayAccessData(Input.parray, (void**)&pData);

	if (FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - SafeArrayAccessData failed {}", GetHRESULT(hr)));

	TimeSeries plot(uBound, pData + uBound, pData);

	hr = SafeArrayUnaccessData(Input.parray);

	if(FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - SafeArrayUnaccessData failed {}", GetHRESULT(hr)));

	return plot;
}


void CResult::Compare(const CResult& other, const CompareRange& range) const
{
	CompareDevices(other, 3, "V", 3 , "V", range);	// узлы
	//CompareDevices(other, 3, "V", 6 * 1000000 + 4, "vras", range);	// узлы
	//CompareDevices(other, 16, "Delta", 1 * 1000000 + 9, "Delta", range); // генераторы
	//CompareDevices(other, 16, "P", 1 * 1000000 + 9, "P", range); // генераторы
	//CompareDevices(other, 16, "Q", 1 * 1000000 + 9, "Q", range); // генераторы
	//CompareDevices(other, 16, "S", 1 * 1000000 + 9, "S", range); // генераторы
}

CResult::TimeSeries CResult::GetPlot(ptrdiff_t DeviceType, ptrdiff_t DeviceId, std::string_view Variable)
{
	TimeSeries plot;
	if (auto device{ CResult::GetDevice(GetDevices(DeviceType), DeviceId) }; device)
	{
		ResultFileLib::IVariablesPtr variables{ device->GetVariables() };
		if (auto variable{ CResult::GetVariable(variables, Variable) }; variable)
			plot = ConstructFromPlot(variable->GetPlot());
	}
	return plot;
}

void CResult::CompareDevices(const CResult& other, long DeviceType1, std::string_view PropName1, long DeviceType2, std::string_view PropName2, const CompareRange& range) const
{
	auto devices1{ GetDevices(DeviceType1) };
	auto devices2{ other.GetDevices(DeviceType2) };

	Log(fmt::format("Comparing devices {} and {} by variables \"{}\" and \"{}\"",
		DeviceType1,
		DeviceType2,
		PropName1,
		PropName2
		));

	struct CompareData
	{
		ResultFileLib::IDevicePtr device;
		TimeSeries::CompareResult diff;
	};

	std::map<double, CompareData, std::greater<double>> CompareOrder;

	// идем по найденным устройствам первых результатов
	for (auto&& dev1 : devices1)
	{
		// получаем переменные первого устройства
		ResultFileLib::IVariablesPtr vars1{ dev1->GetVariables() };
		// получаем переменную из первого устройства
		ResultFileLib::IVariablePtr var1{ CResult::GetVariable(vars1, PropName1) };
		if (var1 == nullptr)
			continue;

		long id = dev1->GetId();
		// ищем устройство с номером первого устройства во вторых результатах
		ResultFileLib::IDevicePtr device2{ CResult::GetDevice(devices2, dev1->GetId()) };

		if (device2)
		{
			// если нашли устройство во вторых результатах
			// получаем переменные второго устройства
			ResultFileLib::IVariablesPtr vars2{ device2->GetVariables() };
			// и находим переменную второго устройства
			ResultFileLib::IVariablePtr var2{ CResult::GetVariable(vars2, PropName2) };

			if (var2 == nullptr)
				continue;

			try
			{
				// строим графики результатов
				auto plot1{ ConstructFromPlot(var1->GetPlot(), range) };
				auto plot2{ other.ConstructFromPlot(var2->GetPlot(), range) };

				// определяем различия графиков в прямом и обратном направлениях

				CResult::TimeSeriesOptions Options;

				//Options.SetAtol(1e-4);
				//Options.SetRtol(1e-4);

				auto diff{ plot1.Compare(plot2, Options) };

				if (CompareOrder.size() < 10)
					CompareOrder.insert({ diff.Max().v(), {dev1, diff} });
				else
				{
					const double lastDiff{ std::prev(CompareOrder.end())->first };
					if (lastDiff < diff.Max().v())
					{
						CompareOrder.erase(lastDiff);
						CompareOrder.insert({ diff.Max().v(), {dev1, diff} });
					}
				}
			}
			catch (const std::runtime_error& ex)
			{
				Log(ex.what());
			}
		}
	}

	for(const auto& res : CompareOrder)
		Log(fmt::format("{:10} differences {:10f} at {:10f} ({:10f} vs {:10f})", 
			res.second.device->GetId(), 
			res.first, 
			res.second.diff.Max().t(),
			res.second.diff.Max().v1(),
			res.second.diff.Max().v2())
		);
}


ResultFileLib::IVariablePtr CResult::GetVariable(ResultFileLib::IVariablesPtr& Variables, std::string_view VariableName)
{
	const LONG nVars{ Variables->GetCount() };
	const _bstr_t bstrVarName{ std::string(VariableName).c_str() };
	for (LONG i = 0; i < nVars; i++)
	{
		ResultFileLib::IVariablePtr var{ Variables->Item(i) };
		if (var->GetName() == bstrVarName)
			return var;
	}
	return nullptr;
}

ResultFileLib::IDevicePtr CResult::GetDevice(const DEVICELIST& Devices, ptrdiff_t Id)
{
	for (const auto& dev : Devices)
		if (dev->GetId() == Id)
			return dev;
	return nullptr;
}