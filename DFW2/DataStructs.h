#pragma once
#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <variant>
#include <tchar.h>
#include "DeviceTypes.h"
#include "DLLStructs.h"

namespace DFW2
{
	struct VariableIndexBase
	{
		ptrdiff_t Index = -100000;
		constexpr bool Indexed() { return Index >= 0; }
	};

	struct VariableIndex : VariableIndexBase
	{
		double Value;
		constexpr operator double& () { return Value; }
		constexpr operator const double& () const { return Value; }
		constexpr double& operator= (double value) { Value = value;  return Value; }
	};

	struct VariableIndexExternal : VariableIndexBase
	{
		double* pValue = nullptr;
		constexpr operator double& () { return *pValue; }
		constexpr operator const double& () const { return *pValue; }
		constexpr double& operator= (double value) { *pValue = value;  return *pValue; }
	};

	struct VariableIndexExternalOptional : VariableIndexExternal
	{
		double Value;
		void MakeLocal() { pValue = &Value; Index = -1; }
		constexpr double& operator= (double value) { *pValue = value;  return *pValue; }
	};

	using VariableIndexRefVec = std::vector<std::reference_wrapper<VariableIndex>>;
	using VariableIndexExternalRefVec = std::vector<std::reference_wrapper<VariableIndexExternal>>;

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

	// описание внешней переменной
	class CExtVarIndex : public CVarIndexBase
	{
	public:
		eDFW2DEVICETYPE m_DeviceToSearch;
		CExtVarIndex(ptrdiff_t nIndex, eDFW2DEVICETYPE eDeviceToSearch) : CVarIndexBase(nIndex),
			m_DeviceToSearch(eDeviceToSearch)
		{

		}


	};

	using VariableVariant = std::variant < const std::reference_wrapper<VariableIndex>,
										   const std::reference_wrapper<VariableIndexExternal>>;

	struct PrimitivePin
	{
		ptrdiff_t nPin;
		VariableVariant Variable;
		PrimitivePin(ptrdiff_t Pin, VariableIndex& vi) : nPin(Pin), Variable(vi) {}
		PrimitivePin(ptrdiff_t Pin, VariableIndexExternal& vi) : nPin(Pin), Variable(vi) {}
		PrimitivePin& operator = (const PrimitivePin& pin) { return *this; }
	};

	using PrimitivePinVec = std::vector<PrimitivePin>;
	class PrimitiveDescription
	{
	public:
		PrimitiveBlockType eBlockType;
		std::wstring_view Name;
		PrimitivePinVec Outputs; 
		PrimitivePinVec Inputs;
	};


	// карта индексов переменных состояния
	using VARINDEXMAP = std::map<std::wstring, CVarIndex>;
	using VARINDEXMAPITR = VARINDEXMAP::iterator;
	using VARINDEXMAPCONSTITR = VARINDEXMAP::const_iterator;

	// карта индексов констант
	using CONSTVARINDEXMAP = std::map<std::wstring, CConstVarIndex>;
	using CONSTVARINDEXMAPITR = CONSTVARINDEXMAP::iterator;
	using CONSTVARINDEXMAPCONSTITR = CONSTVARINDEXMAP::const_iterator;

	// карта индексов внешних переменных
	using EXTVARINDEXMAP = std::map<std::wstring, CExtVarIndex>;
	using EXTVARINDEXMAPITR = EXTVARINDEXMAP::iterator;
	using EXTVARINDEXMAPCONSTITR = EXTVARINDEXMAP::const_iterator;

	// множество типов устройств
	using TYPEINFOSET = std::set<ptrdiff_t>;
	using TYPEINFOSETITR = TYPEINFOSET::iterator;

	using DOUBLEVECTOR = std::vector<double>;
	using VARIABLEVECTOR = std::vector<VariableIndex>;
	using EXTVARIABLEVECTOR = std::vector<VariableIndexExternal>;
	using PRIMITIVEVECTOR = std::vector<PrimitiveDescription>;

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
		LinkDirectionTo(eDFW2DEVICELINKMODE LinkMode, eDFW2DEVICEDEPENDENCY Dependency, ptrdiff_t LinkIndex, const _TCHAR* cszIdField) :
			LinkDirectionFrom(LinkMode, Dependency, LinkIndex),
			strIdField(cszIdField)
		{}
	};

	// данные о ссылках устройства хранятся в карте
	using LINKSFROMMAP = std::map<eDFW2DEVICETYPE, LinkDirectionFrom>;
	using LINKSTOMAP = std::map<eDFW2DEVICETYPE, LinkDirectionTo>;

	// статусы выполнения функций устройства
	enum class eDEVICEFUNCTIONSTATUS
	{
		DFS_OK,							// OK
		DFS_NOTREADY,					// Надо повторить (есть какая-то очередность обработки устройств или итерационный процесс)
		DFS_DONTNEED,					// Функция для данного устройства не нужна
		DFS_FAILED						// Ошибка
	};

	// статусы состояния устройства
	enum class eDEVICESTATE
	{
		DS_OFF,							// полностью отключено
		DS_ON,							// включено
		DS_READY,						// готово (не используется ?)
		DS_DETERMINE,					// должно быть определено (мастер-устройством, например)
	};

	// причина изменения состояния устройства
	enum class eDEVICESTATECAUSE
	{
		DSC_EXTERNAL,					// состояние изменено снаружи устройство
		DSC_INTERNAL,					// состояние изменено действием самого устройства
		DSC_INTERNAL_PERMANENT			// состояние изменено действием устройства и не может быть изменено еще раз
	};
}