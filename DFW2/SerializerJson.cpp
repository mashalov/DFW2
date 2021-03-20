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

// сериализация в json из сериализатора
void CSerializerJson::SerializeClass(SerializerPtr& Serializer)
{
	// предполагается, что сериализатор содержит данные для первого объекта

	auto SetType = [](MetaSerializedValue& mv, nlohmann::json& jsonType) 
	{
		const ptrdiff_t type(static_cast<ptrdiff_t>(mv.Value.ValueType));
		if(type >= 0 && type < static_cast<ptrdiff_t>(std::size(TypedSerializedValue::m_cszTypeDecs)))
			jsonType[CSerializerBase::m_cszDataType] = TypedSerializedValue::m_cszTypeDecs[type];
		else
			jsonType[CSerializerBase::m_cszDataType] = type;
	};

	// если в сериализаторе нет полей - пропускаем

	if (Serializer->ValuesCount() > 0)
	{
		// создаем узел описания класса
		auto Class = nlohmann::json();
		// создаем узел свойств класса
		auto ClassProps = nlohmann::json();

		CDFW2Messages units;
		// перебираем значения из сериализатора
		for (auto&& value : *Serializer)
		{
			auto ClassProp = nlohmann::json();
			MetaSerializedValue& mv = *value.second;

			// задаем тип свойства либо текстом (если есть описание типа для значения перечисления), либо перечислением
			SetType(mv, ClassProp);

			// задаем множитель для вещественного значения
			if (mv.Value.ValueType == TypedSerializedValue::eValueType::VT_DBL && mv.Multiplier != 1.0)
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
		m_JsonStructure[Serializer->GetClassName()] = Class;

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

		while(bContinue)
		{
			auto item = nlohmann::json();

			// обходим значения в сериализаторе
			for (auto&& value : *Serializer)
			{

				if (!value.second->Value.isSignificant())
					continue;

				MetaSerializedValue& mv = *value.second;
				// сериализуем имя значения (если состояние - заменяем на стандартное)
				std::string ValueName = value.second->Value.ValueType == TypedSerializedValue::eValueType::VT_STATE ? CSerializerBase::m_cszState : value.first.c_str();
				// сериализуем значение в соответствии с типом
				// и метаданными
				switch (value.second->Value.ValueType)
				{
				case TypedSerializedValue::eValueType::VT_DBL:
					item[ValueName] = *mv.Value.Value.pDbl / mv.Multiplier;
					break;
				case TypedSerializedValue::eValueType::VT_INT:
					item[ValueName] = *mv.Value.Value.pInt;
					break;
				case TypedSerializedValue::eValueType::VT_BOOL:
					item[ValueName] = *mv.Value.Value.pBool;
					break;
				case TypedSerializedValue::eValueType::VT_CPLX:
					item[ValueName] = { {"re", mv.Value.Value.pCplx->real()}, {"im" ,mv.Value.Value.pCplx->imag()} };
					break;
				case TypedSerializedValue::eValueType::VT_NAME:
					item[ValueName] = Serializer->GetDevice()->GetName();
					break;
				case TypedSerializedValue::eValueType::VT_STATE:
					item[ValueName] = static_cast<int>(Serializer->GetDevice()->GetState());
					item["stateCause"] = static_cast<int>(Serializer->GetDevice()->GetStateCause());
					break;
				case TypedSerializedValue::eValueType::VT_ID:
					item[ValueName] = Serializer->GetDevice()->GetId();
					break;
				case TypedSerializedValue::eValueType::VT_ADAPTER:
					item[ValueName] = mv.Value.Adapter->GetString();
					break;
				default:
					throw dfw2error(fmt::format("CSerializerJson::SerializeClass wrong serializer type {}", mv.Value.ValueType));
				}
			}

			// сериализуем идентификатор устройства
			// (было в xml, зачем ? мы его сериализовали из сериализатора в цикле по свойствам. Закомментировано.
			//item[TypedSerializedValue::m_cszTypeDecs[6]] = Serializer->m_pDevice->GetId();

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

				// у нас есть контейнер и было задано устройство
				// с помощью контейнера переходим к следующем устройству
				// (первое устройство уже обработали)

				pDevice = pContainer->GetDeviceByIndex(++nDeviceIndex);

				// если следующее устройство доступно, обновляем сериализатор на него
				if (pDevice)
					pDevice->UpdateSerializer(Serializer);
				else
					bContinue = false;	// если устройств больше нет - ставим флаг завершения обхода устройств
			}
			else
				bContinue = false; // если в сериализаторе нет устройства - заканчиваем 

			items.push_back(item);
		} 		

		// добавляем массив данных под именем объекта
		m_JsonData[Serializer->GetClassName()] = items;
	}
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

void CSerializerJson::AddLinks(SerializerPtr& Serializer, nlohmann::json& jsonLinks, const LINKSUNDIRECTED& links, bool bMaster)
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

