#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "fmt/format.h"
#import "progid:Astra.Rastr.1" named_guids no_namespace
#include "stringutils.h"


class CJsonField
{
protected:
	std::string m_Name;
	long m_DataType;
public:
	CJsonField(std::string_view Name, long DataType) : m_Name(Name), m_DataType(DataType) {}
	const char* GetName() const
	{
		return m_Name.c_str();
	}
	long GetDataType() const
	{
		return m_DataType;
	}
};

class CRastrCol : public CJsonField
{
protected:
	IColPtr m_Col;
	_variant_t GetVariantData(long index) const;
public:
	CRastrCol(IColPtr Col) : m_Col(Col), CJsonField(stringutils::utf8_encode(Col->GetName()), Col->GetProp(PropType::FL_TIP).lVal) {}
	void AddPropertyIfNotEmpty(nlohmann::json& jobject, std::string key, const _variant_t& value) const;
	void StructureToJson(nlohmann::json& json) const;
	void DataToJson(nlohmann::json& json, long index) const;
	static constexpr const char* cszDataType = "dataType";
	static constexpr const char* cszTypes[] = { "int", "double", "string", "bool", "enum", "enpic", "color", "superenum", "time", "hex" };
};

using RastrTableCols = std::vector<CRastrCol>;

class CRastrTable 
{
protected:
	std::string m_Name;
	ITablePtr m_Table;
	RastrTableCols m_Cols;
public:
	CRastrTable(ITablePtr rastrTable);
	void StructureToJson(nlohmann::json & json) const;
	void DataToJson(nlohmann::json& json) const;
	const char* GetName() const 
	{
		return m_Name.c_str();
	}
};

using RastrTables = std::vector<CRastrTable>;

class CRastr2Json
{
protected:
	IRastrPtr m_Rastr;
	RastrTables m_Tables;
	nlohmann::json m_json;
	void WriteDBStructure(nlohmann::json& ParentJson);
	void WriteData(nlohmann::json& ParentJson);
	// json can't accept string_view key, so we pass string as key
	void AddPropertyIfNotEmpty(nlohmann::json& jobject, std::string key, const _variant_t& value);
	void GetCols(ITablePtr rastrTable, RastrTableCols& rastrCols);
public:
	CRastr2Json() 
	{

	}
	void LoadRastrFile(std::filesystem::path RastrPath);
	void WriteJson(std::filesystem::path JsonPath);
};


