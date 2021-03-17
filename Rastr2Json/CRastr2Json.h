#pragma once
#include <string>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "fmt/format.h"
#import "progid:Astra.Rastr.1" named_guids no_namespace
class CRastr2Json
{
protected:
	IRastrPtr m_Rastr;
	nlohmann::json m_json;
	std::unique_ptr<nlohmann::json> WriteDBStructure();
	std::unique_ptr<nlohmann::json> WriteData();
	// json can't accept string_view key, so we pass string as key
	void AddPropertyIfNotEmpty(nlohmann::json& jobject, std::string key, const _variant_t& value);
public:
	CRastr2Json() 
	{

	}
	void LoadRastrFile(std::filesystem::path RastrPath);
	void WriteJson(std::filesystem::path JsonPath);
};


