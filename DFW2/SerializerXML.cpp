#include "stdafx.h"
#include "SerializerXML.h"
#include "Messages.h"
#include "Dynanode.h"

using namespace DFW2;

// создаем новый документ и его заголовок
void CSerializerXML::CreateNewSerialization()
{
	m_spXMLDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));
	m_spXMLDoc->appendChild(m_spXMLDoc->createProcessingInstruction(L"xml", L"version=\"1.0\" encoding=\"utf-8\""));
	m_spXMLDoc->appendChild(m_spXMLDoc->createElement(L"DFW2"));
}

void CSerializerXML::AddDeviceTypeDescription(ptrdiff_t nType, std::string_view Name)
{
	if (!m_TypeMap.insert(std::make_pair(nType, Name)).second)
		throw dfw2error(fmt::format("CSerializerXML::AddDeviceTypeDescription - duplicate device type {}", nType));
}

// сериализация в xml метаданных сериализатора
void CSerializerXML::SerializeClassMeta(SerializerPtr& Serializer)
{
	// создаем узел описания класса
	MSXML2::IXMLDOMElementPtr spXMLClass = m_spXMLDoc->createElement(L"class");
	// задаем имя класса
	spXMLClass->setAttribute(L"name", Serializer->GetClassName());
	m_spXMLDoc->GetdocumentElement()->appendChild(spXMLClass);
	// создаем перечисление свойств класса
	MSXML2::IXMLDOMElementPtr spXMLProps = m_spXMLDoc->createElement(L"properties");
	spXMLClass->appendChild(spXMLProps);
	CDFW2Messages units;
	// перебираем значения из сериализатора
	for (auto&& value : *Serializer)
	{
		// добавляем свойство в xml
		MSXML2::IXMLDOMElementPtr spXMLProp = m_spXMLDoc->createElement(L"property");
		MetaSerializedValue& mv = *value.second;
		// задаем имя свойства
		spXMLProp->setAttribute(L"name", value.first.c_str());
		// задаем тип свойства либо текстом (если есть описание типа для значения перечисления), либо перечислением
		if(static_cast<ptrdiff_t>(mv.Value.ValueType) >= 0 &&
		   static_cast<ptrdiff_t>(mv.Value.ValueType) < _countof(TypedSerializedValue::m_cszTypeDecs))
			spXMLProp->setAttribute(CSerializerBase::m_cszType, TypedSerializedValue::m_cszTypeDecs[static_cast<ptrdiff_t>(mv.Value.ValueType)]);
		else
			spXMLProp->setAttribute(CSerializerBase::m_cszType, static_cast<long>(mv.Value.ValueType));

		// задаем множитель для вещественного значения
		if(mv.Value.ValueType == TypedSerializedValue::eValueType::VT_DBL && mv.Multiplier != 1.0)
			spXMLProp->setAttribute(L"multiplier", mv.Multiplier);
		// указываем признак переменной состояния
		if(mv.bState)
			spXMLProp->setAttribute(CSerializerBase::m_cszState,variant_t(1L));
		// задаем единицы измерения
		const auto& it = units.VarNameMap().find(static_cast<ptrdiff_t>(mv.Units));
		if(units.VarNameMap().end() != it)
			spXMLProp->setAttribute(L"units", it->second.c_str());
		spXMLProps->appendChild(spXMLProp);
	}
	// создаем заготовку для экземпляров из данного сериализатора
	m_spXMLItems = m_spXMLDoc->createElement(L"items");
	spXMLClass->appendChild(m_spXMLItems);
}


void CSerializerXML::AddLinks(SerializerPtr& Serializer, MSXML2::IXMLDOMElementPtr& spXMLLinks, LINKSUNDIRECTED& links, bool bMaster)
{
	// просматриваем связи устройства
	for (auto&& link: links)
	{
		switch (link->eLinkMode)
		{
		case DLM_MULTI:
			{
				// для мультисвязей выводим все
				CLinkPtrCount *pLinks = Serializer->m_pDevice->GetLink(link->nLinkIndex);
				CDevice **ppDevice(nullptr);
				// перебираем все устройства в мультисвязи и выводим
				// сериалиазацию для каждого
				while (pLinks->In(ppDevice))
					AddLink(spXMLLinks, *ppDevice, bMaster);
			}
			break;
		case DLM_SINGLE:
				// для одиночной связи выводим только ее
				AddLink(spXMLLinks, Serializer->m_pDevice->GetSingleLink(link->nLinkIndex), bMaster);
			break;
		}
	}
}

