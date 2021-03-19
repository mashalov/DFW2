#include "stdafx.h"
#include "Serializer.h"
#include "DeviceContainer.h"
using namespace DFW2;

const char* CSerializerBase::GetClassName()
{
	if (m_pDevice)
		return m_pDevice->GetContainer()->m_ContainerProps.GetSystemClassName();
	else
		return m_strClassName.c_str();
}
