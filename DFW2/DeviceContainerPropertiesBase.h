#pragma once
#include "DataStructs.h"

namespace DFW2
{
	// �������� ���������� ���������
	// �������� ���������� ����� "�����������" � �������� - ����� ����� ������� � �������� ��� ��� ������� ���� ���������,
	// � ����� �� ���� ��������������� ��������� ���������

	class CDeviceContainerPropertiesBase
	{
	public:
		// �������� ��� ����������, ��� ���, �� �������� ���������� �����������
		void SetType(eDFW2DEVICETYPE eDevType)
		{
			m_TypeInfoSet.insert(eDeviceType = eDevType);
		}
		void SetClassName(const _TCHAR* cszVerbalName, const _TCHAR* cszSystemName)
		{
			m_strClassName = cszVerbalName;
			m_strClassSysName = cszSystemName;
		}

		// �������� ����������� ����� �� ����������
		// ��������� ��� ����������, ����� ����� � ��� �����������. ����� ����� � ��� ����������� ����������� ��� �������� ����������
		// ����� ���� ������� ��� ���������� ��������, ��� ��������� ������ ��������� ����� �� ��������� ���� ���������, ������ ������� ��� ���������� ������� ��� �����������
		// ������ ������� ����� ����� ����������� ��������� ��� ��� ��������� �� ������� ����������, � ������� ����� ����� �� ����� ����������
		// ����� ����������� ���� ����� a � b ������ ���� ��������� - a.From b, b.To a, � ����������� �� ����, � ����� ���������� ���� ��������� �������������� ��� �����
		// eDFW2DEVICEDEPENDENCY ������ ���� �������������� - Master - Slave.
		void AddLinkTo(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency, const _TCHAR* cszstrIdField)
		{
			if (m_LinksTo.find(eDevType) == m_LinksTo.end())
			{
				m_LinksTo.insert(std::make_pair(eDevType, LinkDirectionTo(eLinkMode, Dependency, m_LinksTo.size() + m_LinksFrom.size(), cszstrIdField)));
				if (eLinkMode == DLM_SINGLE)
					nPossibleLinksCount++;
			}
		}

		// �������� ����������� ����� �� ����������
		// ��������� ��� ����������, ����� ����� � ��� �����������. ����� ����� � ��� ����������� ����������� ��� �������� ����������
		void AddLinkFrom(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency)
		{
			if (m_LinksFrom.find(eDeviceType) == m_LinksFrom.end())
			{
				// ���� ����� � ������ ����� ���������� ��� ���
				// ��������� ��
				m_LinksFrom.insert(std::make_pair(eDevType, LinkDirectionFrom(eLinkMode, Dependency, m_LinksTo.size() + m_LinksFrom.size())));
				if (eLinkMode == DLM_SINGLE)
					nPossibleLinksCount++;
			}
		}

		bool bNewtonUpdate = false;											// ����� �� ���������� ����� NewtonUpdate. �� ��������� ���������� �� ������� ������ ��������� �������� ������� 
		bool bCheckZeroCrossing = false;									// ����� �� ���������� ����� ZeroCrossing
		bool bPredict = false;												// ����� �� ���������� ����� ����������
		bool bVolatile = false;												// ���������� � ���������� ����� ����������� � ��������� ����������� �� ����� �������
		ptrdiff_t nPossibleLinksCount = 0;									// ��������� ��� ���������� � ���������� ���������� ������ �� ������ ����������
		ptrdiff_t nEquationsCount = 0;										// ���������� ��������� ���������� � ����������
		eDFW2DEVICETYPE	eDeviceType = DEVTYPE_UNKNOWN;
		VARINDEXMAP m_VarMap;												// ����� �������� ��������� ���������
		CONSTVARINDEXMAP m_ConstVarMap;										// ����� �������� ��������
		EXTVARINDEXMAP m_ExtVarMap;											// ����� �������� ������� ����������
		TYPEINFOSET m_TypeInfoSet;											// ����� ����� ����������
		LINKSFROMMAP m_LinksFrom;
		LINKSTOMAP  m_LinksTo;
		std::list<std::wstring> m_lstAliases;								// ��������� ���������� ���� ���������� (���� "Node","node")
	protected:
		std::wstring m_strClassName;										// ��� ���� ����������
		std::wstring m_strClassSysName;										// ��������� ��� ��� ���� ���������� ��� ������������
	};
}
