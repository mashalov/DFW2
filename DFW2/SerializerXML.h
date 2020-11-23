#pragma once
#include "Serializer.h"
#include "Release/msxml6.tlh"

namespace DFW2
{
	class CSerializerXML
	{
	protected:
		// карта типов устройств
		using TypeMapType = std::map<ptrdiff_t, std::wstring>;
		MSXML2::IXMLDOMDocument3Ptr m_spXMLDoc;
		MSXML2::IXMLDOMElementPtr m_spXMLItems;
		void AddLink(MSXML2::IXMLDOMElementPtr& xmlItem, CDevice *pLinkedDevice, bool bMaster);
		void AddLinks(SerializerPtr& Serializer, MSXML2::IXMLDOMElementPtr& spXMLLinks, LINKSUNDIRECTED& links, bool bMaster);
		TypeMapType m_TypeMap;
	public:
		CSerializerXML() {}
		virtual ~CSerializerXML() {}
		void AddDeviceTypeDescription(ptrdiff_t nType, std::wstring_view Name);
		void CreateNewSerialization();
		void Commit();
		void SerializeClass(SerializerPtr& Serializer);
		void SerializeClassMeta(SerializerPtr& Serializer);
		static const _TCHAR *m_cszVim;
	};
}

