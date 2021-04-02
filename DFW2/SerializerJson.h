#pragma once
#include "JsonWalkers.h"
#include "DeviceContainerProperties.h"

namespace DFW2
{

    // Сериализатор второго прохода, который реально читает данные в контейнеры

    using SerializerMap = std::map<std::string, SerializerPtr, std::less<> >;
    using SerializerMapItr = SerializerMap::iterator;

    

    // акцептор для расчета количества элементов
    // в массиве и построения картны названий массивов и их размерностей
    class JsonArrayElementCounter : public JsonSaxAcceptorBase
    {
    public:
        // карта название - количество элементов
        using ObjectMap = std::map<std::string, ptrdiff_t, std::less<>>;
    protected:
        ObjectMap m_ObjectMap;
        ptrdiff_t m_ItemsCount = 0;     // счетчик элементов
    public:

        JsonArrayElementCounter() : JsonSaxAcceptorBase(JsonObjectTypes::Array, "") {}

        // возвращаем готовую карту
        const ObjectMap& Objects()
        {
            return m_ObjectMap;
        }

        // после того, как акцептор объекта в массиве отработал - считаем новый элемент массива
        void NestedEnd(JsonSaxAcceptorBase* nested) override
        {
            JsonSaxAcceptorBase::NestedEnd(nested);
            m_ItemsCount++;
        }

        // в начале обработки массива
        void Start(const JsonStack& stack) override
        {
            JsonSaxAcceptorBase::Start(stack);
            // проверяем, есть ли название обрабатываемого массива в карте,
            // и если нет - создаем с нулевым числом элементов
            if (auto it(m_ObjectMap.find(Key())); it == m_ObjectMap.end())
                m_ObjectMap.insert(std::make_pair(Key(), 0));
            m_ItemsCount = 0;
        }

        // в конце обработки массива
        void End(const JsonStack& stack) override
        {
            JsonSaxAcceptorBase::End(stack);
            // находим название массива в карте и добавляем посчитанное 
            // количество элементов. Если названия массива нет - это необъяснимая просто ошибка
            if (auto it(m_ObjectMap.find(Key())); it != m_ObjectMap.end())
                it->second += m_ItemsCount;
            else
                throw dfw2error(fmt::format("JsonArrayElementCounter::End - no object in map after count has been finished"));
        }
    };

    // базовый сериализатор, настроенный на структуру входного json
    
    class JsonSaxAcceptorWalkerBase : public JsonSaxAcceptorWalker
    {
    protected:
        JsonSaxAcceptorBase* data;
    public:
        JsonSaxAcceptorWalkerBase() : JsonSaxAcceptorWalker(std::make_unique<JsonSaxAcceptorBase>(JsonObjectTypes::Object, ""))
        {
            auto powerSystem = new JsonSaxAcceptorBase(JsonObjectTypes::Object, "powerSystemModel");
            powerSystem->AddAcceptor(data = new JsonSaxAcceptorBase(JsonObjectTypes::Object, "data"));
            rootAcceptor->AddAcceptor(powerSystem);
        }
    };

    // сериализатор первого прохода - считает сколько объектов, которые
    // можно прочитать из json

    class JsonSaxElementCounter : public JsonSaxAcceptorWalkerBase
    {
        // акцептор массива, который считает элементы в массиве
        JsonArrayElementCounter* pArrayCounter = nullptr;
    public:
        JsonSaxElementCounter() : JsonSaxAcceptorWalkerBase()
        {
            auto object = new JsonSaxAcceptorBase(JsonObjectTypes::Object, "");
            pArrayCounter = new JsonArrayElementCounter();
            auto objects = new JsonSaxAcceptorBase(JsonObjectTypes::Object, "objects");
            pArrayCounter->AddAcceptor(object);
            objects->AddAcceptor(pArrayCounter);
            data->AddAcceptor(objects);
        }

        // возвращает карту объектов : название - количество
        const JsonArrayElementCounter::ObjectMap& Objects() const
        {
            return pArrayCounter->Objects();
        }
    };

    class JsonComplexValue : public JsonSaxAcceptorBase
    {
    protected:
        std::string currentKey;
        cplx complexValue;
        void NoCastToDouble(std::string_view type)
        {
            throw dfw2error(fmt::format("JsonComplexValue - cannot cast {} to double complex part", type));
        }

        void SetPart(double value)
        {
            if (currentKey == cszRe)
                complexValue.real(value);
            else if (currentKey == cszIm)
                complexValue.imag(value);
            else
                throw dfw2error("JsonComplexValue no part key (re/im) to set value");
        }


    public:
        JsonComplexValue() : JsonSaxAcceptorBase(JsonObjectTypes::Object, "") { }

        bool key(string_t& val) override
        {
            currentKey = val;
            stringutils::tolower(currentKey);
            return true;
        }

        const cplx& Value()
        {
            return complexValue;
        }

        bool null() override
        {
            NoCastToDouble("null");
            return false;
        }

        bool boolean(bool val) override
        {
            SetPart(val ? 1.0 : 0.0);
            return true;
        }

        bool number_integer(number_integer_t val) override
        {
            SetPart(static_cast<double>(val));
            return true;
        }

        bool number_unsigned(number_unsigned_t val) override
        {
            SetPart(static_cast<double>(val));
            return true;
        }

        bool number_float(number_float_t val, const string_t& s) override
        {
            SetPart(val);
            return true;
        }

        bool string(string_t& val) override
        {
            NoCastToDouble("null");
            return false;
        }

        static constexpr const char* cszRe = "re";
        static constexpr const char* cszIm = "im";
    };

