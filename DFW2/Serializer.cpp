#include "stdafx.h"
#include "Serializer.h"
#include "DeviceContainer.h"
using namespace DFW2;

const char* CSerializerBase::m_cszDupName = "CSerializerBase::AddProperty duplicated name \"{}\"";
const char* CSerializerAdapterBase::cszNotImplemented = "CSerializerAdapterBase: - Not implemented";
const char* CSerializerBase::m_cszState = "state";
const char* CSerializerBase::m_cszV = "v";
const char* CSerializerBase::m_cszStateCause = "cause";
const char* CSerializerBase::m_cszType = "type";

const char* TypedSerializedValue::m_cszTypeDecs[8] =
{
	"double",
	"int",
	"bool",
	"complex",
	"name",
	CSerializerBase::m_cszState,
	"id",
	"adapter"
};

const char* CSerializerBase::GetClassName()
{
	if (m_pDevice)
		return m_pDevice->GetContainer()->m_ContainerProps.GetSystemClassName();
	else
		return m_strClassName.c_str();
}
