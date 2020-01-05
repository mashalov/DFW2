#pragma once
#include "DeviceTypes.h"
#include "string"
#include "map"
#include "list"
#include "cex.h"
#include "dfw2exception.h"
namespace DFW2
{
	struct TypedSerializedValue
	{
		enum class eValueType
		{
			VT_DBL,
			VT_INT,
			VT_BOOL,
			VT_NAME,
			VT_STATE,
			VT_ID
		}
		ValueType;

		union uValue
		{
			double *pDbl;
			ptrdiff_t *pInt;
			bool *pBool;
			uValue(double* pDouble) : pDbl(pDouble) {}
			uValue(ptrdiff_t* pInteger) : pInt(pInteger) {}
			uValue(bool* pBoolean) : pBool(pBoolean) {}
			uValue() {}
		}
		Value;

		TypedSerializedValue(double* pDouble) : Value(pDouble), ValueType(eValueType::VT_DBL) {}
		TypedSerializedValue(ptrdiff_t* pInteger) : Value(pInteger), ValueType(eValueType::VT_INT) {}
		TypedSerializedValue(bool* pBoolean) : Value(pBoolean), ValueType(eValueType::VT_BOOL) {}
		TypedSerializedValue(eValueType Type) : Value(), ValueType(Type) {}
	};

	class CSerializedValueAuxDataBase
	{

	};

	class CDevice;

	struct MetaSerializedValue
	{
		TypedSerializedValue Value;
		eVARUNITS Units = eVARUNITS::VARUNIT_PU;
		double Multiplier = 1.0;
		MetaSerializedValue(double* pDouble) : Value(pDouble) {}
		MetaSerializedValue(ptrdiff_t* pInteger) : Value(pInteger) {}
		MetaSerializedValue(bool* pBoolean) : Value(pBoolean) {}
		MetaSerializedValue(TypedSerializedValue::eValueType Type) : Value(Type) {}
		std::unique_ptr<CSerializedValueAuxDataBase> pAux;
	};

	using SERIALIZERMAP  = std::map<std::wstring, MetaSerializedValue*>;
	using SERIALIZERLIST = std::list<std::unique_ptr<MetaSerializedValue>>;

	class CSerializerBase
	{
	protected:
		SERIALIZERLIST ValueList;
		SERIALIZERMAP ValueMap;
		SERIALIZERLIST::iterator UpdateIterator;
	public:
		CDevice *m_pDevice = nullptr;
		static const _TCHAR* m_cszDupName;

		void BeginUpdate(CDevice *pDevice)
		{
			m_pDevice = pDevice;
			if (ValueList.empty())
				throw dfw2error(_T("CSerializerBase::BeginUpdate on empty value list"));
			else
				UpdateIterator = ValueList.begin();
		}

		inline bool IsCreate()
		{
			return UpdateIterator == ValueList.end();
		}

		MetaSerializedValue* AddProperty(const _TCHAR* cszName, TypedSerializedValue::eValueType Type)
		{
			if (IsCreate())
			{
				// создаем новое значение
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(Type))->get();
				if (!ValueMap.insert(std::make_pair(cszName, mv)).second)
					throw dfw2error(Cex(m_cszDupName, cszName));
				UpdateIterator = ValueList.end();
				return mv;
			}
			else
			{
				// обновляем указатель
				UpdateIterator->get()->Value = TypedSerializedValue(Type);
				UpdateIterator++;
				return prev(UpdateIterator)->get();
			}
		}

		template<typename T>
		MetaSerializedValue* AddProperty(const _TCHAR* cszName, T& Val, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			if (IsCreate())
			{
				// создаем новое значение
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(&Val))->get();
				mv->Multiplier = Multiplier;
				mv->Units = Units;
				if (!ValueMap.insert(std::make_pair(cszName, mv)).second)
					throw dfw2error(Cex(m_cszDupName, cszName));
				UpdateIterator = ValueList.end();
				return mv;
			}
			else
			{
				// обновляем указатель
				UpdateIterator->get()->Value = TypedSerializedValue(&Val);
				UpdateIterator++;
				return prev(UpdateIterator)->get();
			}
		}

		SERIALIZERMAP::const_iterator begin() { return ValueMap.begin(); }
		SERIALIZERMAP::const_iterator end()   { return ValueMap.end();   }

		CSerializerBase(CDevice *pDevice) : m_pDevice(pDevice)
		{
			UpdateIterator = ValueList.end();
		}
		virtual ~CSerializerBase()
		{

		}
	};
}

