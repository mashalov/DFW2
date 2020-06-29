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

	// ���� ���������� ����������
	enum eDEVICEVARIABLETYPE
	{
		eDVT_CONSTSOURCE,			// ��������� �������� ������
		eDVT_INTERNALCONST,			// ��������� �������������� ������ ���������� � Init
		eDVT_STATE					// ���������� ���������, ��� ������� ������ ���� ���������
	};

	// ������� ����� �������� ����������
	// �������� ������ ���������� � ����������
	class CVarIndexBase
	{
	public:
		ptrdiff_t m_nIndex;
		CVarIndexBase(ptrdiff_t nIndex) : m_nIndex(nIndex) { };
	};

	// �������� ���������� ���������
	class CVarIndex : public CVarIndexBase
	{
	public:
		bool m_bOutput;														// ������� ������ ���������� � ����������
		double m_dMultiplier;												// ��������� ����������
		eVARUNITS m_Units;													// ������� ��������� ����������
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

	// �������� ���������
	class CConstVarIndex : public CVarIndexBase
	{
	public:
		eDEVICEVARIABLETYPE m_DevVarType;
		CConstVarIndex(ptrdiff_t nIndex, eDEVICEVARIABLETYPE eDevVarType) : CVarIndexBase(nIndex),
			m_DevVarType(eDevVarType)
		{

		}
	};

	// �������� ������� ����������
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


	// ����� �������� ���������� ���������
	using VARINDEXMAP = std::map<std::wstring, CVarIndex>;
	using VARINDEXMAPITR = VARINDEXMAP::iterator;
	using VARINDEXMAPCONSTITR = VARINDEXMAP::const_iterator;

	// ����� �������� ��������
	using CONSTVARINDEXMAP = std::map<std::wstring, CConstVarIndex>;
	using CONSTVARINDEXMAPITR = CONSTVARINDEXMAP::iterator;
	using CONSTVARINDEXMAPCONSTITR = CONSTVARINDEXMAP::const_iterator;

	// ����� �������� ������� ����������
	using EXTVARINDEXMAP = std::map<std::wstring, CExtVarIndex>;
	using EXTVARINDEXMAPITR = EXTVARINDEXMAP::iterator;
	using EXTVARINDEXMAPCONSTITR = EXTVARINDEXMAP::const_iterator;

	// ��������� ����� ���������
	using TYPEINFOSET = std::set<ptrdiff_t>;
	using TYPEINFOSETITR = TYPEINFOSET::iterator;

	using DOUBLEVECTOR = std::vector<double>;
	using VARIABLEVECTOR = std::vector<VariableIndex>;
	using EXTVARIABLEVECTOR = std::vector<VariableIndexExternal>;
	using PRIMITIVEVECTOR = std::vector<PrimitiveDescription>;

	// ����� _��_ �������� ����������
	struct LinkDirectionFrom
	{
		ptrdiff_t nLinkIndex;												// ������ �����
		eDFW2DEVICELINKMODE eLinkMode;										// ����� ��������
		eDFW2DEVICEDEPENDENCY eDependency;									// ����� ���������� ��������� � �����
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

	// ������ � ������� ���������� �������� � �����
	using LINKSFROMMAP = std::map<eDFW2DEVICETYPE, LinkDirectionFrom>;
	using LINKSTOMAP = std::map<eDFW2DEVICETYPE, LinkDirectionTo>;

	// ������� ���������� ������� ����������
	enum class eDEVICEFUNCTIONSTATUS
	{
		DFS_OK,							// OK
		DFS_NOTREADY,					// ���� ��������� (���� �����-�� ����������� ��������� ��������� ��� ������������ �������)
		DFS_DONTNEED,					// ������� ��� ������� ���������� �� �����
		DFS_FAILED						// ������
	};

	// ������� ��������� ����������
	enum class eDEVICESTATE
	{
		DS_OFF,							// ��������� ���������
		DS_ON,							// ��������
		DS_READY,						// ������ (�� ������������ ?)
		DS_DETERMINE,					// ������ ���� ���������� (������-�����������, ��������)
	};

	// ������� ��������� ��������� ����������
	enum class eDEVICESTATECAUSE
	{
		DSC_EXTERNAL,					// ��������� �������� ������� ����������
		DSC_INTERNAL,					// ��������� �������� ��������� ������ ����������
		DSC_INTERNAL_PERMANENT			// ��������� �������� ��������� ���������� � �� ����� ���� �������� ��� ���
	};
}