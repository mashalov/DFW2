#pragma once
#include "DeviceTypes.h"
#include "DeviceContainerProperties.h"
#include "string"
#include "map"
#include "list"
#include "vector"
#include "dfw2exception.h"
#include "Header.h"
namespace DFW2
{

	// возможные типы значений для сериализации
	union uValue
	{
		double *pDbl;												// double
		ptrdiff_t *pInt;											// int (32/64)
		bool *pBool;												// bool
		cplx *pCplx;												// complex

		// конструкторы для разных типов
		uValue(double* pDouble) noexcept: pDbl(pDouble) {}
		uValue(ptrdiff_t* pInteger) noexcept : pInt(pInteger) {}
		uValue(bool* pBoolean) noexcept : pBool(pBoolean) {}
		uValue(cplx* pComplex) noexcept : pCplx(pComplex) {}
		uValue() noexcept {}
	};

	struct TypedSerializedValue;

	// базовый класс адаптера для пользовательского значения

	class CSerializerAdapterBase
	{
	public:
		virtual ptrdiff_t GetInt()
		{
			throw dfw2error(cszNotImplemented);
		}
		virtual double GetDouble()
		{
			throw dfw2error(cszNotImplemented);
		}
		virtual bool GetBool()
		{
			throw dfw2error(cszNotImplemented);
		}
		virtual std::string GetString()
		{
			throw dfw2error(cszNotImplemented);
		}
		virtual void SetInt(ptrdiff_t vInt)
		{
			throw dfw2error(cszNotImplemented);
		}
		virtual void SetDouble(double vDbl)
		{
			throw dfw2error(cszNotImplemented);
		}
		virtual void SetBool(bool vBool)
		{
			throw dfw2error(cszNotImplemented);
		}
		virtual void SetString(const char* cszString)
		{
			throw dfw2error(cszNotImplemented);
		}

		static constexpr const char* cszNotImplemented = "CSerializerAdapterBase: - Not implemented";

		virtual ~CSerializerAdapterBase() {}
	};

	// сериализуемое значение
	struct TypedSerializedValue
	{
		// типы сериализуемых значений
		enum class eValueType
		{
			VT_DBL,
			VT_INT,
			VT_BOOL,
			VT_CPLX,
			VT_NAME,			// имя (дополнительное преобразование)
			VT_STATE,			// состояние (дополнительное преобразование)
			VT_ID,				// идентификатор (дополнительное преобразование)
			VT_ADAPTER			// пользовательское преобразование с помощью внешнего класса адаптера
		}
		ValueType;		

		static constexpr const char* m_cszTypeDecs[] = { 
			"double",
			"int",
			"bool",
			"complex",
			"name",
			"state",
			"id",
			"adapter"
		};

		uValue Value;										// собственно значение

		std::unique_ptr<CSerializerAdapterBase> Adapter;	// адаптер для типа eValueType::VT_ADAPTER

		// конструкторы для разных типов принимают указатели на значения 

		// переменная состояния
		TypedSerializedValue(VariableIndex* pVariable) : Value(&pVariable->Value), ValueType(eValueType::VT_DBL) {}
		// внешняя переменная
		TypedSerializedValue(VariableIndexExternalOptional* pVariable) : Value(pVariable->pValue), ValueType(eValueType::VT_DBL) {}
		// адаптер
		TypedSerializedValue(CSerializerAdapterBase *pAdapter) : Adapter(pAdapter), ValueType(eValueType::VT_ADAPTER) {}
		TypedSerializedValue(double* pDouble) : Value(pDouble), ValueType(eValueType::VT_DBL) {}
		TypedSerializedValue(ptrdiff_t* pInteger) : Value(pInteger), ValueType(eValueType::VT_INT) {}
		TypedSerializedValue(bool* pBoolean) : Value(pBoolean), ValueType(eValueType::VT_BOOL) {}
		TypedSerializedValue(cplx* pComplex) : Value(pComplex), ValueType(eValueType::VT_CPLX) {}
		// значение без значения, но с типом
		TypedSerializedValue(eValueType Type) : Value(), ValueType(Type) {}
		
