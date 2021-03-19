#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <list>
#include <iostream>
#include "Serializer.h"
#include "nlohmann/json.hpp"
#include "DeviceContainerProperties.h"


namespace DFW2
{
    enum class JsonObjectTypes
    {
        Object,
        Array,
        Key,
        Value
    };

    class CJsonObject
    {
    protected:
        std::string m_Key;
        JsonObjectTypes m_Type;
    public:
        CJsonObject(JsonObjectTypes Type, const std::string_view& Key) : m_Key(Key), m_Type(Type)
        {

        }

        JsonObjectTypes Type() const
        { 
            return m_Type;
        };

        void ChangeType(JsonObjectTypes Type)
        {
            m_Type = Type;
        }

        std::string_view Key() const
        {
            return m_Key;
        }
    };

    using JsonStack_t = std::list<CJsonObject>;

	class CJsonSax : public nlohmann::json_sax<nlohmann::json>
	{

    protected:
        using ObjectMap_t = std::map<std::string, ptrdiff_t>;
        using ObjectMapIterator_t = ObjectMap_t::iterator;
        using ObjectList = std::list<std::string>;
        ObjectMap_t m_ObjectMap;
        ObjectMapIterator_t m_itCurrentObject;
        JsonStack_t stack;
        const ptrdiff_t ObjectsAtNest = 4;

        bool m_bDataInStack = false;
        static constexpr const char* cszData = "data";

        void DumpStack(std::string_view pushpop)
        {
            std::cout << pushpop;
            for (const auto& s : stack)
                std::cout << " { " << s.Key() << " ; " << static_cast<ptrdiff_t>(s.Type()) << " } / ";
            std::cout << std::endl;
        }

        void Push(JsonObjectTypes Type, std::string_view Key = {})
        {
            // если пришел объект или массив, и в стеке был ключ,
            // убираем ключ и ставим вместо него массив или объект. Ключ оставляем
            if ((Type == JsonObjectTypes::Object || Type == JsonObjectTypes::Array) && !stack.empty() && stack.back().Type() == JsonObjectTypes::Key)
                stack.back().ChangeType(Type);
            else
                stack.push_back(CJsonObject(Type, Key));

            if (stack.size() == 3 && stack.back().Key() == cszData)
                m_bDataInStack = true;
        }

        void Pop(JsonObjectTypes Type)
        {
            if (stack.empty())
                throw dfw2error("CJsonSax - parsing error: stack empty on Pop");

            const auto BackType = stack.back().Type();
            if (Type == JsonObjectTypes::Value)
            {
                // если обработали значение, и предыдущий объект был ключ - удаляем этот ключ
                if (BackType == JsonObjectTypes::Key)
                    stack.pop_back();
            }
            else
            {
                if (BackType != Type)
                    throw dfw2error("CJsonSax - parsing error: stack types mismatch");
                stack.pop_back();
            }

            if (stack.size() == 3 && stack.back().Key() != cszData)
                m_bDataInStack = true;

        }

    public:

        CJsonSax() : m_itCurrentObject(m_ObjectMap.end())
        {

        }

        bool null() override
        {
            Pop(JsonObjectTypes::Value);
            return true;
        }

        bool boolean(bool val) override
        {
            Pop(JsonObjectTypes::Value);
            return true;
        }

        bool number_integer(number_integer_t val) override
        {
            Pop(JsonObjectTypes::Value);
            return true;
        }

        bool number_unsigned(number_unsigned_t val) override
        {
            Pop(JsonObjectTypes::Value);
            return true;
        }
        
        bool number_float(number_float_t val, const string_t& s) override
        {
            Pop(JsonObjectTypes::Value);
            return true;
        }

        bool string(string_t& val) override
        {
            Pop(JsonObjectTypes::Value);
            return true;
        }
        
        virtual bool binary(binary_t& val) override
        {
            Pop(JsonObjectTypes::Value);
            return true;
        }

        virtual bool start_object(std::size_t elements) override
        {
            Push(JsonObjectTypes::Object);

            if (m_bDataInStack && stack.size() == 6 && m_itCurrentObject != m_ObjectMap.end())
                m_itCurrentObject->second++;

            return true;
        }

        bool key(string_t& val) override
        {
            Push(JsonObjectTypes::Key, val);
            return true;
        }
        
        bool end_object() override
        {
            Pop(JsonObjectTypes::Object);
            return true;
        }
        
        bool start_array(std::size_t elements) override
        {
            Push(JsonObjectTypes::Array);
            if (m_bDataInStack && stack.size() == 5)
                m_itCurrentObject = m_ObjectMap.insert(std::make_pair(stack.back().Key(), 0)).first;
            return true;
        }
                
        bool end_array() override
        {
            Pop(JsonObjectTypes::Array);
            return true;
        }
                
        bool parse_error(std::size_t position,  const std::string& last_token, const nlohmann::detail::exception& ex) override
        {
            return true;
        }

        const ObjectMap_t GetObjectSizeMap() const
        {
            return m_ObjectMap;
        }

        void Dump()
        {
            for(const auto& [key, val] : m_ObjectMap)
            std::cout << key << " " << val << std::endl;
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

