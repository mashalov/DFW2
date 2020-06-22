#include "stdafx.h"
#include "DeviceContainerProperties.h"
using namespace DFW2;


const _TCHAR* CDeviceContainerProperties::GetVerbalClassName() const
{
	return m_strClassName.c_str();
}
const _TCHAR* CDeviceContainerProperties::GetSystemClassName() const
{
	return m_strClassSysName.c_str();
}

eDFW2DEVICETYPE CDeviceContainerProperties::GetType() const
{
	return eDeviceType;
}

const _TCHAR *CDeviceContainerProperties::m_cszNameGenerator1C = _T("Генератор 1К");
const _TCHAR *CDeviceContainerProperties::m_cszNameGenerator3C = _T("Генератор 3К");
const _TCHAR *CDeviceContainerProperties::m_cszNameGeneratorMustang = _T("Генератор Mustang");
const _TCHAR *CDeviceContainerProperties::m_cszNameGeneratorInfPower = _T("ШБМ");
const _TCHAR *CDeviceContainerProperties::m_cszNameGeneratorMotion = _T("Генератор УД");
const _TCHAR *CDeviceContainerProperties::m_cszNameExciterMustang = _T("Возбудитель Mustang");
const _TCHAR *CDeviceContainerProperties::m_cszNameExcConMustang = _T("АРВ Мустанг");
const _TCHAR *CDeviceContainerProperties::m_cszNameDECMustang = _T("Форсировка Мустанг");
const _TCHAR *CDeviceContainerProperties::m_cszNameNode = _T("Узел");
const _TCHAR *CDeviceContainerProperties::m_cszNameBranch = _T("Ветвь");
const _TCHAR *CDeviceContainerProperties::m_cszNameBranchMeasure = _T("Измерения ветви");

const _TCHAR *CDeviceContainerProperties::m_cszSysNameGenerator1C = _T("Generator1C");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameGenerator3C = _T("Generator3C");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameGeneratorMustang = _T("GeneratorMustang");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameGeneratorInfPower = _T("GeneratorInfBus");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameGeneratorMotion = _T("GeneratorMotion");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameExciterMustang = _T("ExciterMustang");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameExcConMustang = _T("ExcControlMustang");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameDECMustang = _T("DecMustang");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameNode = _T("Node");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameBranch = _T("Branch");
const _TCHAR *CDeviceContainerProperties::m_cszSysNameBranchMeasure = _T("BranchMeasure");

const _TCHAR *CDeviceContainerProperties::m_cszAliasNode = _T("node");
const _TCHAR *CDeviceContainerProperties::m_cszAliasBranch = _T("vetv");
const _TCHAR *CDeviceContainerProperties::m_cszAliasGenerator = _T("Generator");
