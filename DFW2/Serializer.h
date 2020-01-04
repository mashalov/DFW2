#pragma once
#include "DeviceTypes.h"
#include "string"
#include "map"
#include "list"
#include "dfw2exception.h"
namespace DFW2
{
	struct TypedSerializedValue
	{
		union uValue
		{
			double *pDbl;
			ptrdiff_t *pInt;
			bool *pBool;
			uValue(double* pDouble) : pDbl(pDouble) {}
			uValue(ptrdiff_t* pInteger) : pInt(pInteger) {}
			uValue(bool* pBoolean) : pBool(pBoolean) {}
		}
		Value;

		enum class eValueType
		{
			VT_DBL,
			VT_INT,
			VT_BOOL
		}
		ValueType;

		TypedSerializedValue(double* pDouble) : Value(pDouble), ValueType(eValueType::VT_DBL) {}
		TypedSerializedValue(ptrdiff_t* pInteger) : Value(pInteger), ValueType(eValueType::VT_INT) {}
		TypedSerializedValue(bool* pBoolean) : Value(pBoolean), ValueType(eValueType::VT_BOOL) {}
	};

	class CSerializedValueAuxDataBase
	{

	};

	struct MetaSerializedValue
	{
		TypedSerializedValue Value;
		eVARUNITS Units = eVARUNITS::VARUNIT_PU;
		double Multiplier = 1.0;
		MetaSerializedValue(double* pDouble) : Value(pDouble) {}
		MetaSerializedValue(ptrdiff_t* pInteger) : Value(pInteger) {}
		MetaSerializedValue(bool* pBoolean) : Value(pBoolean) {}
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
		void BeginUpdate()
		{
			if (ValueList.empty())
				throw dfw2error(_T("CSerializerBase::BeginUpdate on empty value list"));
			else
				UpdateIterator = ValueList.begin();
		}

		inline bool IsCreate()
		{
			return UpdateIterator == ValueList.end();
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
					throw dfw2error(Cex(_T("CSerializerBase::AddProperty duplicated name \"%s\""), cszName));
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

		CSerializerBase()
		{
			UpdateIterator = ValueList.end();
		}
		virtual ~CSerializerBase()
		{

		}
	};
}