		// Проверяет значение на значение по-умолчанию
		// 0 для int, 0.0 для double, 0.0+j0.0 для complex
		// можно добавить "" для строки
		bool isSignificant() 
		{
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

	protected:
		// deep-copy
		void MakeCopy(const TypedSerializedValue& Copy) noexcept
		{
			switch (Copy.ValueType)
			{
			case eValueType::VT_DBL:
				Value.pDbl = Copy.Value.pDbl;
				break;
			case eValueType::VT_INT:
				Value.pInt = Copy.Value.pInt;
				break;
			case eValueType::VT_BOOL:
				Value.pBool = Copy.Value.pBool;
				break;
			case eValueType::VT_NAME:
				break;
			case eValueType::VT_STATE:
				break;
			case eValueType::VT_ID:
				break;
			}
			ValueType = Copy.ValueType;
		}
	};

	class CSerializedValueAuxDataBase {};

	template<typename T>
	class CSerializerAdapterBaseT : public CSerializerAdapterBase
	{
	protected:
		T* m_pLeft = nullptr;
	public:
		CSerializerAdapterBaseT(T& Left) noexcept : m_pLeft(&Left) {}
		virtual ~CSerializerAdapterBaseT() {}
	};

	using StringVector = std::vector<std::string>;

	template<typename T>
	class CSerializerAdapterEnumT : public CSerializerAdapterBaseT<T>
	{
	protected:
		// текстовое представление значения
		const char** m_StringRepresentation;
		size_t m_nCount;
	public:
		ptrdiff_t GetInt() override
		{
			return static_cast<ptrdiff_t>(*CSerializerAdapterBaseT<T>::m_pLeft);
		}
		void SetInt(ptrdiff_t vInt) noexcept override
		{
			*CSerializerAdapterBaseT<T>::m_pLeft = static_cast<T>(vInt);
		}
		std::string GetString() override
		{
			const ptrdiff_t nIndex = static_cast<ptrdiff_t>(*CSerializerAdapterBaseT<T>::m_pLeft);
			if (nIndex < 0 || nIndex >= static_cast<ptrdiff_t>(m_nCount))
				throw dfw2error(fmt::format("CSerializerAdapterEnumT::GetString - invalid enum index or string representation {}", nIndex));
			return std::string(m_StringRepresentation[nIndex]);
		}
		CSerializerAdapterEnumT(T& Left, const char** ppStringRepresentation, size_t nCount) : CSerializerAdapterBaseT<T>(Left),
																								 m_StringRepresentation(ppStringRepresentation),
																								 m_nCount(nCount)
																								 {}
		virtual ~CSerializerAdapterEnumT() {}
	};

	class CDevice;

	// сериализуемое значение с метаинформацией
	struct MetaSerializedValue
	{
		TypedSerializedValue Value;							// собственно значение
		eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET;		// единицы измерения
		double Multiplier = 1.0;							// множитель
		bool bState = false;								// признак переменной состояния
		MetaSerializedValue(VariableIndex* pVariable) : Value(&pVariable->Value) {}
		MetaSerializedValue(VariableIndexExternalOptional* pVariable) : Value(pVariable->pValue){}
		MetaSerializedValue(CSerializerAdapterBase* pAdapter) : Value(pAdapter) {}
		MetaSerializedValue(double* pDouble) : Value(pDouble) {}
		MetaSerializedValue(ptrdiff_t* pInteger) : Value(pInteger) {}
		MetaSerializedValue(bool* pBoolean) : Value(pBoolean) {}
		MetaSerializedValue(cplx* pComplex) : Value(pComplex) {}
		MetaSerializedValue(TypedSerializedValue::eValueType Type) : Value(Type) {}
		std::unique_ptr<CSerializedValueAuxDataBase> pAux;	// адаптер для внешней базы данных
	};

	using SERIALIZERMAP  = std::map<std::string, MetaSerializedValue*>;
	using SERIALIZERLIST = std::list<std::unique_ptr<MetaSerializedValue>>;

	// базовый сериализатор
	class CSerializerBase
	{
	protected:
		SERIALIZERLIST ValueList;		// список значений
		SERIALIZERMAP ValueMap;			// карта "имя"->"значение"
		SERIALIZERLIST::iterator UpdateIterator;
		std::string m_strClassName;	// имя сериализуемого класса 
		CDevice* m_pDevice = nullptr;
	public:

		static constexpr const char* m_cszDupName = "CSerializerBase::AddProperty duplicated name \"{}\"";
		static constexpr const char* m_cszState = TypedSerializedValue::m_cszTypeDecs[5];
		static constexpr const char* m_cszStateCause = "cause";
		static constexpr const char* m_cszType = "type";
		static constexpr const char* m_cszDataType = "dataType";

