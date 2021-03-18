#pragma once

#include "Serializer.h"
#include "nlohmann/json.hpp"

namespace DFW2
{
	using FnNextSerializer = std::function<bool()>;
	class CSerializerJson
	{
	protected:
		// карта типов устройств
		using TypeMapType = std::map<ptrdiff_t, std::string>;
		nlohmann::json m_JsonDoc;
		nlohmann::json m_JsonData;
		nlohmann::json m_JsonStructure;
		void AddLink(nlohmann::json& jsonLinks, CDevice* pLinkedDevice, bool bMaster);
		void AddLinks(SerializerPtr& Serializer, nlohmann::json& jsonLinks, LINKSUNDIRECTED& links, bool bMaster);
		TypeMapType m_TypeMap;
	public:
		CSerializerJson() {}
		virtual ~CSerializerJson() {}
		void AddDeviceTypeDescription(ptrdiff_t nType, std::string_view Name);
		void CreateNewSerialization();
		void Commit();
		void SerializeClass(SerializerPtr& Serializer, FnNextSerializer NextSerializer = [] { return false; });
		static const char* m_cszVim;
	};
}

