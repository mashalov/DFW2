#include "Result.h"
#include <stack>
#include <set>

std::string GetHRESULT(HRESULT hr)
{
	return fmt::format("HRESULT = {:#0x}", static_cast<unsigned>(hr));
}

void CResult::Analyze(std::filesystem::path InputFile)
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
		CPlot testPlot{ ConstructFromPlot(m_ResultRead->GetPlotByIndex(0)) };
		Log(fmt::format("Loaded file {}, Channels {}, Points {}, Time in [{};{}], Ratio {:3.2}",
			InputFile.filename().string(),
			m_ResultRead->GetChannels(),
			m_ResultRead->GetPoints(),
			testPlot.tmin(),
			testPlot.tmax(),
			m_ResultRead->GetCompressionRatio()));

		ptrdiff_t DeviceType{ 3 };
		auto devices{ GetDevices(DeviceType) };
		Log(fmt::format("{} device(s) of type {} found", devices.size(), DeviceType));

	}
	catch (_com_error& er)
	{
		throw std::runtime_error(er.Description());
	}
}

CResult::DEVICELIST CResult::GetDevices(ptrdiff_t DeviceType)
{
	CResult::DEVICELIST devices;
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
	return devices;
}

CPlot CResult::ConstructFromPlot(_variant_t Input)
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

	uBound -= lBound;

	hr = SafeArrayAccessData(Input.parray, (void**)&pData);

	if (FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - SafeArrayAccessData failed {}", GetHRESULT(hr)));

	CPlot plot(uBound, pData + uBound + 1, pData);

	hr = SafeArrayUnaccessData(Input.parray);

	if(FAILED(hr))
		throw dfw2error(fmt::format("CResult::ConstructFromPlot - SafeArrayUnaccessData failed {}", GetHRESULT(hr)));

	return plot;
}