﻿#pragma once
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

    class JsonObject
    {
    protected:
        std::string m_Key;
        JsonObjectTypes m_Type;
    public:
        JsonObject(JsonObjectTypes Type, const std::string_view& Key) : m_Key(Key), m_Type(Type)
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

        const std::string_view Key() const
        {
            return m_Key;
        }
    };

    using JsonStack_t = std::list<JsonObject>;


    // обход json в SAX-режиме с контролем стека разбора
    // и фиксацией состояний

    class JsonSaxWalkerBase;

    class JsonParsingState
    {
    protected:
        ptrdiff_t m_StackDepth = 0;
        std::string m_Key;
        bool m_State = false;

    public:
        JsonParsingState(JsonSaxWalkerBase* saxWalker, ptrdiff_t StackDepth, std::string_view Key);

        JsonParsingState(ptrdiff_t StackDepth, const std::string_view Key) : m_StackDepth(StackDepth),
            m_Key(Key)
        { }

        // вводит состосяние если глубина стека и ключ совпадают с заданными
        bool Push(ptrdiff_t StackDepth, const std::string_view Key) 
        {
            if (!m_State && StackDepth == m_StackDepth && Key == m_Key)
            {
                std::cout << Key << " state on" << std::endl;
                m_State = true;
            }

            return m_State;
        } 

        // отменяет состояние, если глубина стека и ключ совпадают с заданными
        bool Pop(ptrdiff_t StackDepth, const std::string_view Key)
        {
            if (m_State && StackDepth == m_StackDepth && Key == m_Key)
            {
                std::cout << Key << " state off" << std::endl;
                m_State = false;
            }

            return m_State;
        }

        ptrdiff_t StackDepth() const
        {
            return m_StackDepth;
        }

        operator bool() const {
            return m_State;
        }
    };

    class JsonParsingStateBoundSearch : public JsonParsingState
    {
    public:
        using JsonParsingState::JsonParsingState;
        void SetStackDepth(ptrdiff_t nStackDepth)
        {
            m_StackDepth = nStackDepth;
        }
    };

    using JsonParsingStates = std::list<JsonParsingState*>;
    using JsonParsingStatesItr = JsonParsingStates::iterator;

    class JsonSaxWalkerBase : public nlohmann::json_sax<nlohmann::json>
    {
    protected:
        JsonStack_t stack;
        JsonParsingStates states;

        void DumpStack(std::string_view pushpop)
        {
            std::cout << pushpop;
            for (const auto& s : stack)
                std::cout << " { " << s.Key() << " ; " << static_cast<ptrdiff_t>(s.Type()) << " } / ";
            std::cout << std::endl;
        }

        static bool StatesComp(const JsonParsingState* lhs, const JsonParsingState* rhs)
        {
            return lhs->StackDepth() < rhs->StackDepth();
        };

        std::unique_ptr<JsonParsingStateBoundSearch> boundSearchState = std::make_unique<JsonParsingStateBoundSearch>(0, "");

        void Push(JsonObjectTypes Type, std::string_view Key = {})
        {
            // если пришел объект или массив, и в стеке был ключ,
            // убираем ключ и ставим вместо него массив или объект. Ключ оставляем
            if ((Type == JsonObjectTypes::Object || Type == JsonObjectTypes::Array) && !stack.empty() && stack.back().Type() == JsonObjectTypes::Key)
                stack.back().ChangeType(Type);
            else
                stack.push_back(JsonObject(Type, Key));

            ptrdiff_t nStackDepth(stack.size());
            const std::string_view& StackKey(stack.back().Key());

            auto itl = StatesLower(nStackDepth);
            const auto itu = StatesUpper(nStackDepth);

            while (itl != itu)
            {
                (*itl)->Push(nStackDepth, StackKey);
                itl++;
            }

            //DumpStack("push");
        }

        // !!!!!!! TODO !!!!!!!!!!! можно построить просто пары индексов
        // состояний для каждой глубины стека и не искать их каждый раз
        JsonParsingStatesItr StatesLower(ptrdiff_t nStackDepth)
        {
            boundSearchState->SetStackDepth(nStackDepth);
            return std::lower_bound(states.begin(), states.end(), boundSearchState.get(), StatesComp);
        }

        JsonParsingStatesItr StatesUpper(ptrdiff_t nStackDepth)
        {
            boundSearchState->SetStackDepth(nStackDepth);
            return std::upper_bound(states.begin(), states.end(), boundSearchState.get(), StatesComp);
        }

        void Pop(JsonObjectTypes Type)
        {
            if (stack.empty())
                throw dfw2error("JsonSaxWalkerBase - parsing error: stack empty on Pop");

            const auto BackType = stack.back().Type();

            ptrdiff_t nStackDepth(stack.size());
            const std::string_view StackKey(stack.back().Key());

            // определяем состояния, соответствующие текущей глубине стека
            auto itl = StatesLower(nStackDepth);
            const auto itu = StatesUpper(nStackDepth);

            // обходим состояния и проверяем
            // должны ли они быть отменены
            while (itl != itu)
            {
                (*itl)->Pop(nStackDepth, StackKey);
                itl++;
            }

            if (Type == JsonObjectTypes::Value)
            {
                // если обработали значение, и предыдущий объект был ключ - удаляем этот ключ
                if (BackType == JsonObjectTypes::Key)
                    stack.pop_back();
            }
            else
            {
                if (BackType != Type)
                    throw dfw2error("JsonSaxWalkerBase - parsing error: stack types mismatch");
                stack.pop_back();
            }

            //DumpStack("pop");
        }

        ptrdiff_t StackDepth() const
        {
            return stack.size();
        }

    public:

        void AddState(JsonParsingState& state)
        {
            states.insert(StatesLower(state.StackDepth()),&state);
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
            return true;
        }

        bool end_array() override
        {
            Pop(JsonObjectTypes::Array);
            return true;
        }

        bool parse_error(std::size_t position, const std::string& last_token, const nlohmann::detail::exception& ex) override
        {
            // !!!!!!!!! TODO !!!!!!!!! что-то делать с ошибкой, возможно просто throw или как-то fallback
            return true;
        }

        static constexpr const char* cszRe = "re";
        static constexpr const char* cszIm = "im";
    };

    class JsonSaxDataObjects : public JsonSaxWalkerBase
    {
    protected:
        // добавляем состояние для поиска внутри powerSystemModel/data/
        JsonParsingState stateInData = JsonParsingState(this, 3, "data");
        JsonParsingState stateInObjects = JsonParsingState(this, 4, "objects");
    };

	class JsonSaxCounter : public JsonSaxDataObjects
	{

    protected:
        using ObjectMap_t = std::map<std::string, ptrdiff_t>;
        using ObjectMapIterator_t = ObjectMap_t::iterator;
        ObjectMap_t m_ObjectMap;
        ObjectMapIterator_t m_itCurrentObject;

    public:

        JsonSaxCounter() : m_itCurrentObject(m_ObjectMap.end())
        {

        }

        virtual bool start_object(std::size_t elements) override
        {
            JsonSaxWalkerBase::start_object(elements);

            if(stateInData && stateInObjects && StackDepth() == 6 && m_itCurrentObject != m_ObjectMap.end())
                m_itCurrentObject->second++;

            return true;
        }
        
        bool start_array(std::size_t elements) override
        {
            JsonSaxWalkerBase::start_array(elements);

            if (stateInData && stateInObjects && StackDepth() == 5)
                m_itCurrentObject = m_ObjectMap.insert(std::make_pair(stack.back().Key(), 0)).first;
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

    using SerializerMap = std::map<std::string, SerializerPtr, std::less<> >;
    using SerializerMapItr = SerializerMap::iterator;

    class JsonSaxSerializer : public JsonSaxDataObjects
    {
    protected:
        SerializerMap m_SerializerMap;
        SerializerMapItr itCurrentSerializer;
        bool stateInDevice = false;
        DEVICEVECTORITR itCurrentDevice;
        DEVICEVECTORITR itLastDevice;

        cplx complexValue;          // буфер для комплексного значения
        std::string complexName;    // имя для комплексного значения

    public:
        JsonSaxSerializer() : itCurrentSerializer(m_SerializerMap.end())
        {

        }
        void AddSerializer(const std::string_view& ClassName, SerializerPtr& serializer)
        {
            m_SerializerMap.insert(std::make_pair(ClassName, std::move(serializer)));
        }

        // функция для всех типов значений, которые можно присвоить переменной сериализации
        // по имени
        template<typename T>
        void SerializerSetNamedValue(std::string_view Key, const T& value)
        {
            auto DeSerialize = itCurrentSerializer->second->at(Key);
            if (DeSerialize)
                DeSerialize->Set<const T&>(value);
            //std::cout << itCurrentSerializer->second->GetClassName() << "." << Key << "=" << value << std::endl;
        }

        // shortcut для значения, имя которого в стеке
        template<typename T>
        void SerializerSetValue(const T& value)
        {
            if (stateInData && stateInObjects && itCurrentSerializer != m_SerializerMap.end())
            {
                std::string_view Key(stack.back().Key());
                SerializerSetNamedValue<T>(Key, value);
            }
        }

        // специализация для double, который может идти в переменную
        // напрямую или идти в буфер комплексного значения
        template<> void SerializerSetValue<double>(const double& value)
        {
            if (stateInData && stateInObjects && itCurrentSerializer != m_SerializerMap.end())
            {
                std::string_view Key(stack.back().Key());
                if (complexName.length() > 0)
                {
                    // если буфер есть, определяем соствляющую по имени
                    if (Key == JsonSaxWalkerBase::cszRe)
                        complexValue.real(value);
                    else if (Key == JsonSaxWalkerBase::cszIm)
                        complexValue.imag(value);
                    else
                        throw dfw2error(fmt::format("SerializerSetValue<double> - wrong key for complex number - \"{}\"", Key));
                }
                else
                    SerializerSetNamedValue<double>(Key, value);
            }
        }

        bool boolean(bool val) override
        {
            SerializerSetValue(val);
            return JsonSaxDataObjects::boolean(val);;
        }

        bool number_integer(number_integer_t val) override
        {
            SerializerSetValue(val);
            return JsonSaxDataObjects::number_integer(val);
        }

        bool number_unsigned(number_unsigned_t val) override
        {
            SerializerSetValue(val);
            return JsonSaxDataObjects::number_unsigned(val);
        }


        bool number_float(number_float_t val, const string_t& s) override
        {
            SerializerSetValue(val);
            return JsonSaxDataObjects::number_float(val, s);
        }

        bool string(string_t& val) override
        {
            SerializerSetValue(val);
            return JsonSaxDataObjects::string(val);
        }

        bool start_array(std::size_t elements) override;

        bool start_object(std::size_t elements) override
        {
            JsonSaxDataObjects::start_object(elements);
            if (stateInData && stateInObjects && itCurrentSerializer != m_SerializerMap.end() && StackDepth() == 7)
            {
                // ожидаем комплексное значение
                complexValue.real(0.0);
                complexValue.imag(0.0);
                complexName = stack.back().Key();
            }
            return true;
        }


        bool end_object() override;

        bool end_array() override
        {
            JsonSaxWalkerBase::end_array();
            if (stateInData && stateInObjects && itCurrentSerializer != m_SerializerMap.end())
            {
                if (StackDepth() == 4)
                {
                    std::cout << "finished objects " << itCurrentSerializer->second->GetClassName() << std::endl;
                    stateInDevice = false;
                }
            }

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