// сериализация связей устройства
void CSerializerXML::AddLink(MSXML2::IXMLDOMElementPtr& spXMLLinks, CDevice *pLinkedDevice, bool bMaster)
{
	if (pLinkedDevice)
	{
		// если задано связанное устройство
		// сериализуем тип связи - master или slave
		MSXML2::IXMLDOMElementPtr spXMLLink = m_spXMLDoc->createElement(bMaster ? L"master" : L"slave");
		spXMLLinks->appendChild(spXMLLink);
		// находим описание типа устройства
		const auto& typeit = m_TypeMap.find(pLinkedDevice->GetType());
		if(typeit == m_TypeMap.end())
			spXMLLink->setAttribute(CSerializerBase::m_cszType, pLinkedDevice->GetType()); // если описание не найдено - сериализуем перечисление
		else
			spXMLLink->setAttribute(CSerializerBase::m_cszType, typeit->second.c_str());
		// сериализуем идентификатор связанного устройства
		spXMLLink->setAttribute(TypedSerializedValue::m_cszTypeDecs[6], pLinkedDevice->GetId());
	}
}

// сериализация значений в xml
void CSerializerXML::SerializeClass(SerializerPtr& Serializer)
{
	// сериализуем имя класса
	MSXML2::IXMLDOMElementPtr spXMLClass = m_spXMLDoc->createElement(Serializer->GetClassName());
	m_spXMLItems->appendChild(spXMLClass);

	// обходим значения в сериализаторе
	for (auto&& value : *Serializer)
	{
		MetaSerializedValue& mv = *value.second;

		// сериализуем имя значения (если состояние - заменяем на стандартное)
		MSXML2::IXMLDOMElementPtr spXMLValue = m_spXMLDoc->createElement(value.second->Value.ValueType == TypedSerializedValue::eValueType::VT_STATE ? 
																									CSerializerBase::m_cszState : value.first.c_str());
		spXMLClass->appendChild(spXMLValue);
		// сериализуем значение в соответствии с типом
		// и метаданными
		switch (value.second->Value.ValueType)
		{
		case TypedSerializedValue::eValueType::VT_DBL:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, *mv.Value.Value.pDbl / mv.Multiplier);
				break;
		case TypedSerializedValue::eValueType::VT_INT:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, *mv.Value.Value.pInt);
				break;
		case TypedSerializedValue::eValueType::VT_BOOL:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, *mv.Value.Value.pBool);
				break;
		case TypedSerializedValue::eValueType::VT_CPLX:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, mv.Value.Value.pCplx->real());
			spXMLValue->setAttribute(m_cszVim, mv.Value.Value.pCplx->imag());
			break;
		case TypedSerializedValue::eValueType::VT_NAME:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, Serializer->m_pDevice->GetName());
				break;
		case TypedSerializedValue::eValueType::VT_STATE:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, static_cast<int>(Serializer->m_pDevice->GetState()));
			spXMLValue->setAttribute(CSerializerBase::m_cszStateCause, static_cast<int>(Serializer->m_pDevice->GetStateCause()));
				break;
		case TypedSerializedValue::eValueType::VT_ID:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, Serializer->m_pDevice->GetId());
				break;
		case TypedSerializedValue::eValueType::VT_ADAPTER:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, mv.Value.Adapter->GetString().c_str());
				break;
		default:
			throw dfw2error(fmt::format("CSerializerXML::SerializeClass wrong serializer type {}", mv.Value.ValueType));
		}
	}

	if (Serializer->m_pDevice)
	{
		// если в сериализаторе есть устройство
		CDeviceContainer *pContainer(Serializer->m_pDevice->GetContainer());
		// достаем контейнер устройства
		if (pContainer)
		{
			// и сериализуем связи данного экземпляра устройства
			// по свойствам контейнера
			CDeviceContainerProperties &Props = pContainer->m_ContainerProps;
			MSXML2::IXMLDOMElementPtr spXMLLinks = m_spXMLDoc->createElement(L"links");

			// добавляем связи от ведущих и ведомых
			AddLinks(Serializer, spXMLLinks, Props.m_Masters, true);
			AddLinks(Serializer, spXMLLinks, Props.m_Slaves, false);

			// проверяем, добавлены ли связи
			IXMLDOMNodeListPtr spNodes = spXMLLinks->selectNodes(L"*");
			long nCount(0);
			spNodes->get_length(&nCount);
			// если хотя бы одна связь добавлена - выводим связи в xml
			if (nCount)
				spXMLClass->appendChild(spXMLLinks);
		}
		// сериализуем идентификатор устройства
		spXMLClass->setAttribute(TypedSerializedValue::m_cszTypeDecs[6], Serializer->m_pDevice->GetId());
	}
}

void CSerializerXML::Commit()
{
	if(m_spXMLDoc)
		m_spXMLDoc->save(L"c:\\tmp\\serialization.xml");
}

const char* CSerializerXML::m_cszVim = "vim";
