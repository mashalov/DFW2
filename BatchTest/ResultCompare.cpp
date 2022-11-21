#include <stack>
#include <set>
#include <map>
#include "ResultCompare.h"
#include "..\DFW2\dfw2exception.h"
#import "progid:ResultFile.Raiden.1" named_guids no_namespace

std::string ResultCompare::Compare(const std::filesystem::path& ResultPath1, const std::filesystem::path& ResultPath2)
{
	std::string strResult;
	IResultPtr Result1, Result2;
	if (const HRESULT hr{ Result1.CreateInstance(CLSID_Result) }; FAILED(hr))
		throw dfw2error("Result CoCreate failed with scode {}", hr);
	if (const HRESULT hr{ Result2.CreateInstance(CLSID_Result) }; FAILED(hr))
		throw dfw2error("Result CoCreate failed with scode {}", hr);

	IResultReadPtr ResultRead1{ Result1->Load(_bstr_t(ResultPath1.c_str())) };
	IResultReadPtr ResultRead2{ Result2->Load(_bstr_t(ResultPath2.c_str())) };

	std::map<long, long> RUSTabDeviceTypes;
	for (const auto& DevType : DeviceTypeMap)
		RUSTabDeviceTypes.emplace(DevType.RUSTab.Type * 1000000 + DevType.RUSTab.SubType, DevType.Raiden);


	struct ComparedDevices
	{
		long DeviceId;
		std::string DeviceName;
		ICompareResultPtr Compare;
	};

	std::map<double, ComparedDevices, std::greater<double> > Comps;

	std::stack<IDevicePtr> stack;
	std::set<IDevicePtr> visited;
	stack.push(ResultRead1->GetRoot());
	while (!stack.empty())
	{
		IDevicePtr device{ stack.top() };
		stack.pop();
		if (auto MappedType{ RUSTabDeviceTypes.find(device->GetType()) } ; MappedType != RUSTabDeviceTypes.end())
		{
			IVariablesPtr Vars{ device->Variables };
			for (long v{ 0 }; v < Vars->Count; v++)
			{
				IVariablePtr Var{ Vars->Item(v) };
				if (Var->Name == _bstr_t("Delta"))
				{
					ICompareResultPtr CompareResult{ Var->Compare(ResultRead2->GetPlot(MappedType->second, device->Id, Var->Name)) };
					IMinMaxDataPtr Max{ CompareResult->Max };

					Comps.emplace(std::abs(Max->Metric), 
						ComparedDevices{
							device->Id, 
							stringutils::utf8_encode(device->Name), 
							CompareResult
						});

					while (Comps.size() > 10)
						Comps.erase(std::prev(Comps.end()));
				}
			}
		}
		if (visited.find(device) == visited.end())
		{
			visited.insert(device);
			IDevicesPtr children{ device->GetChildren() };
			for (long i = 0; i < children->GetCount(); i++)
				stack.push(children->Item(i));
		}
	}

	for (const auto& diff : Comps)
	{
		IMinMaxDataPtr Max{ diff.second.Compare->Max };
		strResult.append(fmt::format("{} {} Max {:.5f}({:.5f}) {:.5f}<=>{:.5f} Avg {:.5f}\n",
			diff.second.DeviceId,
			diff.second.DeviceName,
			Max->Metric,
			Max->Time,
			Max->Value1,
			Max->Value2,
			diff.second.Compare->Average
		)
		);
	}
	return strResult;
}
