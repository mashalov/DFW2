#include "stdafx.h"
#include "SerializerXML.h"
#include "Messages.h"
#include "Dynanode.h"

using namespace DFW2;

void CSerializerXML::CreateNewSerialization()
{
	m_spXMLDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));
	m_spXMLDoc->appendChild(m_spXMLDoc->createProcessingInstruction(_T("xml"), _T("version=\"1.0\" encoding=\"utf-8\"")));
	m_spXMLDoc->appendChild(m_spXMLDoc->createElement(_T("DFW2")));
}

void CSerializerXML::AddDeviceTypeDescription(ptrdiff_t nType, std::wstring_view Name)
{
	if (!m_TypeMap.insert(std::make_pair(nType, Name)).second)
		throw dfw2error(fmt::format(_T("CSerializerXML::AddDeviceTypeDescription - duplicate device type {}"), nType));
}

void CSerializerXML::SerializeClassMeta(SerializerPtr& Serializer)
{
	MSXML2::IXMLDOMElementPtr spXMLClass = m_spXMLDoc->createElement(_T("class"));
	spXMLClass->setAttribute(_T("name"), Serializer->GetClassName());
	m_spXMLDoc->GetdocumentElement()->appendChild(spXMLClass);
	MSXML2::IXMLDOMElementPtr spXMLProps = m_spXMLDoc->createElement(_T("properties"));
	spXMLClass->appendChild(spXMLProps);
	CDFW2Messages units;
	for (auto&& value : *Serializer)
	{
		MSXML2::IXMLDOMElementPtr spXMLProp = m_spXMLDoc->createElement(_T("property"));
		MetaSerializedValue& mv = *value.second;
		spXMLProp->setAttribute(_T("name"), value.first.c_str());
		if(static_cast<ptrdiff_t>(mv.Value.ValueType) >= 0 &&
		   static_cast<ptrdiff_t>(mv.Value.ValueType) < _countof(TypedSerializedValue::m_cszTypeDecs))
			spXMLProp->setAttribute(CSerializerBase::m_cszType, TypedSerializedValue::m_cszTypeDecs[static_cast<ptrdiff_t>(mv.Value.ValueType)]);
		else
			spXMLProp->setAttribute(CSerializerBase::m_cszType, static_cast<long>(mv.Value.ValueType));

		if(mv.Value.ValueType == TypedSerializedValue::eValueType::VT_DBL && mv.Multiplier != 1.0)
			spXMLProp->setAttribute(_T("multiplier"), mv.Multiplier);
		if(mv.bState)
			spXMLProp->setAttribute(CSerializerBase::m_cszState,variant_t(1L));
		auto& it = units.VarNameMap().find(static_cast<ptrdiff_t>(mv.Units));
		if(units.VarNameMap().end() != it)
			spXMLProp->setAttribute(_T("units"), it->second.c_str());
		spXMLProps->appendChild(spXMLProp);
	}
	m_spXMLItems = m_spXMLDoc->createElement(_T("items"));
	spXMLClass->appendChild(m_spXMLItems);
}


void CSerializerXML::AddLinks(SerializerPtr& Serializer, MSXML2::IXMLDOMElementPtr& spXMLLinks, LINKSUNDIRECTED& links, bool bMaster)
{
	for (auto&& link: links)
	{
		switch (link->eLinkMode)
		{
		case DLM_MULTI:
			{
				CLinkPtrCount *pLinks = Serializer->m_pDevice->GetLink(link->nLinkIndex);
				CDevice **ppDevice(nullptr);
				while (pLinks->In(ppDevice))
					AddLink(spXMLLinks, *ppDevice, bMaster);
			}
			break;
		case DLM_SINGLE:
				AddLink(spXMLLinks, Serializer->m_pDevice->GetSingleLink(link->nLinkIndex), bMaster);
			break;
		}
	}
}

void CSerializerXML::AddLink(MSXML2::IXMLDOMElementPtr& spXMLLinks, CDevice *pLinkedDevice, bool bMaster)
{
	if (pLinkedDevice)
	{
		MSXML2::IXMLDOMElementPtr spXMLLink = m_spXMLDoc->createElement(bMaster ? _T("master") : _T("slave"));
		spXMLLinks->appendChild(spXMLLink);
		auto& typeit = m_TypeMap.find(pLinkedDevice->GetType());
		if(typeit == m_TypeMap.end())
			spXMLLink->setAttribute(CSerializerBase::m_cszType, pLinkedDevice->GetType());
		else
			spXMLLink->setAttribute(CSerializerBase::m_cszType, typeit->second.c_str());
		spXMLLink->setAttribute(TypedSerializedValue::m_cszTypeDecs[6], pLinkedDevice->GetId());
	}
}

void CSerializerXML::SerializeClass(SerializerPtr& Serializer)
{
	MSXML2::IXMLDOMElementPtr spXMLClass = m_spXMLDoc->createElement(Serializer->GetClassName());
	m_spXMLItems->appendChild(spXMLClass);

	for (auto&& value : *Serializer)
	{
		MetaSerializedValue& mv = *value.second;

		MSXML2::IXMLDOMElementPtr spXMLValue = m_spXMLDoc->createElement(value.second->Value.ValueType == TypedSerializedValue::eValueType::VT_STATE ? 
																									CSerializerBase::m_cszState : value.first.c_str());
		spXMLClass->appendChild(spXMLValue);
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
			throw dfw2error(fmt::format(_T("CSerializerXML::SerializeClass wrong serializer type {}"), mv.Value.ValueType));
		}
	}

	if (Serializer->m_pDevice)
	{
		CDeviceContainer *pContainer(Serializer->m_pDevice->GetContainer());
		if (pContainer)
		{
			CDeviceContainerProperties &Props = pContainer->m_ContainerProps;
			MSXML2::IXMLDOMElementPtr spXMLLinks = m_spXMLDoc->createElement(_T("links"));

			AddLinks(Serializer, spXMLLinks, Props.m_Masters, true);
			AddLinks(Serializer, spXMLLinks, Props.m_Slaves, false);

			IXMLDOMNodeListPtr spNodes = spXMLLinks->selectNodes(_T("*"));
			long nCount(0);
			spNodes->get_length(&nCount);
			if (nCount)
				spXMLClass->appendChild(spXMLLinks);
		}
		spXMLClass->setAttribute(TypedSerializedValue::m_cszTypeDecs[6], Serializer->m_pDevice->GetId());
	}
}

void CSerializerXML::Commit()
{
	if(m_spXMLDoc)
		m_spXMLDoc->save(_T("c:\\tmp\\serialization.xml"));
}

const _TCHAR* CSerializerXML::m_cszVim = _T("vim");
