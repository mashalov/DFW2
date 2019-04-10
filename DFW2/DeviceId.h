#pragma once
#include "header.h"
#include "DeviceTypes.h"

using namespace std;

namespace DFW2
{
	// ����� ��� ������������� ���������. �������� ������� ��� ���� ���������
	class CDeviceId
	{
	protected:
		ptrdiff_t m_Id;							// ������������� (��� ���������������������, ���� ������ - ������ ������)
		wstring m_strName;						// ��� ���������� ��� ������������. ���� ��� ����
		ptrdiff_t m_DBIndex;					// ������ ���������� �� ������� ��
		wstring m_strVerbalName;				// ��� ���������� � ������������� - � ����� � �.�.
		virtual void UpdateVerbalName();		// ������� ���������� ���������� ����� ����������, ��� ��������� ���� � �.�.
	public:
		CDeviceId();
		CDeviceId(ptrdiff_t nId);
		virtual ~CDeviceId() {}
		ptrdiff_t GetId() const;				// �������� �������������
		void SetId(ptrdiff_t nId);				// ������ �������������
		const _TCHAR* GetName() const;			// �������� ��� ��� ������������
		void SetName(const _TCHAR* cszName);	// ������ ��� ��� ������������
		const _TCHAR* GetVerbalName() const;	// �������� ��� � �������������
		ptrdiff_t GetDBIndex();					// �������� ������ � ��
		void SetDBIndex(ptrdiff_t nIndex);		// ������ ������ � ��
	};

	// �������� ��� ���������� � ������ ���������. ������������� �� GetId
	struct CDeviceIdComparator
	{
		bool operator()(const CDeviceId* lhs, const CDeviceId* rhs) const
		{
			return lhs->GetId() < rhs->GetId();
		}
	};
}
