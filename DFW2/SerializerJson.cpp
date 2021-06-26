#include "stdafx.h"
#include <iomanip>
#include <fstream>
#include "SerializerJson.h"
#include "Messages.h"
#include "DynaNode.h"

using namespace DFW2;


void JsonSerializerObject::Start(const JsonStack& stack)
{
	if(!nestedSerializer)
		AddAcceptor(nestedSerializer = new JsonSerializerArray());
}

bool JsonSerializerObject::ConfirmAccept(const AcceptorPtr& acceptor, const JsonObject& jsonObject)
{
	// акцепторы могут запуститься если в сериализаторе есть параметр соответствующий ключу
	if (auto input(m_pSerializer->at(currentKey)); input)
	{
		// для параметра комплексного типа разрешаем комплексный акцептор
		if (input->ValueType == TypedSerializedValue::eValueType::VT_CPLX && acceptor.get() == complex)
			return true;
		// для параметра, который представляет собой сериализатор разрешаем вложенный JsonSerializerArray
		if (input->ValueType == TypedSerializedValue::eValueType::VT_SERIALIZER && acceptor.get() == nestedSerializer)
		{
			nestedSerializer->SetSerializer(input->m_pNestedSerializer.get());
			return true;
		}
	}
	return false;
}

// создаем новый Json
void CSerializerJson::CreateNewSerialization(const std::filesystem::path& path)
{
	m_Path = path;
	m_JsonDoc = nlohmann::json();
}

void CSerializerJson::Commit()
{
	nlohmann::json jsonModel = nlohmann::json();
	nlohmann::json jsonDataObjects = nlohmann::json();
	nlohmann::json jsonStructureObjects = nlohmann::json();
	jsonDataObjects["objects"] = m_JsonData;
	jsonStructureObjects["objects"] = m_JsonStructure;
	jsonModel["data"]= jsonDataObjects;
	jsonModel["structure"] = jsonStructureObjects;
	m_JsonDoc["powerSystemModel"] = jsonModel;

	std::ofstream fjson(stringutils::utf8_decode(m_Path.string()));
	if (fjson.is_open())
		fjson << std::setw(4) << m_JsonDoc << std::endl;
}

void CSerializerJson::AddDeviceTypeDescription(ptrdiff_t nType, std::string_view Name)
{
	if (!m_TypeMap.insert(std::make_pair(nType, Name)).second)
		throw dfw2error(fmt::format("CSerializerJson::AddDeviceTypeDescription - duplicate device type {}", nType));
}

void CSerializerJson::SerializeProps(CSerializerBase* pSerializer, nlohmann::json& Class)
{
	// предполагается, что сериализатор содержит данные для первого объекта
	auto SetType = [this](MetaSerializedValue& mv, nlohmann::json& jsonType)
	{
		jsonType[CSerializerBase::m_cszDataType] = mv.GetTypeVerb();
		if (mv.ValueType == TypedSerializedValue::eValueType::VT_SERIALIZER)
		{
			auto serializerType = nlohmann::json();
			SerializeProps(mv.m_pNestedSerializer.get(), jsonType);
		}
	};

	auto ClassProps = nlohmann::json();

	CDFW2Messages units;
	// перебираем значения из сериализатора
	for (auto&& value : *pSerializer)
	{
		auto ClassProp = nlohmann::json();
		MetaSerializedValue& mv = *value.second;

		// задаем тип свойства либо текстом (если есть описание типа для значения перечисления), либо перечислением
		SetType(mv, ClassProp);

		// задаем множитель для вещественного значения
		if (mv.ValueType == TypedSerializedValue::eValueType::VT_DBL && mv.Multiplier != 1.0)
			ClassProp["multiplier"] = mv.Multiplier;
		// указываем признак переменной состояния
		if (mv.bState)
			ClassProp[CSerializerBase::m_cszState] = true;

		// задаем единицы измерения
		const auto& it = units.VarNameMap().find(static_cast<ptrdiff_t>(mv.Units));
		if (units.VarNameMap().end() != it)
			ClassProp["units"] = it->second;

		ClassProps[value.first] = ClassProp;
	}

	// добавляем свойства в класс
	Class["properties"] = ClassProps;
}

