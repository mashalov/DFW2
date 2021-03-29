#include "stdafx.h"
#include "Serializer.h"
#include "DeviceContainer.h"
using namespace DFW2;


ptrdiff_t CSerializerDataSourceContainer::ItemsCount()
{
	return m_pContainer->Count();
}

bool CSerializerDataSourceContainer::NextItem()
{
	nItemIndex++;
	if (nItemIndex < ItemsCount())
		return true;
	else
		return false;
}

CDevice* CSerializerDataSourceContainer::GetDevice()
{
	if (m_pContainer->Count())
		return m_pContainer->GetDeviceByIndex(0);
	else
		return CSerializerDataSourceBase::GetDevice();
}

void CSerializerDataSourceContainer::UpdateSerializer(CSerializerBase* pSerializer)
{
	m_pContainer->GetDeviceByIndex(nItemIndex)->UpdateSerializer(pSerializer);
}

const char* CSerializerBase::GetClassName()
{
	if (m_pDevice)
		return m_pDevice->GetContainer()->m_ContainerProps.GetSystemClassName();
	else
		return m_strClassName.c_str();
}

std::string CSerializerBase::GetVariableName(TypedSerializedValue* pValue) const
{
	for (const auto [key, val] : ValueMap)
		if (val == pValue)
			return key;
	return CDFW2Messages::m_cszUnknown;
}

const SERIALIZERMAP CSerializerBase::GetUnsetValues() const
{
	SERIALIZERMAP outMap;
	for (const auto& [Name, Var] : ValueMap)
		if (!Var->bSet)
			outMap.insert(std::make_pair(Name, Var));
	return outMap;
}

/*bool CDeviceSerializer::NextItem()
{
	if (auto container = m_pDevice->GetContainer() ; container)
	{
		if (m_nDeviceIndex < static_cast<ptrdiff_t>(container->Count()))
		{
			m_nDeviceIndex++;
			m_pDevice = container->GetDeviceByIndex(m_nDeviceIndex);
			std::unique_ptr<CDeviceSerializer> serializer(this);
			m_pDevice->UpdateSerializer(serializer);
			serializer.release();
			return false;
		}
	}
	return false;
}*/

bool TypedSerializedValue::isSignificant()
{
	return true;

	switch (ValueType)
	{
	case eValueType::VT_DBL:
		if (*Value.pDbl == 0.0) return false;
		break;
	case eValueType::VT_INT:
		if (*Value.pInt == 0.0) return false;
		break;
	case eValueType::VT_CPLX:
		if (Value.pCplx->real() == 0.0 && Value.pCplx->imag() == 0.0) return false;
		break;
	}
	return true;
}


std::string_view TypedSerializedValue::GetTypeVerbByType(TypedSerializedValue::eValueType type)
{
	const ptrdiff_t ntype(static_cast<ptrdiff_t>(type));
	if (ntype >= 0 && ntype < static_cast<ptrdiff_t>(std::size(TypedSerializedValue::m_cszTypeDecs)))
		return TypedSerializedValue::m_cszTypeDecs[ntype];
	else
		return CDFW2Messages::m_cszUnknown;
}

std::string_view TypedSerializedValue::GetTypeVerb() const
{
	return TypedSerializedValue::GetTypeVerbByType(ValueType);
}

MetaSerializedValue* MetaSerializedValue::Update(TypedSerializedValue&& value)
{
	Value = value.Value;
	Adapter = std::move(value.Adapter);
	return this;
}

CDevice* TypedSerializedValue::GetDevice()
{
	if (m_pSerializer)
		return m_pSerializer->GetDevice();
	else
		return nullptr;
}


void TypedSerializedValue::NoConversion(eValueType fromType)
{
	std::string msg = fmt::format("There is no conversion of variable of type '\"{}\" to type \"{}\"",
		GetTypeVerb(),
		TypedSerializedValue::GetTypeVerbByType(fromType)
	);


	if (m_pSerializer)
	{
		auto add = fmt::format(" for variable \"{}\" in container \"{}\"",
			m_pSerializer->GetVariableName(this),
			m_pSerializer->GetClassName()
		);
		msg += add;
	}

	msg += '.';

	throw dfw2error(msg);
}

