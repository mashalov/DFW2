#pragma once
#include "Serializer.h"
#import <msxml6.dll> named_guids

namespace DFW2
{
	class CSerializerXML
	{
	protected:
		MSXML2::IXMLDOMDocument3Ptr m_spXMLDoc;
		MSXML2::IXMLDOMElementPtr m_spXMLItems;
	public:
		CSerializerXML() {}
		virtual ~CSerializerXML() {}
		void CreateNewSerialization();
		void Commit();
		void SerializeClass(unique_ptr<CSerializerBase>& Serializer);
		void SerializeClassMeta(unique_ptr<CSerializerBase>& Serializer);
	};
}