		ptrdiff_t ValuesCount() noexcept
		{
			return ValueMap.size();
		}
		CDevice* GetDevice() { return m_pDevice; }


		// начало обновления сериализатора с заданного устройства
		void BeginUpdate(CDevice *pDevice)
		{
			// запоминаем устройство
			m_pDevice = pDevice;
			// проверяем есть ли сериализуемые значения
			if (ValueList.empty())
				throw dfw2error("CSerializerBase::BeginUpdate on empty value list");
			else
				UpdateIterator = ValueList.begin();  // ставим итератор обновления на начало списка значений
		}

		inline bool IsCreate()
		{
			// если итератор обновления не сброшен в начало списка значений - то мы только что создали сериализатор
			return UpdateIterator == ValueList.end();
		}

		void SetClassName(std::string_view ClassName)
		{
			m_strClassName = ClassName;
		}

		// добавление свойства
		MetaSerializedValue* AddProperty(std::string_view Name, TypedSerializedValue::eValueType Type)
		{
			if (IsCreate())
			{
				// создаем новое свойство 
				// в список свойств добавляем новое значение с метаинформацией
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(Type))->get();
				// добавляем свойство в список значений
				return AddValue(Name, mv);
			}
			else
			{
				// обновляем указатель 
				UpdateIterator->get()->Value = TypedSerializedValue(Type);
				// переходим к следующему значению
				return UpdateValue();
			}
		}

		// добавляем переменную состояния
		template<typename T>
		MetaSerializedValue* AddState(std::string_view Name, T& Val, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			// добавляем свойство по заданному типу
			MetaSerializedValue *meta = AddProperty(Name, Val, Units, Multiplier);
			// ставим признак переменной состояния
			meta->bState = true;
			return meta;
		}

		template<typename T>
		MetaSerializedValue* AddProperty(std::string_view Name, T& Val, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			if (IsCreate())
			{
				// создаем новое значение
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(&Val))->get();
				AddValue(Name, mv);
				mv->Multiplier = Multiplier;
				mv->Units = Units;
				return mv;
			}
			else
			{
				// обновляем указатель
				UpdateIterator->get()->Value = TypedSerializedValue(&Val);
				return UpdateValue();
			}
		}

		MetaSerializedValue* AddEnumProperty(std::string_view Name, CSerializerAdapterBase* pAdapter, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			if (IsCreate())
			{
				// создаем новое значение
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(pAdapter))->get();
				return AddValue(Name, mv);
			}
			else
			{
				UpdateIterator->get()->Value = TypedSerializedValue(pAdapter);
				return UpdateValue();
			}
		}

		template<typename T>
		MetaSerializedValue* AddEnumState(std::string_view Name, CSerializerAdapterBase* pAdapter, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			MetaSerializedValue *meta = AddEnumProperty(Name, pAdapter, Units, Multiplier);
			meta->bState = true;
			return meta;
		}

		// поддержка range для карты значений
		SERIALIZERMAP::const_iterator begin() { return ValueMap.begin(); }
		SERIALIZERMAP::const_iterator end()   { return ValueMap.end();   }


		CSerializerBase() : m_pDevice(nullptr)
		{
			UpdateIterator = ValueList.end();
		}

		CSerializerBase(CDevice *pDevice) : m_pDevice(pDevice)
		{
			UpdateIterator = ValueList.end();
		}

		virtual ~CSerializerBase()
		{

		}

		const char* GetClassName();

	protected:

		// добавляем в карту значений новое по имени
		MetaSerializedValue* AddValue(std::string_view Name, MetaSerializedValue* mv)
		{
			// имеем имя значения и его метаданные
			// проверяем нет ли такого значения в карте по имени
			if (!ValueMap.insert(std::make_pair(Name, mv)).second)
				throw dfw2error(fmt::format(m_cszDupName, Name));
			// итератор обновления ставим в конец списка (режим создания итератора)
			UpdateIterator = ValueList.end();
			return mv;
		}

		MetaSerializedValue* UpdateValue()
		{
			UpdateIterator++;
			return prev(UpdateIterator)->get();
		}
	};

	using SerializerPtr = std::unique_ptr<CSerializerBase>;
}