void MetaSerializedValue::SetDouble(double value)
{
	switch (ValueType)
	{
	case eValueType::VT_DBL:
		*Value.pDbl = value * Multiplier;
		bSet = true;
		break;
	case eValueType::VT_INT:
		*Value.pInt = static_cast<ptrdiff_t>(value);
		bSet = true;
		break;
	case eValueType::VT_BOOL:
		*Value.pBool = value > 0.0;
		bSet = true;
		break;
	case eValueType::VT_CPLX:
	case eValueType::VT_NAME:
	case eValueType::VT_ID:
		NoConversion(eValueType::VT_DBL);
		break;
	case eValueType::VT_STATE:
		if (auto device = GetDevice(); device)
		{
			device->SetState(value > 0 ? eDEVICESTATE::DS_ON : eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_EXTERNAL);
			bSet = true;
		}
		break;
	case eValueType::VT_ADAPTER:
		Adapter->SetInt(static_cast<ptrdiff_t>(value));
		bSet = true;
		break;
	}
}

void MetaSerializedValue::SetBool(bool value)
{
	switch (ValueType)
	{
	case eValueType::VT_DBL:
		*Value.pDbl = Multiplier * (value ? 1.0 : 0.0);
		bSet = true;
		break;
	case eValueType::VT_INT:
		*Value.pInt = value ? 1 : 0;
		bSet = true;
		break;
	case eValueType::VT_BOOL:
		*Value.pBool = value;
		bSet = true;
		break;
	case eValueType::VT_CPLX:
	case eValueType::VT_NAME:
	case eValueType::VT_ID:
		NoConversion(eValueType::VT_BOOL);
		break;
	case eValueType::VT_STATE:
		if (auto device = GetDevice(); device)
		{
			device->SetState(value ? eDEVICESTATE::DS_ON : eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_EXTERNAL);
			bSet = true;
		}
		break;
	case eValueType::VT_ADAPTER:
		Adapter->SetInt(static_cast<ptrdiff_t>(value));
		bSet = true;
		break;
	}
}

void MetaSerializedValue::SetInt(ptrdiff_t value)
{
	switch (ValueType)
	{
	case eValueType::VT_DBL:
		*Value.pDbl = Multiplier * static_cast<double>(value);
		bSet = true;
		break;
	case eValueType::VT_INT:
		*Value.pInt = value;
		bSet = true;
		break;
	case eValueType::VT_BOOL:
		*Value.pBool = value > 0.0;
		bSet = true;
		break;
	case eValueType::VT_CPLX:
	case eValueType::VT_NAME:
		NoConversion(eValueType::VT_INT);
		break;
	case eValueType::VT_STATE:
		if (auto device = GetDevice(); device)
		{
			device->SetState(value > 0 ? eDEVICESTATE::DS_ON : eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_EXTERNAL);
			bSet = true;
		}
		break;
	case eValueType::VT_ID:
		if (auto device = GetDevice(); device)
		{
			device->SetId(static_cast<ptrdiff_t>(value));
			bSet = true;
		}
		break;
	case eValueType::VT_ADAPTER:
		Adapter->SetInt(static_cast<ptrdiff_t>(value));
		bSet = true;
		break;
	}
}

void MetaSerializedValue::SetComplex(const cplx& value)
{
	switch (ValueType)
	{
	case eValueType::VT_CPLX:
		*Value.pCplx = value * Multiplier;
		bSet = true;
		break;
	//case eValueType::VT_ADAPTER:
	//	Adapter->SetInt(static_cast<ptrdiff_t>(value));
	//	break;
	default:
		NoConversion(eValueType::VT_CPLX);
	}
}

void MetaSerializedValue::SetString(std::string_view value)
{
	switch (ValueType)
	{
	case eValueType::VT_NAME:
		if (auto device = GetDevice(); device)
		{
			device->SetName(value);
			bSet = true;
		}
		break;
		case eValueType::VT_ADAPTER:
			Adapter->SetString(value);
			bSet = true;
			break;
	default:
		NoConversion(eValueType::VT_NAME);
	}
}


template<> MetaSerializedValue* MetaSerializedValue::Set<const double&>(const double& value)
{
	SetDouble(value);
	return this;
}

template<> MetaSerializedValue* MetaSerializedValue::Set<const ptrdiff_t&>(const ptrdiff_t& value)
{
	SetInt(value);
	return this;
}

template<> MetaSerializedValue* MetaSerializedValue::Set<const bool&>(const bool& value)
{
	SetBool(value);
	return this;
}

template<> MetaSerializedValue* MetaSerializedValue::Set<const size_t&>(const size_t& value)
{
	SetInt(value);
	return this;
}

template<> MetaSerializedValue* MetaSerializedValue::Set<const std::string&>(const std::string& value)
{
	SetString(value);
	return this;
}

template<> MetaSerializedValue* MetaSerializedValue::Set<const cplx&>(const cplx& value)
{
	SetComplex(value);
	return this;
}

