#pragma once
#include "JsonWalkers.h"
#include "DeviceContainerProperties.h"

namespace DFW2
{

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

        // вводит состояние если глубина стека и ключ совпадают с заданными
        bool Push(ptrdiff_t StackDepth, const std::string_view Key) 
        {
            if (!m_State && StackDepth == m_StackDepth && Key == m_Key)
            {
                //std::cout << Key << " state on" << std::endl;
                m_State = true;
            }

            return m_State;
        } 

        // отменяет состояние, если глубина стека и ключ совпадают с заданными
        bool Pop(ptrdiff_t StackDepth, const std::string_view Key)
        {
            if (m_State && StackDepth == m_StackDepth && Key == m_Key)
            {
                //std::cout << Key << " state off" << std::endl;
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

    class JsonSaxWalkerBase : public JsonSaxStackWalker
    {
    protected:
        JsonParsingStates states;

        static bool StatesComp(const JsonParsingState* lhs, const JsonParsingState* rhs)
        {
            return lhs->StackDepth() < rhs->StackDepth();
        };

        std::unique_ptr<JsonParsingStateBoundSearch> boundSearchState = std::make_unique<JsonParsingStateBoundSearch>(0, "");

        void Push(JsonObjectTypes Type, std::string_view Key = {}) override
        {
            JsonSaxStackWalker::Push(Type, Key);

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

            JsonSaxStackWalker::Pop(Type);
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


    // Сериализатор второго прохода, который реально читает данные в контейнеры

    using SerializerMap = std::map<std::string, SerializerPtr, std::less<> >;
    using SerializerMapItr = SerializerMap::iterator;

    class JsonSaxSerializer : public JsonSaxDataObjects
    {
    protected:
        SerializerMap m_SerializerMap;
        SerializerMapItr itCurrentSerializer;
        size_t nDevicesCount = 0; // количество устройств в контейнере
        bool stateInDeviceArray = false; // флаг состояния "в векторе устройств"
        DEVICEVECTORITR itCurrentDevice;
        DEVICEVECTORITR itLastDevice;

        cplx complexValue;          // буфер для комплексного значения
        std::string complexName;    // имя для комплексного значения

        CDeviceContainer* GetContainer();
        void ContainerDoesNotFitJsonArray(CDeviceContainer* pContainer);

    public:
        JsonSaxSerializer() : itCurrentSerializer(m_SerializerMap.end())
        {

        }
        // добавить в карту сериалиазтор контейнера, по имени которого и списку
        // полей будет работать json-сериализатор
        void AddSerializer(const std::string_view& ClassName, SerializerPtr&& serializer)
        {
            m_SerializerMap.insert(std::make_pair(ClassName, std::move(serializer)));
        }

        // функция для всех типов значений, которые можно присвоить переменной сериализации
        // по имени
        template<typename T>
        void SerializerSetNamedValue(std::string_view Key, const T& value)
        {
            if(itCurrentDevice == itLastDevice)
                ContainerDoesNotFitJsonArray(GetContainer());

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
        bool end_object() override;

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

        bool end_array() override
        {
            JsonSaxWalkerBase::end_array();
            if (stateInData && stateInObjects && itCurrentSerializer != m_SerializerMap.end())
            {
                if (StackDepth() == 4)
                {
                    //std::cout << "finished objects " << itCurrentSerializer->second->GetClassName() << std::endl;
                    stateInDeviceArray = false;
                }
            }

            return true;
        }

        // overolad для double, который может идти в переменную
        // напрямую или идти в буфер комплексного значения
        void SerializerSetValue(const double& value)
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
    };

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

    class JsonObjectValue : public JsonSaxAcceptorBase
    {
    protected:
        MetaSerializedValue* m_InputValue = nullptr;
    public:
        JsonObjectValue() : JsonSaxAcceptorBase(JsonObjectTypes::Object, "") {}
        void SetInputValue(MetaSerializedValue* value)
        {
            m_InputValue = value;
        }

        bool null() override
        {
            if (!m_InputValue) return true;
            m_InputValue->Set(ptrdiff_t(0));
            return true;
        }

        bool boolean(bool val) override
        {
            if (!m_InputValue) return true;
            m_InputValue->Set(val);
            return true;
        }

        bool number_integer(number_integer_t val) override
        {
            if (!m_InputValue) return true;
            m_InputValue->Set(static_cast<ptrdiff_t>(val));
            return true;
        }

        bool number_unsigned(number_unsigned_t val) override
        {
            if (!m_InputValue) return true;
            m_InputValue->Set(static_cast<ptrdiff_t>(val));
            return true;
        }

        bool number_float(number_float_t val, const string_t& s) override
        {
            if (!m_InputValue) return true;
            m_InputValue->Set(val);
            return true;
        }

        bool string(string_t& val) override
        {
            if (!m_InputValue) return true;
            m_InputValue->Set<const std::string&>(val);
            return true;
        }
    };


    class JsonContainerObject : public JsonSaxAcceptorBase
    {
    protected:
        CSerializerBase* m_pSerializer = nullptr;
    public:
        JsonContainerObject() : JsonSaxAcceptorBase(JsonObjectTypes::Object, "") {}
        void SetSerializer(CSerializerBase* serializer)
        {
            m_pSerializer = serializer;
        }

        void NestedStart(JsonSaxAcceptorBase* nested) override
        {
            if (!m_pSerializer) return;
            JsonObjectValue* value(static_cast<JsonObjectValue*>(nested));
            value->SetInputValue(m_pSerializer->at(nested->Key()));
        }


        void NestedEnd(JsonSaxAcceptorBase* nested) override
        {
            
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
            auto objectValue = new JsonObjectValue();
            containers->AddAcceptor(containerArray);
            containerArray->AddAcceptor(containerObject);
            containerObject->AddAcceptor(objectValue);
           
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

