#include "stdafx.h"
#include <iomanip>
#include <fstream>
#include "SerializerJson.h"
#include "Messages.h"
#include "DynaNode.h"

using namespace DFW2;

JsonParsingState::JsonParsingState(JsonSaxWalkerBase* saxWalker, ptrdiff_t StackDepth, std::string_view Key) : JsonParsingState(StackDepth, Key)
{
	saxWalker->AddState(*this);
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
		case TypedSerializedValue::eValueType::VT_CPLX:
			item[ValueName] = {
				{JsonSaxWalkerBase::cszRe, mv.Value.pCplx->real()},
				{JsonSaxWalkerBase::cszIm ,mv.Value.pCplx->imag()}
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
		jsonLinkDirection[TypedSerializedValue::m_cszTypeDecs[6]] = pLinkedDevice->GetId();
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
			const CLinkPtrCount* pLinks = Serializer->GetDevice()->GetLink(link->nLinkIndex);
			CDevice** ppDevice(nullptr);
			// перебираем все устройства в мультисвязи и выводим
			// сериалиазацию для каждого
			while (pLinks->In(ppDevice))
				AddLink(jsonLinks, *ppDevice, bMaster);
		}
		break;
		case DLM_SINGLE:
			// для одиночной связи выводим только ее
			AddLink(jsonLinks, Serializer->GetDevice()->GetSingleLink(link->nLinkIndex), bMaster);
			break;
		}
	}
}



bool JsonSaxSerializer::start_array(std::size_t elements)
{
	JsonSaxWalkerBase::start_array(elements);
	
	// начинается некий массив, убеждаемся что это массив объектов, которые нас
	// интересуют (мы в "data\objects" и на нужной глубине стека

	if (stateInData && stateInObjects && StackDepth() == 5)
	{
		// находим сериализатор с именем ключа, который пришел из json
		if (auto it = m_SerializerMap.find(stack.back().Key());  it != m_SerializerMap.end())
		{
			itCurrentSerializer = it;
			//std::cout << "start " << it->first << std::endl;
			if (auto container = GetContainer(); container)
			{
				// контейнер есть, берем диапазон устройств
				// ставим состояние stateInDeviceArray, обновляем сериализатор
				// переменными из первого устройства контейнера

				itCurrentDevice = container->begin();
				itLastDevice = container->end();
				nDevicesCount = container->Count();
				if (itCurrentDevice != itLastDevice)
				{
					stateInDeviceArray = true;
					(*itCurrentDevice)->UpdateSerializer(itCurrentSerializer->second.get());
				}
				else
					ContainerDoesNotFitJsonArray(container);
			}
		}
		else
		{
			// если не нашли сериализатор для теущего json-объекта - сбрасываем текущий сериализатор
			itCurrentSerializer = m_SerializerMap.end();
		}
			
	}
	return true;
}


CDeviceContainer* JsonSaxSerializer::GetContainer()
{
	if (itCurrentSerializer != m_SerializerMap.end())
	{
		auto device = itCurrentSerializer->second->GetDevice();
		if (device)
			if (auto container = device->GetContainer(); container)
				return container;
	}
	return nullptr;
}

void JsonSaxSerializer::ContainerDoesNotFitJsonArray(CDeviceContainer* pContainer = nullptr)
{
	throw dfw2error(fmt::format("JsonSaxWalkerBase::start_array - device container with length set to {} cannot fit new device",
		"from \"{}\" serializer",
		pContainer ? pContainer->Count() : 0,
		itCurrentSerializer != m_SerializerMap.end() ? itCurrentSerializer->second->GetClassName() : ""));
}


bool JsonSaxSerializer::end_object()
{
	JsonSaxDataObjects::end_object();
	// заканчивается некоторый объект

	if (stateInData && stateInObjects && itCurrentSerializer != m_SerializerMap.end())
	{
		// мы находимся в "data/objects" и имеем сериализатор

		if (StackDepth() == 5)
		{
			// на этой глубине стека закрывается объект из массива 
			// проверяем все ли переменные объекта прочитаны

			auto unset = itCurrentSerializer->second->GetUnsetValues();

			if (unset.size())
			{
				STRINGLIST unsetNames;
				for (const auto& [Name, Var] : unset)
					unsetNames.push_back(Name);
				/*
				std::cout << fmt::format("Finished object {} : unset variables {}",
					itCurrentSerializer->second->GetClassName(),
					fmt::join(unsetNames, ",")) << std::endl;
				*/
			}

			if (stateInDeviceArray) 
			{
				// а еще мы находимся в чтении устройства контейнера, мы его прочитали 
				// и должны перейти к следующему

				// первое устройство в массиве мы уже обработали
				// переходим к следующему. Если мы попали сюда отработав
				// все доступные в контейнере устройства, выбрасываем исключение
				if (itCurrentDevice != itLastDevice)
				{
					auto& device = (*itCurrentDevice);
					ptrdiff_t index(itCurrentDevice - itLastDevice + nDevicesCount);
					device->SetDBIndex(index);
					if(device->GetId() < 0)
						device->SetId(index);
					itCurrentDevice++;
				}
				else
					ContainerDoesNotFitJsonArray(GetContainer());

				// если устройств больше нет, не ставим следующий сериализатор
				if (itCurrentDevice != itLastDevice)
					(*itCurrentDevice)->UpdateSerializer(itCurrentSerializer->second.get());
			}
		}
		else if (StackDepth() == 6)
		{
			// закрывается комплексное значение
			// ставим значение из буфера по сохранненому имени
			// в переменную сериализации
			SerializerSetNamedValue(complexName, complexValue);
			complexName.clear();
		}
	}
	return true;
}