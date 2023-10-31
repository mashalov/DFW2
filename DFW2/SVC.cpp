#include "stdafx.h"
#include "DynaNode.h"
#include "SVC.h"
#include "DynaModel.h"
using namespace DFW2;


eDEVICEFUNCTIONSTATUS CSVC::PreInit(CDynaModel* pDynaModel)
{
	const double Unom2{ Unom * Unom };
	xsl_ = Droop_ * Unom2 / Qnom_;
	Bmin_ = LFQmin / Unom2;
	Bmax_ = LFQmax / Unom2;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

const CSVC::Output& CSVC::B(const CDynaNodeBase& pNode)
{
	Output_.Bunlimited =(pNode.V - Vref_) / xsl_ / pNode.V;

	if (Output_.Bunlimited > Bmax_)
	{
		Output_.Blimited = Bmax_;
		Output_.State = CDynaPrimitiveLimited::eLIMITEDSTATES::Max;
	}
	else if (Output_.Bunlimited < Bmin_)
	{
		Output_.Blimited = Bmin_;
		Output_.State = CDynaPrimitiveLimited::eLIMITEDSTATES::Min;
	}
	else
	{
		Output_.Blimited = Output_.Bunlimited;
		Output_.State = CDynaPrimitiveLimited::eLIMITEDSTATES::Mid;
	}

	const double V2{ pNode.V * pNode.V };

	Q = -(Output_.Qlimited = Output_.Blimited * V2);
	Output_.Qunlimited = Output_.Bunlimited * V2;

	return Output_;
} 


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
	Serializer->AddProperty(CDynaPowerInjector::cszVref_, Vref_, eVARUNITS::VARUNIT_KVOLTS);
}

void CSVC::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaPowerInjector::UpdateValidator(Validator);
	Validator->AddRule(CDynaPowerInjector::m_cszUnom, &CDynaPowerInjector::ValidatorUnom);
	Validator->AddRule(CDynaPowerInjector::m_cszQnom, &CSerializerValidatorRules::BiggerThanZero);
}