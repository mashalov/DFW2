#pragma once
#include "Header.h"
#include "DeviceTypes.h"

namespace DFW2
{
	
	enum eDEVICEVARIABLETYPE
	{
		eDVT_CONSTSOURCE,
		eDVT_INTERNALCONST,
		eDVT_STATE
	};

	class CVarIndexBase
	{
	public:
		ptrdiff_t m_nIndex;
		CVarIndexBase(ptrdiff_t nIndex) : m_nIndex(nIndex) { };
	};

	class CVarIndex : public CVarIndexBase
	{
	public:
		bool m_bOutput;
		double m_dMultiplier;
		eVARUNITS m_Units;
		CVarIndex(ptrdiff_t nIndex, eVARUNITS eVarUnits) : CVarIndexBase(nIndex),
			m_bOutput(true),
			m_dMultiplier(1.0),
			m_Units(eVarUnits)
		{

		};

		CVarIndex(ptrdiff_t nIndex, bool bOutput, eVARUNITS eVarUnits) : CVarIndexBase(nIndex),
			m_bOutput(bOutput),
			m_dMultiplier(1.0),
			m_Units(eVarUnits)
		{

		};
	};

	class CConstVarIndex : public CVarIndexBase
	{
	public:
		eDEVICEVARIABLETYPE m_DevVarType;
		CConstVarIndex(ptrdiff_t nIndex, eDEVICEVARIABLETYPE eDevVarType) : CVarIndexBase(nIndex),
			m_DevVarType(eDevVarType)
		{

		}
	};

	typedef std::map<std::wstring, CVarIndex> VARINDEXMAP;
	typedef VARINDEXMAP::iterator VARINDEXMAPITR;
	typedef VARINDEXMAP::const_iterator VARINDEXMAPCONSTITR;

	typedef std::map<std::wstring, CConstVarIndex> CONSTVARINDEXMAP;
	typedef CONSTVARINDEXMAP::iterator CONSTVARINDEXMAPITR;
	typedef CONSTVARINDEXMAP::const_iterator CONSTVARINDEXMAPCONSTITR;

	typedef std::set<ptrdiff_t> TYPEINFOSET;
	typedef TYPEINFOSET::iterator TYPEINFOSETITR;

	struct LinkDirectionFrom
	{
		ptrdiff_t nLinkIndex;
		eDFW2DEVICELINKMODE eLinkMode;
		eDFW2DEVICEDEPENDENCY eDependency;
		LinkDirectionFrom() : nLinkIndex(0),
							  eLinkMode(DLM_SINGLE),
							  eDependency(DPD_MASTER)
							  {}
		LinkDirectionFrom(eDFW2DEVICELINKMODE LinkMode, eDFW2DEVICEDEPENDENCY Dependency, ptrdiff_t LinkIndex) : 
			eLinkMode(LinkMode),
			nLinkIndex(LinkIndex),
			eDependency(Dependency)
			{}
	};

	struct LinkDirectionTo : LinkDirectionFrom
	{
		std::wstring strIdField;
		LinkDirectionTo() : LinkDirectionFrom()
		{
			strIdField.clear();
		}
		LinkDirectionTo(eDFW2DEVICELINKMODE LinkMode, eDFW2DEVICEDEPENDENCY Dependency, ptrdiff_t LinkIndex, const _TCHAR *cszIdField) : 
			LinkDirectionFrom(LinkMode, Dependency, LinkIndex),
			strIdField(cszIdField) 
			{}
	};

	typedef std::map<eDFW2DEVICETYPE, LinkDirectionFrom> LINKSFROMMAP;
	typedef LINKSFROMMAP::iterator LINKSFROMMAPITR;

	typedef std::map<eDFW2DEVICETYPE, LinkDirectionTo> LINKSTOMAP;
	typedef LINKSTOMAP::iterator LINKSTOMAPITR;

	class CDeviceContainerProperties
	{
	public:
		CDeviceContainerProperties();
		~CDeviceContainerProperties();

		void SetType(eDFW2DEVICETYPE eDevType);
		void AddLinkTo(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency, const _TCHAR* cszstrIdField);
		void AddLinkFrom(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency);
		bool bNewtonUpdate;
		bool bCheckZeroCrossing;
		bool bPredict;
		ptrdiff_t nPossibleLinksCount;
		ptrdiff_t nEquationsCount;
		eDFW2DEVICETYPE		eDeviceType;
		VARINDEXMAP m_VarMap;
		CONSTVARINDEXMAP m_ConstVarMap;
		TYPEINFOSET m_TypeInfoSet;

		LINKSFROMMAP m_LinksFrom;
		LINKSTOMAP  m_LinksTo;

		LINKSFROMMAP m_MasterLinksFrom;
		LINKSTOMAP  m_MasterLinksTo;

		std::wstring m_strClassName;

		STRINGLIST m_lstAliases;

		static const _TCHAR *m_cszNameGenerator1C;
		static const _TCHAR *m_cszNameGenerator3C;
		static const _TCHAR *m_cszNameGeneratorMustang;
		static const _TCHAR *m_cszNameGeneratorInfPower;
		static const _TCHAR *m_cszNameGeneratorMotion;
		static const _TCHAR *m_cszNameExciterMustang;
		static const _TCHAR *m_cszNameExcConMustang;
		static const _TCHAR *m_cszNameDECMustang;
		static const _TCHAR *m_cszNameNode;
		static const _TCHAR *m_cszNameBranch;
		static const _TCHAR *m_cszNameBranchMeasure;

		static const _TCHAR *m_cszAliasNode;
		static const _TCHAR *m_cszAliasBranch;
		static const _TCHAR *m_cszAliasGenerator;
	};
}

