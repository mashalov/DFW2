#pragma once
#include "Header.h"
#include "DeviceContainerPropertiesBase.h"
#include "DLLStructs.h"

namespace DFW2
{
	// для ускорения обработки ссылок разделенных по иерархии и направлениям
	// используются те же карты, но с const указателями на second из основных карт ссылок
	using LINKSFROMMAPPTR = std::map<eDFW2DEVICETYPE, LinkDirectionFrom const*>;
	using LINKSTOMAPPTR = std::map<eDFW2DEVICETYPE, LinkDirectionTo const*>;
	// ссылки без разделения на направления
	using LINKSUNDIRECTED = std::vector<LinkDirectionFrom const*>;

	class CDeviceContainerProperties : public CDeviceContainerPropertiesBase
	{
	public:
		LINKSFROMMAPPTR m_MasterLinksFrom;
		LINKSTOMAPPTR  m_MasterLinksTo;
		LINKSUNDIRECTED m_Masters, m_Slaves;

		eDFW2DEVICETYPE GetType() const;
		const _TCHAR* GetVerbalClassName() const;
		const _TCHAR* GetSystemClassName() const;
		static const _TCHAR *m_cszNameGenerator1C, *m_cszSysNameGenerator1C;
		static const _TCHAR *m_cszNameGenerator3C, *m_cszSysNameGenerator3C;
		static const _TCHAR *m_cszNameGeneratorMustang, *m_cszSysNameGeneratorMustang;
		static const _TCHAR *m_cszNameGeneratorInfPower, *m_cszSysNameGeneratorInfPower;
		static const _TCHAR *m_cszNameGeneratorMotion, *m_cszSysNameGeneratorMotion;
		static const _TCHAR *m_cszNameExciterMustang, *m_cszSysNameExciterMustang;
		static const _TCHAR *m_cszNameExcConMustang, *m_cszSysNameExcConMustang;
		static const _TCHAR *m_cszNameDECMustang, *m_cszSysNameDECMustang;
		static const _TCHAR *m_cszNameNode, *m_cszSysNameNode;
		static const _TCHAR *m_cszNameBranch, *m_cszSysNameBranch;
		static const _TCHAR *m_cszNameBranchMeasure, *m_cszSysNameBranchMeasure;
		static const _TCHAR *m_cszAliasNode;
		static const _TCHAR *m_cszAliasBranch;
		static const _TCHAR *m_cszAliasGenerator;
	};
}

