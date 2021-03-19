#pragma once
#include <filesystem>
#include <map>
#include <iostream>
#include "Serializer.h"

#include "nlohmann/json.hpp"


namespace DFW2
{

	class CJsonSax : public nlohmann::json_sax<nlohmann::json>
	{
    protected:
        std::map<std::string, ptrdiff_t> objectmap;
    public:
        bool null() override
        {
            return true;
        }

        bool boolean(bool val) override
        {
            return true;
        }

        bool number_integer(number_integer_t val) override
        {
            return true;
        }

        bool number_unsigned(number_unsigned_t val) override
        {
            return true;
        }
        
        bool number_float(number_float_t val, const string_t& s) override
        {
            return true;
        }

        bool string(string_t& val) override
        {
            return true;
        }
        
        virtual bool binary(binary_t& val) override
        {
            return true;
        }

        virtual bool start_object(std::size_t elements) override
        {
            //std::cout << "object " << elements << std::endl;
            return true;
        }

        bool key(string_t& val) override
        {
            //std::cout << val << std::endl;
            return true;
        }
        
        bool end_object() override
        {
            return true;
        }
        
        bool start_array(std::size_t elements) override
        {
            return true;
        }
                
        bool end_array() override
        {
            return true;
        }
                
        bool parse_error(std::size_t position,  const std::string& last_token, const nlohmann::detail::exception& ex) override
        {
            return true;
        }
	};

	class CSerializerJson
	{
	protected:
		// карта типов устройств
		using TypeMapType = std::map<ptrdiff_t, std::string>;
		nlohmann::json m_JsonDoc;		// общий json документ
		nlohmann::json m_JsonData;		// данные
		nlohmann::json m_JsonStructure;	// структура данных
		void AddLink(nlohmann::json& jsonLinks, const CDevice* pLinkedDevice, bool bMaster);
		void AddLinks(SerializerPtr& Serializer, nlohmann::json& jsonLinks, const LINKSUNDIRECTED& links, bool bMaster);
		TypeMapType m_TypeMap;
		std::filesystem::path m_Path;
	public:
		CSerializerJson() {}
		virtual ~CSerializerJson() {}
		void AddDeviceTypeDescription(ptrdiff_t nType, std::string_view Name);
		void CreateNewSerialization(const std::filesystem::path& path);
		void Commit();
		void SerializeClass(SerializerPtr& Serializer);
		static const char* m_cszVim;
	};
}

