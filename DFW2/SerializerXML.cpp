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

void CSerializerXML::SerializeClass(SerializerPtr& Serializer)
{
	MSXML2::IXMLDOMElementPtr spXMLClass = m_spXMLDoc->createElement(Serializer->GetClassName().c_str());
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
			spXMLValue->setAttribute(CSerializerBase::m_cszV, Serializer->m_pDevice->GetState());
			spXMLValue->setAttribute(CSerializerBase::m_cszStateCause, Serializer->m_pDevice->GetStateCause());
				break;
		case TypedSerializedValue::eValueType::VT_ID:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, Serializer->m_pDevice->GetId());
				break;
		case TypedSerializedValue::eValueType::VT_ADAPTER:
			spXMLValue->setAttribute(CSerializerBase::m_cszV, mv.Value.Adapter->GetString().c_str());
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

const _TCHAR* CSerializerXML::m_cszVim = _T("vim");
