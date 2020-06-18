#pragma once
#include "Header.h"
#include "DeviceTypes.h"

namespace DFW2
{
	struct VariableIndex
	{
		double Value;
		ptrdiff_t Index = 0;
		constexpr operator double& () { return Value; }
		constexpr operator const double& () const { return Value; }
		constexpr double& operator= (double value) { Value = value;  return Value; }
	};

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
	using VARINDEXMAP = std::map<std::wstring, CVarIndex>;
	using VARINDEXMAPITR = VARINDEXMAP::iterator;
	using VARINDEXMAPCONSTITR = VARINDEXMAP::const_iterator;

	// карта индексов констант
	using CONSTVARINDEXMAP = std::map<std::wstring, CConstVarIndex>;
	using CONSTVARINDEXMAPITR = CONSTVARINDEXMAP::iterator;
	using CONSTVARINDEXMAPCONSTITR = CONSTVARINDEXMAP::const_iterator;

	// множество типов устройств
	using TYPEINFOSET = std::set<ptrdiff_t>;
	using TYPEINFOSETITR = TYPEINFOSET::iterator;

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

	// данные о ссылках устройства хранятся в карте
	using LINKSFROMMAP = std::map<eDFW2DEVICETYPE, LinkDirectionFrom>;
	using LINKSTOMAP = std::map<eDFW2DEVICETYPE, LinkDirectionTo>;
	// для ускорения обработки ссылок разделенных по иерархии и направлениям
	// используются те же карты, но с const указателями на second из основных карт ссылок
	using LINKSFROMMAPPTR = std::map<eDFW2DEVICETYPE, LinkDirectionFrom const *>;
	using LINKSTOMAPPTR = std::map<eDFW2DEVICETYPE, LinkDirectionTo const *>;

	// ссылки без разделения на направления
	using LINKSUNDIRECTED = std::vector<LinkDirectionFrom const *>;

	// атрибуты контейнера устройств
	// Атрибуты контейнера можно "наследовать" в рантайме - брать некий атрибут и изменять его для другого типа устройств,
	// а далее по нему специфицировать контейнер устройств

	class CDeviceContainerProperties
	{
	public:

		void SetType(eDFW2DEVICETYPE eDevType);
		void AddLinkTo(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency, const _TCHAR* cszstrIdField);
		void AddLinkFrom(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency);
		bool bNewtonUpdate = false;											// нужен ли контейнеру вызов NewtonUpdate. По умолчанию устройство не требует особой обработки итерации Ньютона 
		bool bCheckZeroCrossing = false;									// нужен ли контейнеру вызов ZeroCrossing
		bool bPredict = false;												// нужен ли контейнеру вызов предиктора
		bool bVolatile = false;												// устройства в контейнере могут создаваться и удаляться динамически во время расчета
		ptrdiff_t nPossibleLinksCount = 0;									// возможное для устройства в контейнере количество ссылок на другие устройства
		ptrdiff_t nEquationsCount = 0;										// количество уравнений устройства в контейнере
		eDFW2DEVICETYPE	eDeviceType = DEVTYPE_UNKNOWN;
		VARINDEXMAP m_VarMap;												// карта индексов перменных состояния
		CONSTVARINDEXMAP m_ConstVarMap;										// карта индексов констант
		TYPEINFOSET m_TypeInfoSet;											// набор типов устройства

		LINKSFROMMAP m_LinksFrom;
		LINKSTOMAP  m_LinksTo;

		LINKSFROMMAPPTR m_MasterLinksFrom;
		LINKSTOMAPPTR  m_MasterLinksTo;

		LINKSUNDIRECTED m_Masters, m_Slaves;
		STRINGLIST m_lstAliases;											// возможные псевдонимы типа устройства (типа "Node","node")

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

		void SetClassName(const _TCHAR* cszVerbalName, const _TCHAR* cszSystemName);
		const _TCHAR* GetVerbalClassName();
		const _TCHAR* GetSystemClassName();

	protected:
		std::wstring m_strClassName;										// имя типа устройства
		std::wstring m_strClassSysName;										// системное имя имя типа устройства для сериализации
	};
}

