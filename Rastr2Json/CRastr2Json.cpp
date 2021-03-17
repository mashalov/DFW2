#include "stdafx.h"
#include <windows.h>
#include "stringutils.h"
#include <fstream>
#include "CRastr2Json.h"


void CRastr2Json::AddPropertyIfNotEmpty(nlohmann::json& jobject, std::string key, const _variant_t& value)
{
	switch (value.vt) 
	{
	case VT_BSTR:
		{
			std::string str = stringutils::utf8_encode(value.bstrVal);
			if (str.length() > 0)
				jobject[key] = str;
		}
		break;
	case VT_R8:
		{
			if(value.dblVal != 0.0)
				jobject[key] = value.dblVal;
		}
		break;
	}
}

std::unique_ptr<nlohmann::json> CRastr2Json::WriteData()
{
	auto jsonData = std::make_unique<nlohmann::json>();
	return jsonData;
}


std::unique_ptr<nlohmann::json> CRastr2Json::WriteDBStructure() 
{
	auto jsonDBstructure = std::make_unique<nlohmann::json>();

	auto jsonTables = nlohmann::json::array();
	ITablesPtr Tables = m_Rastr->Tables;
	long tablesCount(Tables->GetCount());
	for (long xtable = 0; xtable < tablesCount; xtable++)
	{
		ITablePtr rastrTable = Tables->Item(xtable);
		auto jsonTable = nlohmann::json({ {"name", stringutils::utf8_encode(rastrTable->GetName()) } ,
			 							  { "description", stringutils::utf8_encode(rastrTable->GetDescription()) },
										  { "keys", stringutils::utf8_encode(rastrTable->GetKey()) }
			});
		IColsPtr Cols = rastrTable->Cols;
		long colsCount(Cols->GetCount());

		nlohmann::json jsonCols = nlohmann::json::array();
		for (long xcol = 0; xcol < colsCount; xcol++)
		{
			IColPtr rastrCol = Cols->Item(xcol);

			auto dataType = rastrCol->GetProp(PropType::FL_TIP).lVal;

			auto jsonCol = nlohmann::json({ { "name", stringutils::utf8_encode(rastrCol->GetName()) },
											{ "dataType", dataType }
				});

			AddPropertyIfNotEmpty(jsonCol, "caption", rastrCol->GetProp(PropType::FL_ZAG));
			AddPropertyIfNotEmpty(jsonCol, "description", rastrCol->GetProp(PropType::FL_DESC));

			if (dataType == PropTT::PR_REAL)
			{
				jsonCol["precision"] = rastrCol->GetProp(PropType::FL_PREC).lVal;
				AddPropertyIfNotEmpty(jsonCol, "formula", rastrCol->GetProp(PropType::FL_FORMULA));
				AddPropertyIfNotEmpty(jsonCol, "units", rastrCol->GetProp(PropType::FL_UNIT));
			}
			jsonCols.push_back(jsonCol);
		}

		jsonTable["fields"] = jsonCols;
		jsonTables.push_back(jsonTable);
	}

	jsonDBstructure->operator[]("tables") = jsonTables;

	return jsonDBstructure;
}

void CRastr2Json::WriteJson(std::filesystem::path JsonPath)
{
	auto jsonDatabase = nlohmann::json({});

	jsonDatabase["structure"] = *WriteDBStructure();
	jsonDatabase["data"] = *WriteData();

	m_json["rastrDatabase"] = jsonDatabase;

	std::ofstream fjson(JsonPath.wstring());
	if (fjson.is_open()) 
	{
		fjson << std::setw(4) << m_json << std::endl;
	}
	
}

void CRastr2Json::LoadRastrFile(std::filesystem::path RastrPath) 
{
	if (HRESULT hr = CoInitialize(NULL); FAILED(hr))
		throw std::runtime_error(fmt::format("CoInitialize failed with scode {}", hr));
	if (!m_Rastr)
		if (FAILED(m_Rastr.CreateInstance("Astra.Rastr.1")))
			throw std::runtime_error("Rastr COM Object unavailable");
	m_Rastr->Load(RG_KOD::RG_REPL, RastrPath.wstring().c_str(), L"");
}