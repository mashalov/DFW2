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
		void SerializeClass(SerializerPtr& Serializer);
		void SerializeClassMeta(SerializerPtr& Serializer);

		static const _TCHAR *m_cszVim;
	};
}