void CSerializerJson::SerializeData(CSerializerBase* pSerializer, nlohmann::json& item)
{
	// обходим значения в сериализаторе

	auto pDevice = pSerializer->GetDevice();

	for (auto&& value : *pSerializer)
	{
		if (!value.second->isSignificant())
			continue;

		MetaSerializedValue& mv = *value.second;
		// сериализуем имя значения (если состояние - заменяем на стандартное)
		std::string ValueName = value.second->ValueType == TypedSerializedValue::eValueType::VT_STATE ? CSerializerBase::m_cszState : value.first.c_str();
		// сериализуем значение в соответствии с типом
		// и метаданными
		switch (value.second->ValueType)
		{
		case TypedSerializedValue::eValueType::VT_DBL:
			item[ValueName] = *mv.Value.pDbl / mv.Multiplier;
			break;
		case TypedSerializedValue::eValueType::VT_INT:
			item[ValueName] = *mv.Value.pInt;
			break;
		case TypedSerializedValue::eValueType::VT_BOOL:
			item[ValueName] = *mv.Value.pBool;
			break;
		case TypedSerializedValue::eValueType::VT_STRING:
			item[ValueName] = *mv.Value.pStr;
			break;
		case TypedSerializedValue::eValueType::VT_CPLX:
			item[ValueName] = {
				{JsonComplexValue::cszRe, mv.Value.pCplx->real()},
				{JsonComplexValue::cszIm ,mv.Value.pCplx->imag()}
			};
			break;
		case TypedSerializedValue::eValueType::VT_NAME:
			if (pDevice)
				item[ValueName] = pDevice->GetName();
			break;
		case TypedSerializedValue::eValueType::VT_STATE:
			if (pDevice)
			{
				item[ValueName] = static_cast<int>(pDevice->GetState());
				item["stateCause"] = static_cast<int>(pDevice->GetStateCause());
			}
			break;
		case TypedSerializedValue::eValueType::VT_ID:
			if(pDevice)
				item[ValueName] = pDevice->GetId();
			break;
		case TypedSerializedValue::eValueType::VT_ADAPTER:
			item[ValueName] = mv.Adapter->GetString();
			break;
		case TypedSerializedValue::eValueType::VT_SERIALIZER:
			{
				// если вложенный сериализатор не пустой
				if (mv.m_pNestedSerializer->ItemsCount())
				{
					// создаем массив
					auto items = nlohmann::json::array();
					// обновляем сериализатор с первого элемента источника данных
					mv.m_pNestedSerializer->Update();
					do
					{
						// и сериализуем первый и последующие элементы
						// в массив
						auto data = nlohmann::json();
						SerializeData(mv.m_pNestedSerializer.get(), data);
						items.push_back(data);
					}
					while (mv.m_pNestedSerializer->NextItem());

					// массив вводим под именем сериализатора
					item[ValueName] = items;
				}
			}
			break;
		default:
			throw dfw2error(fmt::format("CSerializerJson::SerializeClass wrong serializer type {}", mv.ValueType));
		}
	}
}

// сериализация в json из сериализатора
void CSerializerJson::SerializeClass(const SerializerPtr& Serializer)
{
	if (!Serializer || !Serializer->ValuesCount())
		return;

	// создаем узел описания класса
	auto Class = nlohmann::json();
	SerializeProps(Serializer.get(), Class);
	m_JsonStructure[Serializer->GetClassName()] = Class;
	// перечень устройств размещаем в массиве json
	auto items = nlohmann::json::array();

	// если в сериализаторе есть устройство, проходим
	// по всем устройствам из контейнера
	CDevice* pDevice(Serializer->GetDevice());
	CDeviceContainer* pContainer(pDevice ? pDevice->GetContainer() : nullptr);
	size_t nDeviceIndex(0);
	bool bContinue(true);

	// продолжаем пока сериализатор
	// может переходить на следующий объект
	// одну сериализацию делаем в любом случае, так как получили
	// настроенный сериализатор

	do
	{
		auto item = nlohmann::json();
		SerializeData(Serializer.get(), item);

		// если есть контейнер, достаем и сериализуем
		// информаию о связях устройств
		if (pContainer)
		{
			// и сериализуем связи данного экземпляра устройства
			// по свойствам контейнера
			const CDeviceContainerProperties& Props = pContainer->m_ContainerProps;
			auto jsonLinks = nlohmann::json::array();
			// добавляем связи от ведущих и ведомых
			AddLinks(Serializer, jsonLinks, Props.m_Masters, true);
			AddLinks(Serializer, jsonLinks, Props.m_Slaves, false);
			// если что-то добавилось в ссылки - добавляем их в данные объекта
			// пустые ссылки не добавляем
			if (!jsonLinks.empty())
				item["Links"] = jsonLinks;
		}

		items.push_back(item);

	} while (Serializer->NextItem());

	// добавляем массив данных под именем объекта
	m_JsonData[Serializer->GetClassName()] = items;
}

// сериализация связей устройства
void CSerializerJson::AddLink(nlohmann::json& jsonLinks, const CDevice* pLinkedDevice, bool bMaster)
{
	if (pLinkedDevice)
	{
		// если задано связанное устройство
		// сериализуем тип связи - master или slave
		auto jsonLinkDirection = nlohmann::json();
		const char* szLinkDirection = bMaster ? "master" : "slave";

		// находим описание типа устройства
		const auto& typeit = m_TypeMap.find(pLinkedDevice->GetType());
		if (typeit == m_TypeMap.end())
			jsonLinkDirection[szLinkDirection] = pLinkedDevice->GetType(); // если описание не найдено - сериализуем перечисление
		else
			jsonLinkDirection[szLinkDirection] = typeit->second;

		// сериализуем идентификатор связанного устройства
		jsonLinkDirection[TypedSerializedValue::m_cszTypeDecs[7]] = pLinkedDevice->GetId();
		jsonLinks.push_back(jsonLinkDirection);
	}
}

void CSerializerJson::AddLinks(const SerializerPtr& Serializer, nlohmann::json& jsonLinks, const LINKSUNDIRECTED& links, bool bMaster)
{
	// просматриваем связи устройства
	for (auto&& link : links)
	{
		switch (link->eLinkMode)
		{
		case DLM_MULTI:
		{
			// для мультисвязей выводим все
			auto device = Serializer->GetDevice();
			// сначала проверяем есть ли в контейнере связь, объявленная в свойствах контейнера.
			// связи может не быть, если в модели нет устройств такого типа
			if (device->GetContainer()->CheckLink(link->nLinkIndex))
			{
				const CLinkPtrCount* pLinks = device->GetLink(link->nLinkIndex);
				CDevice** ppDevice(nullptr);
				// перебираем все устройства в мультисвязи и выводим
				// сериалиазацию для каждого
				while (pLinks->In(ppDevice))
					AddLink(jsonLinks, *ppDevice, bMaster);
			}
		}
		break;
		case DLM_SINGLE:
			// для одиночной связи выводим только ее
			AddLink(jsonLinks, Serializer->GetDevice()->GetSingleLink(link->nLinkIndex), bMaster);
			break;
		}
	}
}



