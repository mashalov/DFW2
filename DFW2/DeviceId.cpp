#include "stdafx.h"
#include "DeviceId.h"

using namespace DFW2;

// конструктор без параметров
// по умолчанию Id и индекс в Ѕƒ не установлены, что соответствует отрицательным значени€м
CDeviceId::CDeviceId() : m_Id(-1), m_DBIndex(-1)
{

}
// конструктор с определением Id
CDeviceId::CDeviceId(ptrdiff_t nId) : m_Id(nId)
{
}

const _TCHAR* CDeviceId::GetVerbalName() const
{
	return m_strVerbalName.c_str();
		
}

// базова€ функци€ собирает им€ из простого имени и идентификатора
void CDeviceId::UpdateVerbalName()
{
	// если им€ не пустое, формируетс€ подробное им€ в формате Id - [»м€]
	// если им€ пустое - просто Id
	m_strVerbalName = static_cast<const _TCHAR*>(m_strName.length() ? Cex(_T("%d-[%s]"), m_Id, m_strName.c_str()) : Cex(_T("%d"), m_Id));
}

void CDeviceId::SetName(const _TCHAR* cszName)
{
	// при изменении имени перестраиваем подробное им€
	m_strName = cszName;
	UpdateVerbalName();
}

void CDeviceId::SetDBIndex(ptrdiff_t nIndex)
{
	m_DBIndex = nIndex;
}

ptrdiff_t CDeviceId::GetDBIndex()
{
	return m_DBIndex;
}


ptrdiff_t CDeviceId::GetId() const
{
	return m_Id;
}

const _TCHAR* CDeviceId::GetName() const
{
	return m_strName.c_str();
}

void CDeviceId::SetId(ptrdiff_t nId)
{
	// при изменении идентификатора перестраиваем подробное им€
	// эта функци€ может нарушить сортировку в контейнере !
	// не вызывать после сборки контейнера !
	m_Id = nId;
	UpdateVerbalName();
}