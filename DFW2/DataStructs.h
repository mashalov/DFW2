#pragma once
#include "vector"
#include "list"
#include "map"
#include "set"
#include "string"
#include "tchar.h"
#include "DeviceTypes.h"

namespace DFW2
{
	struct VariableIndexBase
	{
		ptrdiff_t Index = 0;
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
		double* m_pValue = nullptr;
		constexpr operator double& () { return *m_pValue; }
		constexpr operator const double& () const { return *m_pValue; }
		constexpr double& operator= (double value) { *m_pValue = value;  return *m_pValue; }
	};

	struct VariableIndexExternalOptional : VariableIndexExternal
	{
		double Value;
		void MakeLocal() { m_pValue = &Value; Index = -1; }
		constexpr double& operator= (double value) { *m_pValue = value;  return *m_pValue; }
	};

	using VariableIndexVec = std::vector<std::reference_wrapper<VariableIndex>>;

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

	// ����� �������� ���������� ���������
	using VARINDEXMAP = std::map<std::wstring, CVarIndex>;
	using VARINDEXMAPITR = VARINDEXMAP::iterator;
	using VARINDEXMAPCONSTITR = VARINDEXMAP::const_iterator;

	// ����� �������� ��������
	using CONSTVARINDEXMAP = std::map<std::wstring, CConstVarIndex>;
	using CONSTVARINDEXMAPITR = CONSTVARINDEXMAP::iterator;
	using CONSTVARINDEXMAPCONSTITR = CONSTVARINDEXMAP::const_iterator;

	// ��������� ����� ���������
	using TYPEINFOSET = std::set<ptrdiff_t>;
	using TYPEINFOSETITR = TYPEINFOSET::iterator;

	using DOUBLEVECTOR = std::vector<double>;

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
	// ��� ��������� ��������� ������ ����������� �� �������� � ������������
	// ������������ �� �� �����, �� � const ����������� �� second �� �������� ���� ������
	using LINKSFROMMAPPTR = std::map<eDFW2DEVICETYPE, LinkDirectionFrom const*>;
	using LINKSTOMAPPTR = std::map<eDFW2DEVICETYPE, LinkDirectionTo const*>;

	// ������ ��� ���������� �� �����������
	using LINKSUNDIRECTED = std::vector<LinkDirectionFrom const*>;
}