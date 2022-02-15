#include "stdafx.h"
#include "Serializer.h"
#include "DeviceContainer.h"
#include "FmtComplexFormat.h"
using namespace DFW2;


ptrdiff_t CSerializerDataSourceContainer::ItemsCount() const
{
	return pContainer_->Count();
}

bool CSerializerDataSourceContainer::NextItem()
{
	nItemIndex++;
	if (nItemIndex < ItemsCount())
		return true;
	else
		return false;
}

CDevice* CSerializerDataSourceContainer::GetDevice() const
{
	if (pContainer_->Count())
		return pContainer_->GetDeviceByIndex(0);
	else
		return CSerializerDataSourceBase::GetDevice();
}

void CSerializerDataSourceContainer::UpdateSerializer(CSerializerBase* pSerializer)
{
	pContainer_->GetDeviceByIndex(nItemIndex)->UpdateSerializer(pSerializer);
}

const char* CSerializerBase::GetClassName()
{
	if (m_pDevice)
		return m_pDevice->GetContainer()->GetSystemClassName();
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
	m_pNestedSerializer = std::move(value.m_pNestedSerializer);
	return this;
}

CDevice* TypedSerializedValue::GetDevice()
{
	if (m_pSerializer)
		return m_pSerializer->GetDevice();
	else
		return nullptr;
}

const CDevice* TypedSerializedValue::GetDevice() const
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
	case eValueType::VT_STRING:
		*Value.pStr = value;
		bSet = true;
		break;
	default:
		NoConversion(ValueType);
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

// Возвращает true, если данный указатель соответствует типу переменной и равен 
// сохраненному в переменной указателю

// Специализация для double
template<> bool TypedSerializedValue::IsThatPointer<double>(double* type)
{
	return ValueType == TypedSerializedValue::eValueType::VT_DBL && Value.pDbl == type;
}

double TypedSerializedValue::Double() const
{
	switch (ValueType)
	{
	case eValueType::VT_DBL:
		return *Value.pDbl;
	case eValueType::VT_INT:
		return static_cast<double>(*Value.pInt);
	case eValueType::VT_BOOL:
		return *Value.pBool ? 1.0 : 0.0;
	case eValueType::VT_STATE:
		if (const auto device(GetDevice()); device)
			return device->GetState() == eDEVICESTATE::DS_ON ? 1.0 : 0.0;
	case eValueType::VT_ID:
		if (const auto device(GetDevice()); device)
			return static_cast<double>(device->GetId());
	case eValueType::VT_ADAPTER:
		return Adapter->GetDouble();
	}

	throw dfw2error(fmt::format("TypedSerializedValue::Double() - unexpected value type {}, device not available or bad adapter",
		static_cast<std::underlying_type<TypedSerializedValue::eValueType>::type>(ValueType)));
}

std::string TypedSerializedValue::String() const
{
	constexpr const char* braces = "{}";
	switch (ValueType)
	{
	case eValueType::VT_DBL:
		return fmt::format(braces, *Value.pDbl);
	case eValueType::VT_INT:
		return fmt::format(braces, *Value.pInt);
	case eValueType::VT_BOOL:
		return fmt::format(braces, *Value.pBool ? "true": "false");
	case eValueType::VT_CPLX:
		return fmt::format(braces, *Value.pCplx);
	case eValueType::VT_STRING:
		return *Value.pStr;
	case eValueType::VT_NAME:
		if (const auto device(GetDevice()); device)
			return fmt::format(braces, device->GetName());
	case eValueType::VT_STATE:
		if (const auto device(GetDevice()); device)
			return fmt::format(braces, device->GetState());
	case eValueType::VT_ID:
		if (const auto device(GetDevice()); device)
			return fmt::format(braces, device->GetId());
	case eValueType::VT_ADAPTER:
		return fmt::format(braces, Adapter->GetString());
	}

	throw dfw2error(fmt::format("TypedSerializedValue::String() - unexpected value type {}, device not available or bad adapter",
		static_cast<std::underlying_type<TypedSerializedValue::eValueType>::type>(ValueType)));
}