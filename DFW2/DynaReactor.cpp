#include "stdafx.h"
#include "DynaReactor.h"
#include "DynaNode.h"
using namespace DFW2;


void CDynaReactor::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_REACTOR);
	props.SetClassName(CDeviceContainerProperties::m_cszNameReactor, CDeviceContainerProperties::m_cszSysNameReactor);
	props.bStoreStates = false;
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaReactor>>();
}



void CDynaReactor::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	AddStateProperty(Serializer);
	Serializer->AddProperty("Id", TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty("HeadNode", HeadNode, eVARUNITS::VARUNIT_UNITLESS);	// узел или узел начала
	Serializer->AddProperty("TailNode", TailNode, eVARUNITS::VARUNIT_UNITLESS);	// узел конца
	Serializer->AddProperty("ParallelBranch", ParrBranch, eVARUNITS::VARUNIT_UNITLESS);	// номер параллельной цепи
	Serializer->AddProperty(CSerializerBase::m_cszType, Type, eVARUNITS::VARUNIT_UNITLESS);	// тип (0 - шинный / 1 - линейный в начале / 2 - линейный в конце)
	Serializer->AddProperty("Placement", Placement, eVARUNITS::VARUNIT_UNITLESS);	// установка (0 - до выключателя / 1 - после выключателя )
	// "до выключателя" - при отключении ветви реактор уходит на шину. Реально он и так фактически на шине. Положение влияет на расчет потоков ветви
	// "после выключателя" - при отключении остается на ветви, видимо может шунтировать открытый конец
	Serializer->AddProperty(CDynaNodeBase::cszb_, b, eVARUNITS::VARUNIT_SIEMENS, -1.0);		// реактивная проводимость
	Serializer->AddProperty(CDynaNodeBase::cszg_, g, eVARUNITS::VARUNIT_SIEMENS);		// активная проводимость
}
