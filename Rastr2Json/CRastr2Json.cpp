#include "stdafx.h"
#include <windows.h>
#include <fstream>
#include "CRastr2Json.h"


CRastrTable::CRastrTable(ITablePtr rastrTable) : m_Table(rastrTable), 
												 m_Name(stringutils::utf8_encode(rastrTable->GetName()))
{
	IColsPtr Cols = rastrTable->Cols;
	long colsCount(Cols->GetCount());
	m_Cols.clear();
	m_Cols.reserve(colsCount);
	for (long xcol = 0; xcol < colsCount; xcol++)
		m_Cols.emplace_back(Cols->Item(xcol));
}

void CRastrTable::StructureToJson(nlohmann::json & json) const
{
	json["description"] = stringutils::utf8_encode(m_Table->GetDescription());
	json["keys"] = stringutils::utf8_encode(m_Table->GetKey());

	nlohmann::json jsonCols = nlohmann::json();
	for (const auto& col : m_Cols)
	{
		auto jsonCol = nlohmann::json({});
		col.StructureToJson(jsonCol);
		jsonCols[col.GetName()] = jsonCol;
	}
	json["properties"] = jsonCols;
}

void CRastrTable::DataToJson(nlohmann::json& json) const
{
	auto jsonRecords = nlohmann::json::array();
	for (long index = 0; index < m_Table->GetSize(); index++)
	{
		nlohmann::json jsonCols = nlohmann::json();
		for (const auto& col : m_Cols)
			col.DataToJson(jsonCols, index);
		jsonRecords.push_back(jsonCols);
	}

	json[m_Name] = jsonRecords;
}

void CRastrCol::StructureToJson(nlohmann::json& json) const
{
	json["dataType"] = GetDataType();
	AddPropertyIfNotEmpty(json, "caption", m_Col->GetProp(PropType::FL_ZAG));
	AddPropertyIfNotEmpty(json, "description", m_Col->GetProp(PropType::FL_DESC));

	if (m_DataType == PropTT::PR_REAL)
	{
		AddPropertyIfNotEmpty(json, "precision", m_Col->GetProp(PropType::FL_PREC));
		AddPropertyIfNotEmpty(json, "formula", m_Col->GetProp(PropType::FL_FORMULA));
		AddPropertyIfNotEmpty(json, "units", m_Col->GetProp(PropType::FL_UNIT));
	}
}

_variant_t CRastrCol::GetVariantData(long index) const
{
	_variant_t value = m_Col->GetZ(index);
	switch (m_DataType) 
	{
	case PropTT::PR_REAL:
		value.ChangeType(VT_R8);
		break;
	case PropTT::PR_INT:
		value.ChangeType(VT_I4);
		break;
	case PropTT::PR_STRING:
		value.ChangeType(VT_BSTR);
		break;
	case PropTT::PR_ENUM:
		// OutEnumAsInt Required
		value.ChangeType(VT_I4);
		break;
	case PropTT::PR_ENPIC:
		// OutEnumAsInt Required
		value.ChangeType(VT_I4);
		break;
	case PropTT::PR_COLOR:
		// OutEnumAsInt Required
		value.ChangeType(VT_I4);
		break;
	case PropTT::PR_SUPERENUM:
		// OutEnumAsInt Required
		value.ChangeType(VT_I4);
		break;
	case PropTT::PR_TIME:
		// OutEnumAsInt Required
		value.ChangeType(VT_DATE);
		break;
	case PropTT::PR_HEX:
		// OutEnumAsInt Required
		value.ChangeType(VT_BSTR);
		break;
	}
	return value;
}

void CRastrCol::DataToJson(nlohmann::json& json, long index) const
{
	AddPropertyIfNotEmpty(json, m_Name, GetVariantData(index));
}

void CRastrCol::AddPropertyIfNotEmpty(nlohmann::json& jobject, std::string key, const _variant_t& value) const
{
	switch (value.vt) 
	{
	case VT_I4:
		if(value.lVal != 0)
			jobject[key] = value.lVal;
		break;
	case VT_BSTR:
		{
			std::string str = stringutils::utf8_encode(value.bstrVal);
			if (str.length() > 0)
				jobject[key] = str;
		}
		break;
	case VT_R8:
			if(value.dblVal != 0.0)
				jobject[key] = value.dblVal;
		break;
	}
}

void CRastr2Json::WriteData(nlohmann::json& ParentJson)
{
	auto jsonObjects = nlohmann::json();
	auto jsonTables = nlohmann::json();

	for (const auto& table : m_Tables)
		table.DataToJson(jsonTables);

	jsonObjects["objects"] = jsonTables;
	ParentJson["data"] = jsonObjects;
}


void CRastr2Json::WriteDBStructure(nlohmann::json& ParentJson) 
{
	auto jsonObjects = nlohmann::json();
	auto jsonTables = nlohmann::json();
	for (const auto& table : m_Tables)
	{
		auto jsonTable = nlohmann::json();
		table.StructureToJson(jsonTable);
		jsonTables[table.GetName()] = jsonTable;
	}
	jsonObjects["objects"] = jsonTables;
	ParentJson["structure"] = jsonObjects;
}

void CRastr2Json::WriteJson(std::filesystem::path JsonPath)
{
	auto jsonDatabase = nlohmann::json({});

	ITablesPtr Tables = m_Rastr->Tables;
	long tablesCount(Tables->GetCount());
	m_Tables.reserve(tablesCount);
	for (long xtable = 0; xtable < tablesCount; xtable++)
		m_Tables.emplace_back(Tables->Item(xtable));


	WriteDBStructure(jsonDatabase);
	WriteData(jsonDatabase);
	m_json["powerSystemModel"] = jsonDatabase;

	std::ofstream fjson(JsonPath.wstring());
	if (fjson.is_open()) 
		fjson << std::setw(4) << m_json << std::endl;
}

void CRastr2Json::LoadRastrFile(std::filesystem::path RastrPath) 
{
	if (HRESULT hr = CoInitialize(NULL); FAILED(hr))
		throw std::runtime_error(fmt::format("CoInitialize failed with scode {}", hr));
	if (!m_Rastr)
		if (FAILED(m_Rastr.CreateInstance("Astra.Rastr.1")))
			throw std::runtime_error("Rastr COM Object unavailable");
	m_Rastr->Load(RG_KOD::RG_REPL, RastrPath.wstring().c_str(), L"");

};