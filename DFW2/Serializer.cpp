#include "stdafx.h"
#include "Serializer.h"
#include "DeviceContainer.h"
using namespace DFW2;

const _TCHAR* CSerializerBase::m_cszDupName = _T("CSerializerBase::AddProperty duplicated name \"%s\"");
const _TCHAR* CSerializerAdapterBase::cszNotImplemented = _T("CSerializerAdapterBase: - Not implemented");
const _TCHAR* CSerializerBase::m_cszState = _T("state");
const _TCHAR* CSerializerBase::m_cszV = _T("v");
const _TCHAR* CSerializerBase::m_cszStateCause = _T("cause");
const _TCHAR* CSerializerBase::m_cszType = _T("type");
const _TCHAR* eValueDecs[];

const _TCHAR* TypedSerializedValue::m_cszTypeDecs[8] =
{
	_T("double"),
	_T("int"),
	_T("bool"),
	_T("complex"),
	_T("name"),
	CSerializerBase::m_cszState,
	_T("id"),
	_T("adapter")
};

const _TCHAR* CSerializerBase::GetClassName()
{
	if (m_pDevice)
		return m_pDevice->GetContainer()->m_ContainerProps.GetSystemClassName();
	else
		return m_strClassName.c_str();
}
