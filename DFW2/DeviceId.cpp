#include "stdafx.h"
#include "DeviceId.h"

using namespace DFW2;

// конструктор без параметров
// по умолчанию Id и индекс в БД не установлены, что соответствует отрицательным значениям
CDeviceId::CDeviceId() {}

// конструктор с определением Id
CDeviceId::CDeviceId(ptrdiff_t nId) : m_Id(nId)
{
}

const char* CDeviceId::GetVerbalName() const
{
	return m_strVerbalName.c_str();
		
}

// базовая функция собирает имя из простого имени и идентификатора
void CDeviceId::UpdateVerbalName()
{
	// если имя не пустое, формируется подробное имя в формате Id - [Имя]
	// если имя пустое - просто Id
	m_strVerbalName = (m_strName.length() ? fmt::format("{}-[{}]", m_Id, m_strName) : fmt::format("{}", m_Id));
}

void CDeviceId::SetName(std::string_view Name)
{
	// при изменении имени перестраиваем подробное имя
	m_strName = Name;
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

const char* CDeviceId::GetName() const
{
	return m_strName.c_str();
}

void CDeviceId::SetId(ptrdiff_t nId)
{
	// при изменении идентификатора перестраиваем подробное имя
	// эта функция может нарушить сортировку в контейнере !
	// не вызывать после сборки контейнера !
	m_Id = nId;
	UpdateVerbalName();
}