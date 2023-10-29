#include "stdafx.h"
#include "DynaNode.h"
#include "SVC.h"
#include "DynaModel.h"
using namespace DFW2;


void CSVC::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaPowerInjector::DeviceProperties(props);
	props.SetType(DEVTYPE_SVC);
	props.SetClassName(CDeviceContainerProperties::m_cszNameSVC, CDeviceContainerProperties::m_cszSysNameSVC);
	props.EquationsCount = CSVC::VARS::V_LAST;
	props.DeviceFactory = std::make_unique<CDeviceFactory<CSVC>>();
}

void CSVC::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaPowerInjector::UpdateSerializer(Serializer);
	Serializer->AddProperty(cszDroop_, Droop_, eVARUNITS::VARUNIT_PERCENT, 0.01);
	Serializer->AddProperty(CDynaPowerInjector::m_cszUnom, Unom, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty(CDynaPowerInjector::m_cszQnom, Qnom_, eVARUNITS::VARUNIT_MVAR);
}

void CSVC::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaPowerInjector::UpdateValidator(Validator);
	Validator->AddRule(CDynaPowerInjector::m_cszUnom, &CDynaPowerInjector::ValidatorUnom);
	Validator->AddRule(CDynaPowerInjector::m_cszQnom, &CSerializerValidatorRules::BiggerThanZero);
}