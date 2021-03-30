#pragma once
#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <list>
#include <iostream>
#include "nlohmann/json.hpp"
#include "dfw2exception.h"

namespace DFW2
{

    // типы объектов json
    enum class JsonObjectTypes
    {
        Object,
        Array,
        Key,
        Value
    };

    // объекты json для ведения стека
    class JsonObject
    {
    protected:
        std::string m_Key;          // имя объекта
        JsonObjectTypes m_Type;     // тип объекта
    public:
        JsonObject(JsonObjectTypes Type, const std::string_view& Key) : m_Key(Key), m_Type(Type) { }
        JsonObjectTypes Type() const { return m_Type; };
        void ChangeType(JsonObjectTypes Type) { m_Type = Type; }
        const std::string_view Key() const { return m_Key; }
    };

    // стек объектов json
    using JsonStack_t = std::list<JsonObject>;

    // обход json в sax-режиме под управлением nlohmann::json::sax_parse

    class JsonSaxStackWalker : public nlohmann::json_sax<nlohmann::json>
    {
    protected:
        JsonStack_t stack;
    
        void DumpStack(std::string_view pushpop)
        {
            std::cout << pushpop;
            for (const auto& s : stack)
                std::cout << " { " << s.Key() << " ; " << static_cast<ptrdiff_t>(s.Type()) << " } / ";
            std::cout << std::endl;
        }
    
        virtual void Push(JsonObjectTypes Type, std::string_view Key = {})
        {
            // если пришел объект или массив, и в стеке был ключ,
            // убираем ключ и ставим вместо него массив или объект. Ключ оставляем
            if ((Type == JsonObjectTypes::Object || Type == JsonObjectTypes::Array) && !stack.empty() && stack.back().Type() == JsonObjectTypes::Key)
                stack.back().ChangeType(Type);
            else
                stack.push_back(JsonObject(Type, Key));
        }

        virtual void Pop(JsonObjectTypes Type)
        {
            if (stack.empty())
                throw dfw2error("JsonSaxWalkerBase - parsing error: stack empty on Pop");

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
    };

}