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
        bool operator == (const JsonObject& other) const { return m_Type == other.m_Type && m_Key == other.m_Key; }

        JsonObject(JsonObjectTypes Type, const std::string_view& Key) : m_Key(Key), m_Type(Type) { }
        JsonObjectTypes Type() const { return m_Type; };
        void ChangeType(JsonObjectTypes Type) { m_Type = Type; }
        const std::string_view Key() const { return m_Key; }
        virtual ~JsonObject() {}
    };

    // стек объектов json
    using JsonStack = std::list<JsonObject>;


    class JsonSaxAcceptorBase : public JsonObject, public nlohmann::json_sax<nlohmann::json>
    {
    public:
        using AcceptorPtr = std::unique_ptr<JsonSaxAcceptorBase>;
        using AcceptorList = std::list<AcceptorPtr>;

    protected:
        std::string temporaryKey;
        AcceptorList m_Acceptors;
        JsonSaxAcceptorBase* m_ParentAcceptor = nullptr;
        ptrdiff_t m_StackDepth = -1;

        virtual void NestedStart(JsonSaxAcceptorBase* nested)
        {

        }

        virtual void NestedEnd(JsonSaxAcceptorBase* nested)
        {

        }

        virtual void Start(const JsonStack& stack)
        {

        }

        virtual void End(const JsonStack& stack)
        {

        }

    public:

        std::string VerbalName() const
        {
            return fmt::format("Type:{} Key:{}", Type(), Key());
        }

        const ptrdiff_t& StackDepth()
        {
            return m_StackDepth;
        }

        void StartFromStack(const JsonStack& stack)
        {
            m_StackDepth = stack.size();
            temporaryKey = Key();
            if (Key().empty())
                m_Key = stack.back().Key();

            Start(stack);

            if (m_ParentAcceptor)
                m_ParentAcceptor->NestedStart(this);
        }

        void EndAtStack(const JsonStack& stack)
        {
            if (stack.size() != m_StackDepth)
                throw dfw2error(fmt::format("JsonSaxAcceptorBase::StopAtStack - stack mismatch {}", VerbalName()));

            End(stack);

            if (m_ParentAcceptor)
                m_ParentAcceptor->NestedEnd(this);

            m_Key = temporaryKey;
        }


        JsonSaxAcceptorBase* Accept(const JsonObject& jsonObject)
        {
            std::list<JsonSaxAcceptorBase*> candidates;
            for (auto&& acceptor : m_Acceptors)
            {
                // если у акцептора не задан ключ, он подходит для любых jsonOбъектов совпадающего с ним типа
                if (acceptor->Type() == jsonObject.Type())
                    if (acceptor->Key().empty() || acceptor->Key() == jsonObject.Key())
                            candidates.push_back(acceptor.get());
            }
            if (candidates.size() == 1)
                return candidates.front();
            else if (candidates.size() > 1)
            {
                STRINGLIST ambigous;
                for (const auto& candidate : candidates)
                    ambigous.push_back(candidate->VerbalName());

                throw dfw2error(fmt::format("JsonSaxAcceptorBase::Accept - Ambigous child acceptors [{}] for acceptor {}",
                    fmt::join(ambigous, ","),
                    VerbalName()));
            }
            else
                return nullptr;
        }

        void AddAcceptor(JsonSaxAcceptorBase* acceptor)
        {
            m_Acceptors.emplace_back(acceptor);

            if (acceptor->m_ParentAcceptor)
                throw dfw2error(fmt::format("JsonSaxAcceptorBase::AddAcceptor - acceptor {} to be added to {} already has parent {}",
                    acceptor->VerbalName(),
                    VerbalName(),
                    acceptor->m_ParentAcceptor->VerbalName()));

            acceptor->m_ParentAcceptor = this;
        }

        JsonSaxAcceptorBase* GetParent()
        {
            return m_ParentAcceptor;
        }

        JsonSaxAcceptorBase(JsonObjectTypes Type, const std::string_view& Key) : 
            JsonObject(Type, Key)
        {}

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

        bool binary(binary_t& val) override
        {
            return true;
        }

        bool start_object(std::size_t elements) override
        {
            return true;
        }

        bool key(string_t& val) override
        {
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

        bool parse_error(std::size_t position, const std::string& last_token, const nlohmann::detail::exception& ex) override
        {
            return true;
        }
    };


    // обход json в sax-режиме под управлением nlohmann::json::sax_parse
    class JsonSaxStackWalker : public nlohmann::json_sax<nlohmann::json>
    {
    protected:
        JsonStack stack;

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

        bool binary(binary_t& val) override
        {
            Pop(JsonObjectTypes::Value);
            return true;
        }

        bool start_object(std::size_t elements) override
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

    class JsonSaxAcceptorWalker : public JsonSaxStackWalker
    {
    protected:
        JsonSaxAcceptorBase::AcceptorPtr rootAcceptor;
        JsonSaxAcceptorBase* m_CurrentAcceptor;
    public:
        JsonSaxAcceptorWalker(JsonSaxAcceptorBase::AcceptorPtr acceptor) : 
            rootAcceptor(std::move(acceptor)), 
            m_CurrentAcceptor(rootAcceptor.get()) { }

        bool null() override
        {
            m_CurrentAcceptor->null();
            return JsonSaxStackWalker::null();
        }

        bool boolean(bool val) override
        {
            m_CurrentAcceptor->boolean(val);
            return JsonSaxStackWalker::boolean(val);
        }

        bool number_integer(number_integer_t val) override
        {
            m_CurrentAcceptor->number_integer(val);
            return JsonSaxStackWalker::number_integer(val);
        }

        bool number_unsigned(number_unsigned_t val) override
        {
            m_CurrentAcceptor->number_unsigned(val);
            return JsonSaxStackWalker::number_unsigned(val);
        }

        bool number_float(number_float_t val, const string_t& s) override
        {
            m_CurrentAcceptor->number_float(val, s);
            return JsonSaxStackWalker::number_float(val, s);
        }

        bool string(string_t& val) override
        {
            m_CurrentAcceptor->string(val);
            return JsonSaxStackWalker::string(val);
        }

        bool binary(binary_t& val) override
        {
            m_CurrentAcceptor->binary(val);
            return JsonSaxStackWalker::binary(val);
        }

        bool start_object(std::size_t elements) override
        {
            JsonSaxStackWalker::start_object(elements);
            return m_CurrentAcceptor->start_object(elements);
        }

        bool key(string_t& val) override
        {
            JsonSaxStackWalker::key(val);
            return m_CurrentAcceptor->key(val);
        }

        bool end_object() override
        {
            m_CurrentAcceptor->end_object();
            return JsonSaxStackWalker::end_object();
        }

        bool start_array(std::size_t elements) override
        {
            JsonSaxStackWalker::start_array(elements);
            return m_CurrentAcceptor->start_array(elements);
        }

        bool end_array() override
        {
            m_CurrentAcceptor->end_array();
            return JsonSaxStackWalker::end_array();
        }

        bool parse_error(std::size_t position, const std::string& last_token, const nlohmann::detail::exception& ex) override
        {
            // !!!!!!!!! TODO !!!!!!!!! что-то делать с ошибкой, возможно просто throw или как-то fallback
            return m_CurrentAcceptor->parse_error(position, last_token, ex);
        }

        void NextAcceptor(JsonSaxAcceptorBase* pNewAcceptor)
        {
            m_CurrentAcceptor = pNewAcceptor;
            pNewAcceptor->StartFromStack(stack);
            //std::cout << "Entering acceptor " << pNewAcceptor->VerbalName() << std::endl;
        }

        void Push(JsonObjectTypes Type, std::string_view Key = {}) override
        {
            JsonSaxStackWalker::Push(Type, Key);
            if (auto newAcceptor = m_CurrentAcceptor->Accept(stack.back()); newAcceptor)
                NextAcceptor(newAcceptor);
        }

        void Pop(JsonObjectTypes Type) override
        {
            if (m_CurrentAcceptor->StackDepth() == stack.size())
            {
                //std::cout << "Leaving acceptor " << m_CurrentAcceptor->VerbalName() << std::endl;
                if (auto parent(m_CurrentAcceptor->GetParent()); parent)
                {
                    m_CurrentAcceptor->EndAtStack(stack);
                    m_CurrentAcceptor = parent;
                }
                else
                    throw dfw2error(fmt::format("JsonSaxAcceptorWalker::Pop - poping acceptor {} has no parent", m_CurrentAcceptor->VerbalName()));
            }
            JsonSaxStackWalker::Pop(Type);
        }
    };

}