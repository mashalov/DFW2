#include "stdafx.h"
#include "DeviceContainerProperties.h"
using namespace DFW2;


const char* CDeviceContainerProperties::GetVerbalClassName() const noexcept
{
	return ClassName_.c_str();
}
const char* CDeviceContainerProperties::GetSystemClassName() const noexcept
{
	return ClassSysName_.c_str();
}

eDFW2DEVICETYPE CDeviceContainerProperties::GetType() const noexcept
{
	return eDeviceType;
}



const char* CDeviceContainerProperties::m_cszNameGenerator1C = "Генератор 1К";
const char* CDeviceContainerProperties::m_cszNameGenerator2C = "Генератор 2К";
const char* CDeviceContainerProperties::m_cszNameGenerator3C = "Генератор 3К";
const char* CDeviceContainerProperties::m_cszNameGeneratorPark3C = "Генератор Парк 3К";
const char* CDeviceContainerProperties::m_cszNameGeneratorPark4C = "Генератор Парк 4К";
const char* CDeviceContainerProperties::m_cszNameGeneratorMustang = "Генератор Mustang";
const char* CDeviceContainerProperties::m_cszNameGeneratorInfPower = "ШБМ";
const char* CDeviceContainerProperties::m_cszNameGeneratorPowerInjector = "Генератор-инъекция";
const char* CDeviceContainerProperties::m_cszNameGeneratorMotion = "Генератор УД";
const char* CDeviceContainerProperties::m_cszNameExciterMustang = "Возбудитель Mustang";
const char* CDeviceContainerProperties::m_cszNameExcConMustang = "АРВ Мустанг";
const char* CDeviceContainerProperties::m_cszNameDECMustang = "Форсировка Мустанг";
const char* CDeviceContainerProperties::m_cszNameNode = "Узел";
const char* CDeviceContainerProperties::m_cszNameBranch = "Ветвь";
const char* CDeviceContainerProperties::m_cszNameBranchMeasure = "Измерения ветви";
const char* CDeviceContainerProperties::m_cszNameNodeMeasure = "Измерения узла";
const char* CDeviceContainerProperties::m_cszNameZeroLoadFlow = "Потокораспределение суперузлов";
const char* CDeviceContainerProperties::m_cszNameLRC = "СХН";
const char* CDeviceContainerProperties::m_cszNameReactor = "Реактор";
const char* CDeviceContainerProperties::m_cszNameSVC = "УШР";
const char* CDeviceContainerProperties::m_cszNameSVCDEC = "УШР с форсировкой";

const char* CDeviceContainerProperties::m_cszSysNameGenerator1C = "Generator1C";
const char* CDeviceContainerProperties::m_cszSysNameGenerator2C = "Generator2C";
const char* CDeviceContainerProperties::m_cszSysNameGenerator3C = "Generator3C";
const char* CDeviceContainerProperties::m_cszSysNameGeneratorPark3C = "GeneratorPark3C";
const char* CDeviceContainerProperties::m_cszSysNameGeneratorPark4C = "GeneratorPark4C";
const char* CDeviceContainerProperties::m_cszSysNameGeneratorMustang = "GeneratorMustang";
const char* CDeviceContainerProperties::m_cszSysNameGeneratorInfPower = "GeneratorInfBus";
const char* CDeviceContainerProperties::m_cszSysNameGeneratorPowerInjector = "GeneratorPowerInjector";
const char* CDeviceContainerProperties::m_cszSysNameGeneratorMotion = "GeneratorMotion";
const char* CDeviceContainerProperties::m_cszSysNameExciterMustang = "ExciterMustang";
const char* CDeviceContainerProperties::m_cszSysNameExcConMustang = "ExcControlMustang";
const char* CDeviceContainerProperties::m_cszSysNameDECMustang = "DecMustang";
const char* CDeviceContainerProperties::m_cszSysNameNode = "Node";
const char* CDeviceContainerProperties::m_cszSysNameBranch = "Branch";
const char* CDeviceContainerProperties::m_cszSysNameBranchMeasure = "BranchMeasure";
const char* CDeviceContainerProperties::m_cszSysNameNodeMeasure = "NodeMeasure";
const char* CDeviceContainerProperties::m_cszSysNameZeroLoadFlow = "ZeroLoadFlow";
const char* CDeviceContainerProperties::m_cszSysNameLRC = "LRC";
const char* CDeviceContainerProperties::m_cszSysNameReactor = "Reactor";
const char* CDeviceContainerProperties::m_cszSysNameSVC = "SVC";
const char* CDeviceContainerProperties::m_cszSysNameSVCDEC = "SVCDEC";

const char* CDeviceContainerProperties::m_cszAliasNode = "node";
const char* CDeviceContainerProperties::m_cszAliasBranch = "vetv";
const char* CDeviceContainerProperties::m_cszAliasGenerator = "Generator";
