#pragma once
#include "Header.h"
#include "DeviceTypes.h"

namespace DFW2
{
	// типы переменных устройства
	enum eDEVICEVARIABLETYPE
	{
		eDVT_CONSTSOURCE,			// константа исходных данных
		eDVT_INTERNALCONST,			// константа рассчитываемая внутри устройства в Init
		eDVT_STATE					// переменная состояния, для которой должно быть уравнение
	};

	// базовый класс описания переменной
	// содержит индекс переменной в устройстве
	class CVarIndexBase
	{
	public:
		ptrdiff_t m_nIndex;
		CVarIndexBase(ptrdiff_t nIndex) : m_nIndex(nIndex) { };
	};

	// описание переменной состояния
	class CVarIndex : public CVarIndexBase
	{
	public:
		bool m_bOutput;														// признак вывода переменной в результаты
		double m_dMultiplier;												// множитель переменной
		eVARUNITS m_Units;													// единицы измерения переменной
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

	// описание константы
	class CConstVarIndex : public CVarIndexBase
	{
	public:
		eDEVICEVARIABLETYPE m_DevVarType;
		CConstVarIndex(ptrdiff_t nIndex, eDEVICEVARIABLETYPE eDevVarType) : CVarIndexBase(nIndex),
			m_DevVarType(eDevVarType)
		{

		}
	};

	// карта индексов переменных состояния
	typedef std::map<std::wstring, CVarIndex> VARINDEXMAP;
	typedef VARINDEXMAP::iterator VARINDEXMAPITR;
	typedef VARINDEXMAP::const_iterator VARINDEXMAPCONSTITR;

	// карта индексов констант
	typedef std::map<std::wstring, CConstVarIndex> CONSTVARINDEXMAP;
	typedef CONSTVARINDEXMAP::iterator CONSTVARINDEXMAPITR;
	typedef CONSTVARINDEXMAP::const_iterator CONSTVARINDEXMAPCONSTITR;

	// множество типов устройств
	typedef std::set<ptrdiff_t> TYPEINFOSET;
	typedef TYPEINFOSET::iterator TYPEINFOSETITR;

	// связь _от_ внешнего устройства
	struct LinkDirectionFrom
	{
		ptrdiff_t nLinkIndex;												// индекс связи
		eDFW2DEVICELINKMODE eLinkMode;										// режим линковки
		eDFW2DEVICEDEPENDENCY eDependency;									// режим подчинения устройств в связи
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

	// атрибуты контейнера устройств
	// Атрибуты контейнера можно "наследовать" в рантайме - брать некий атрибут и изменять его для другого типа устройств,
	// а далее по нему специфицировать контейнер устройств

	class CDeviceContainerProperties
	{
	public:
		CDeviceContainerProperties();
		~CDeviceContainerProperties();

		void SetType(eDFW2DEVICETYPE eDevType);
		void AddLinkTo(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency, const _TCHAR* cszstrIdField);
		void AddLinkFrom(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency);
		bool bNewtonUpdate;													// нужен ли контейнеру вызов NewtonUpdate 
		bool bCheckZeroCrossing;											// нужен ли контейнеру вызов ZeroCrossing
		bool bPredict;														// нужен ли контейнеру вызов предиктора
		ptrdiff_t nPossibleLinksCount;
		ptrdiff_t nEquationsCount;											// количество уравнений устройства в контейнере
		eDFW2DEVICETYPE		eDeviceType;
		VARINDEXMAP m_VarMap;												// карта индексов перменных состояния
		CONSTVARINDEXMAP m_ConstVarMap;										// карта индексов констант
		TYPEINFOSET m_TypeInfoSet;											// набор типов устройства

		LINKSFROMMAP m_LinksFrom;
		LINKSTOMAP  m_LinksTo;

		LINKSFROMMAP m_MasterLinksFrom;
		LINKSTOMAP  m_MasterLinksTo;

		std::wstring m_strClassName;										// имя типа устройства

		STRINGLIST m_lstAliases;											// возможные псевдонимы типа устройства (типа "Node","node")

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

