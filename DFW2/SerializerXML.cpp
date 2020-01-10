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

void CSerializerXML::SerializeClassMeta(SerializerPtr& Serializer)
{
	MSXML2::IXMLDOMElementPtr spXMLClass = m_spXMLDoc->createElement(_T("class"));
	spXMLClass->setAttribute(_T("name"), Serializer->GetClassName().c_str());
	m_spXMLDoc->GetdocumentElement()->appendChild(spXMLClass);
	MSXML2::IXMLDOMElementPtr spXMLProps = m_spXMLDoc->createElement(_T("properties"));
	spXMLClass->appendChild(spXMLProps);
	CDFW2Messages units;
	for (auto&& value : *Serializer)
	{
		MSXML2::IXMLDOMElementPtr spXMLProp = m_spXMLDoc->createElement(_T("property"));
		MetaSerializedValue& mv = *value.second;
		spXMLProp->setAttribute(_T("name"), value.first.c_str());
		spXMLProp->setAttribute(_T("type"), static_cast<long>(mv.Value.ValueType));
		if(mv.Value.ValueType == TypedSerializedValue::eValueType::VT_DBL && mv.Multiplier != 1.0)
			spXMLProp->setAttribute(_T("multiplier"), mv.Multiplier);
		if(mv.bState)
			spXMLProp->setAttribute(_T("state"),variant_t(1L));
		auto& it = units.VarNameMap().find(static_cast<ptrdiff_t>(mv.Units));
		if(units.VarNameMap().end() != it)
			spXMLProp->setAttribute(_T("units"), it->second.c_str());
		spXMLProps->appendChild(spXMLProp);
	}
	m_spXMLItems = m_spXMLDoc->createElement(_T("items"));
	spXMLClass->appendChild(m_spXMLItems);
}

void CSerializerXML::SerializeClass(SerializerPtr& Serializer)
{
	MSXML2::IXMLDOMElementPtr spXMLClass = m_spXMLDoc->createElement(Serializer->GetClassName().c_str());
	m_spXMLItems->appendChild(spXMLClass);

	for (auto&& value : *Serializer)
	{
		MetaSerializedValue& mv = *value.second;
		MSXML2::IXMLDOMElementPtr spXMLValue = m_spXMLDoc->createElement(value.first.c_str());
		spXMLClass->appendChild(spXMLValue);
		switch (value.second->Value.ValueType)
		{
		case TypedSerializedValue::eValueType::VT_DBL:
			spXMLValue->setAttribute(CDynaNodeBase::m_cszV, *mv.Value.Value.pDbl / mv.Multiplier);
				break;
		case TypedSerializedValue::eValueType::VT_INT:
			spXMLValue->setAttribute(CDynaNodeBase::m_cszV, *mv.Value.Value.pInt);
				break;
		case TypedSerializedValue::eValueType::VT_BOOL:
			spXMLValue->setAttribute(CDynaNodeBase::m_cszV, *mv.Value.Value.pBool);
				break;
		case TypedSerializedValue::eValueType::VT_CPLX:
			spXMLValue->setAttribute(CDynaNodeBase::m_cszV, mv.Value.Value.pCplx->real());
			spXMLValue->setAttribute(CDynaNodeBase::m_cszVim, mv.Value.Value.pCplx->imag());
			break;
		case TypedSerializedValue::eValueType::VT_NAME:
			spXMLValue->setAttribute(CDynaNodeBase::m_cszV, Serializer->m_pDevice->GetName());
				break;
		case TypedSerializedValue::eValueType::VT_STATE:
			spXMLValue->setAttribute(CDynaNodeBase::m_cszV, (Serializer->m_pDevice->GetState() == eDEVICESTATE::DS_OFF) ? false : true);
				break;
		case TypedSerializedValue::eValueType::VT_ID:
			spXMLValue->setAttribute(CDynaNodeBase::m_cszV, Serializer->m_pDevice->GetId());
				break;
		case TypedSerializedValue::eValueType::VT_ADAPTER:
			spXMLValue->setAttribute(CDynaNodeBase::m_cszV, mv.Value.Adapter->GetString().c_str());
				break;
		default:
			throw dfw2error(Cex(_T("CSerializerXML::SerializeClass wrong serializer type %d"), mv.Value.ValueType));
		}
	}
}

void CSerializerXML::Commit()
{
	if(m_spXMLDoc)
		m_spXMLDoc->save(_T("c:\\tmp\\serialization.xml"));
}