    class JsonDeviceLinks: public JsonSaxAcceptorBase
    {
    public:
        JsonDeviceLinks() : JsonSaxAcceptorBase(JsonObjectTypes::Array, "Links")
        {

        }

        void Start(const JsonStack& stack) override
        {

        }

        void End(const JsonStack& stack) override
        {

        }
    };

    class JsonContainerObject : public JsonSaxAcceptorBase
    {
    protected:
        CSerializerBase* m_pSerializer = nullptr;
        std::string currentKey;
        JsonComplexValue* complex = nullptr;
        JsonDeviceLinks* links = nullptr;
    public:
        JsonContainerObject() : JsonSaxAcceptorBase(JsonObjectTypes::Object, "") 
        {
            AddAcceptor(complex = new JsonComplexValue());
            AddAcceptor(links = new JsonDeviceLinks());
        }

        void SetSerializer(CSerializerBase* serializer)
        {
            m_pSerializer = serializer;
        }

        template<typename T>
        void Set(const T& value)
        {
            if (auto input(m_pSerializer->at(currentKey)); input)
                input->Set<const T&>(value);
        }

        bool key(string_t& val) override
        {
            currentKey = val;
            return true;
        }

        bool null() override
        {
            Set(ptrdiff_t(0));
            return true;
        }

        bool boolean(bool val) override
        {
            Set(val);
            return true;
        }

        bool number_integer(number_integer_t val) override
        {
            Set(val);
            return true;
        }

        bool number_unsigned(number_unsigned_t val) override
        {
            Set(val);
            return true;
        }

        bool number_float(number_float_t val, const string_t& s) override
        {
            Set(val);
            return true;
        }

        bool string(string_t& val) override
        {
            Set(val);
            return true;
        }

        void NestedEnd(JsonSaxAcceptorBase* nested) override
        {
            if (nested == complex)
                Set(static_cast<JsonComplexValue*>(complex)->Value());
        }

        bool ConfirmAccept(const AcceptorPtr& acceptor, const JsonObject& jsonObject) override
        {
            if (auto input(m_pSerializer->at(currentKey)); input)
            {
                if (input->ValueType == TypedSerializedValue::eValueType::VT_CPLX && acceptor.get() == complex)
                    return true;
            }
            return false;
        }

    };



    class JsonContainerArray : public JsonSaxAcceptorBase
    {
    protected:
        CSerializerBase* m_pSerializer = nullptr;
        ptrdiff_t counter = 0;
    public:
        JsonContainerArray() : JsonSaxAcceptorBase(JsonObjectTypes::Array, "") {}

        void SetSerializer(CSerializerBase* serializer)
        {
            m_pSerializer = serializer;
            counter = 0;
        }

        void NestedStart(JsonSaxAcceptorBase* nested) override
        {
            if (!m_pSerializer) return;
            if(counter >= m_pSerializer->ItemsCount())
                throw dfw2error(fmt::format("JsonContainerArray::NestedStart - device container with length set to {} cannot fit new device",
                    "from \"{}\" serializer",
                    m_pSerializer->ItemsCount(),
                    m_pSerializer->GetClassName()));

            static_cast<JsonContainerObject*>(nested)->SetSerializer(m_pSerializer);
        }

        void NestedEnd(JsonSaxAcceptorBase* nested) override
        {
            if (!m_pSerializer) return;

            m_pSerializer->NextItem();
            counter++;
        }
    };


    class JsonContainers : public JsonSaxAcceptorBase
    {
    protected:
        SerializerMap m_SerializerMap;
    public:
        JsonContainers() : JsonSaxAcceptorBase(JsonObjectTypes::Object, "") {}

        void AddSerializer(const std::string_view& ClassName, SerializerPtr&& serializer)
        {
            m_SerializerMap.insert(std::make_pair(ClassName, std::move(serializer)));
        }

        void NestedStart(JsonSaxAcceptorBase* nested) override
        {
            JsonContainerArray* pArray(static_cast<JsonContainerArray*>(nested));

            if (auto its(m_SerializerMap.find(nested->Key())); its != m_SerializerMap.end())
            {
                pArray->SetSerializer(its->second.get());
            }
            else
                pArray->SetSerializer(nullptr);
        }
    };

    class JsonSaxMainSerializer : public JsonSaxAcceptorWalkerBase
    {
    protected:
        JsonContainers* containers;
    public:
        JsonSaxMainSerializer() : JsonSaxAcceptorWalkerBase()
        {
            data->AddAcceptor(containers = new JsonContainers());
            auto containerArray = new JsonContainerArray();
            auto containerObject = new JsonContainerObject();
            containers->AddAcceptor(containerArray);
            containerArray->AddAcceptor(containerObject);
        }

        void AddSerializer(const std::string_view& ClassName, SerializerPtr&& serializer)
        {
            containers->AddSerializer(ClassName, std::move(serializer));
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
		void AddLinks(const SerializerPtr& Serializer, nlohmann::json& jsonLinks, const LINKSUNDIRECTED& links, bool bMaster);
		TypeMapType m_TypeMap;
		std::filesystem::path m_Path;
	public:
		CSerializerJson() {}
		virtual ~CSerializerJson() {}
		void AddDeviceTypeDescription(ptrdiff_t nType, std::string_view Name);
		void CreateNewSerialization(const std::filesystem::path& path);
		void Commit();
		void SerializeClass(const SerializerPtr& Serializer);
        void SerializeProps(CSerializerBase* Serializer, nlohmann::json& Class);
        void SerializeData(CSerializerBase* pSerializer, nlohmann::json& Class);
		static const char* m_cszVim;
	};
}

