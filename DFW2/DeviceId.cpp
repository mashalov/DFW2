#include "stdafx.h"
#include "DeviceId.h"

using namespace DFW2;

// конструктор без параметров
// по умолчанию Id и индекс в БД не установлены, что соответствует отрицательным значениям
CDeviceId::CDeviceId() {}

// конструктор с определением Id
CDeviceId::CDeviceId(ptrdiff_t nId) : Id_(nId)
{
}

const char* CDeviceId::GetVerbalName() const noexcept
{
	return VerbalName_.c_str();
		
}

// базовая функция собирает имя из простого имени и идентификатора
void CDeviceId::UpdateVerbalName()
{
	// если имя не пустое, формируется подробное имя в формате Id - [Имя]
	// если имя пустое - просто Id
	VerbalName_ = (Name_.length() ? fmt::format("{}-[{}]", Id_, Name_) : fmt::format("{}", Id_));
}

void CDeviceId::SetName(std::string_view Name)
{
	// при изменении имени перестраиваем подробное имя
	Name_ = Name;
	UpdateVerbalName();
}

void CDeviceId::SetDBIndex(ptrdiff_t nIndex) noexcept
{
	DBIndex_ = nIndex;
}

ptrdiff_t CDeviceId::GetDBIndex() const noexcept
{
	return DBIndex_;
}


ptrdiff_t CDeviceId::GetId() const noexcept
{
	return Id_;
}

const char* CDeviceId::GetName() const noexcept
{
	return Name_.c_str();
}

void CDeviceId::SetId(ptrdiff_t nId)
{
	// при изменении идентификатора перестраиваем подробное имя
	// эта функция может нарушить сортировку в контейнере !
	// не вызывать после сборки контейнера !
	Id_ = nId;
	UpdateVerbalName();
}