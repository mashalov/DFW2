#include <stack>
#include <set>
#include "ResultCompare.h"
#include "..\DFW2\dfw2exception.h"

ResultCompare::CompareResultsT ResultCompare::Compare(const std::filesystem::path& ResultPath1, const std::filesystem::path& ResultPath2)
{
	CompareResultsT Results;

	Results.push_back({ 100, "Генератор", "Delta", {} });

	ResultFileLib::IResultPtr Result1, Result2;
	if (const HRESULT hr{ Result1.CreateInstance(ResultFileLib::CLSID_Result) }; FAILED(hr))
		throw dfw2error("Result CoCreate failed with scode {}", hr);
	if (const HRESULT hr{ Result2.CreateInstance(ResultFileLib::CLSID_Result) }; FAILED(hr))
		throw dfw2error("Result CoCreate failed with scode {}", hr);

	ResultFileLib::IResultReadPtr ResultRead1{ Result1->Load(_bstr_t(ResultPath1.c_str())) };
	ResultFileLib::IResultReadPtr ResultRead2{ Result2->Load(_bstr_t(ResultPath2.c_str())) };

	std::map<long, long> RUSTabDeviceTypes;
	for (const auto& DevType : DeviceTypeMap)
		RUSTabDeviceTypes.emplace(DevType.RUSTab.Type * 1000000 + DevType.RUSTab.SubType, DevType.Raiden);

	const _bstr_t bstrVarName{ Results.back().VariableName.c_str() };

	

	std::stack<ResultFileLib::IDevicePtr> stack;
	std::set<ResultFileLib::IDevicePtr> visited;
	stack.push(ResultRead1->GetRoot());
	while (!stack.empty())
	{
		ResultFileLib::IDevicePtr device{ stack.top() };
		stack.pop();
		if (auto MappedType{ RUSTabDeviceTypes.find(device->GetType()) } ; MappedType != RUSTabDeviceTypes.end())
		{
			ResultFileLib::IVariablesPtr Vars{ device->Variables };
			for (long v{ 0 }; v < Vars->Count; v++)
			{
				ResultFileLib::IVariablePtr Var{ Vars->Item(v) };
				if (Var->Name == bstrVarName)
				{
					ResultFileLib::ICompareResultPtr CompareResult{ Var->Compare(ResultRead2->GetPlot(MappedType->second, device->Id, Var->Name)) };
					ResultFileLib::IMinMaxDataPtr Max{ CompareResult->Max };

					auto& Comps{ Results.back().Ordered };

					Comps.emplace(std::abs(Max->Metric), 
						ComparedDevices{
							device->Id, 
							stringutils::utf8_encode(device->Name), 
							CompareResult
						});

					while (Comps.size() > 5)
						Comps.erase(std::prev(Comps.end()));
				}
			}
		}
		if (visited.find(device) == visited.end())
		{
			visited.insert(device);
			ResultFileLib::IDevicesPtr children{ device->GetChildren() };
			for (long i = 0; i < children->GetCount(); i++)
				stack.push(children->Item(i));
		}
	}
	return Results;
}
