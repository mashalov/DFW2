#pragma once
#include <map>
#include <list>
#include <vector>
#include <optional>
#include "DeviceTypes.h"
#include "DeviceContainerPropertiesBase.h"
#include "string"
#include "dfw2exception.h"
#include "Header.h"
namespace DFW2
{
	class CSerializerBase;

	// возможные типы значений для сериализации
	union uValue
	{
		double *pDbl = nullptr;								// double
		ptrdiff_t *pInt;									// int (32/64)
		bool *pBool;										// bool
		cplx *pCplx;										// complex
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
		virtual void SetString(const std::string_view String)
		{
			throw dfw2error(cszNotImplemented);
		}

		static constexpr const char* cszNotImplemented = "CSerializerAdapterBase: - Not implemented";

		virtual ~CSerializerAdapterBase() {}
	};

	class CDevice;
	class CSerializerBase;

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
			VT_ADAPTER,			// пользовательское преобразование с помощью внешнего класса адаптера
			VT_SERIALIZER		// вложенный сериализатор
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
			"adapter",
			"serializer"
		};

		uValue Value;										// собственно значение
		bool bSet = false;									// флаг изменения

		std::unique_ptr<CSerializerAdapterBase> Adapter;		// адаптер для типа eValueType::VT_ADAPTER
		std::unique_ptr<CSerializerBase> m_pNestedSerializer;	// вложенный сериализатор

		// конструкторы для разных типов принимают указатели на значения 

		// переменная состояния
		TypedSerializedValue(CSerializerBase* pSerializer, VariableIndex* pVariable) : m_pSerializer(pSerializer),
																					   Value(&pVariable->Value), 
																					   ValueType(eValueType::VT_DBL) {}
		// внешняя переменная
		TypedSerializedValue(CSerializerBase* pSerializer, VariableIndexExternalOptional* pVariable) : m_pSerializer(pSerializer),
																									   // если указатель внутри переменной nullptr
																									   // забираем локальную переменную
																									   Value(pVariable->pValue ? pVariable->pValue : &pVariable->Value),
																									   ValueType(eValueType::VT_DBL) {}
		// адаптер
		TypedSerializedValue(CSerializerBase* pSerializer, CSerializerAdapterBase *pAdapter) : m_pSerializer(pSerializer),
																							   Adapter(pAdapter), 
																							   ValueType(eValueType::VT_ADAPTER) {}

		TypedSerializedValue(CSerializerBase* pSerializer, double* pDouble) : m_pSerializer(pSerializer),
																			  Value(pDouble), 
																			  ValueType(eValueType::VT_DBL) {}

		TypedSerializedValue(CSerializerBase* pSerializer, ptrdiff_t* pInteger) : m_pSerializer(pSerializer),
																				  Value(pInteger), 
																				  ValueType(eValueType::VT_INT) {}

		TypedSerializedValue(CSerializerBase* pSerializer, bool* pBoolean) : m_pSerializer(pSerializer),
																			 Value(pBoolean), 
																			 ValueType(eValueType::VT_BOOL) {}

		TypedSerializedValue(CSerializerBase* pSerializer, cplx* pComplex) : m_pSerializer(pSerializer),
																		     Value(pComplex), 
																			 ValueType(eValueType::VT_CPLX) {}

		// значение без значения, но с типом
		TypedSerializedValue(CSerializerBase* pSerializer, eValueType Type) : m_pSerializer(pSerializer),
																			  Value(), 
																			  ValueType(Type) {}
		
		TypedSerializedValue(CSerializerBase* pSerializer, CSerializerBase* pNestedSerializer) : m_pSerializer(pSerializer),
			m_pNestedSerializer(pNestedSerializer),
			ValueType(eValueType::VT_SERIALIZER) {}

		// Проверяет значение на значение по-умолчанию
		// 0 для int, 0.0 для double, 0.0+j0.0 для complex
		// можно добавить "" для строки
		bool isSignificant();

		// Возвращает тип в виде ptrdiff_t
		inline ptrdiff_t GetTypeTag() const
		{
			return static_cast<ptrdiff_t>(ValueType);
		}
		// Возвращает тип в виде строки, "unknown" если для типа не найдено строкового описания
		std::string_view GetTypeVerb() const;
		static std::string_view GetTypeVerbByType(TypedSerializedValue::eValueType type);
	protected:
		void NoConversion(eValueType fromType);
		CSerializerBase* m_pSerializer = nullptr;
		CDevice* GetDevice();
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

		std::string GetEnumStrings()
		{
			std::string enumStrings = "[";
			for (ptrdiff_t nIndex = 0; nIndex < static_cast<ptrdiff_t>(m_nCount); nIndex++)
			{
				if (nIndex > 0)
					enumStrings += ',';

				enumStrings += m_StringRepresentation[nIndex];
			}
			enumStrings += "]";
			return enumStrings;
		}

		void SetString(const std::string_view String) override
		{
			ptrdiff_t nIndex(0);
			for ( ; nIndex < static_cast<ptrdiff_t>(m_nCount); nIndex++)
				if (String == m_StringRepresentation[nIndex])
				{
					SetInt(nIndex);
					break;
				}

			if(nIndex == m_nCount)
				throw dfw2error(fmt::format("CSerializerAdapterEnumT::SetString - enum string representation \"{}\" not found in enum {}", 
					String,
					GetEnumStrings()
					));
		}

		CSerializerAdapterEnumT(T& Left, const char** ppStringRepresentation, size_t nCount) : CSerializerAdapterBaseT<T>(Left),
																								 m_StringRepresentation(ppStringRepresentation),
																								 m_nCount(nCount)
																								 {}
		virtual ~CSerializerAdapterEnumT() {}
	};

	// сериализуемое значение с метаинформацией
	struct MetaSerializedValue : public TypedSerializedValue
	{
		eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET;		// единицы измерения
		double Multiplier = 1.0;							// множитель
		bool bState = false;								// признак переменной состояния
		MetaSerializedValue(CSerializerBase* pSerializer, VariableIndex* pVariable) : TypedSerializedValue(pSerializer, &pVariable->Value) {}
		MetaSerializedValue(CSerializerBase* pSerializer, VariableIndexExternalOptional* pVariable) : TypedSerializedValue(pSerializer, pVariable){}
		MetaSerializedValue(CSerializerBase* pSerializer, CSerializerAdapterBase* pAdapter) : TypedSerializedValue(pSerializer, pAdapter) {}
		MetaSerializedValue(CSerializerBase* pSerializer, double* pDouble) : TypedSerializedValue(pSerializer, pDouble) {}
		MetaSerializedValue(CSerializerBase* pSerializer, ptrdiff_t* pInteger) : TypedSerializedValue(pSerializer, pInteger) {}
		MetaSerializedValue(CSerializerBase* pSerializer, bool* pBoolean) : TypedSerializedValue(pSerializer, pBoolean) {}
		MetaSerializedValue(CSerializerBase* pSerializer, cplx* pComplex) : TypedSerializedValue(pSerializer, pComplex) {}
		MetaSerializedValue(CSerializerBase* pSerializer, TypedSerializedValue::eValueType Type) : TypedSerializedValue(pSerializer, Type) {}
		MetaSerializedValue(CSerializerBase* pSerializer, CSerializerBase* pNestedSerializer) : TypedSerializedValue(pSerializer, pNestedSerializer) {}
		std::unique_ptr<CSerializedValueAuxDataBase> pAux;	// адаптер для внешней базы данных
		MetaSerializedValue* Update(TypedSerializedValue&& value);

		void SetDouble(double value);
		void SetBool(bool value);
		void SetInt(ptrdiff_t value);
		void SetComplex(const cplx& value);
		void SetString(std::string_view value);

		template<typename T>
		MetaSerializedValue* Set(T value);
	};

	using SERIALIZERMAP  = std::map<std::string, MetaSerializedValue*, std::less<> >;
	using SERIALIZERLIST = std::list<std::unique_ptr<MetaSerializedValue>>;

	// базовый сериализатор
	class CSerializerBase
	{
	protected:
		SERIALIZERLIST ValueList;			// список значений
		SERIALIZERMAP ValueMap;				// карта "имя"->"значение"

		SERIALIZERLIST::iterator UpdateIterator;
		std::string m_strClassName;	// имя сериализуемого класса 
	public:

		static constexpr const char* m_cszDupName = "CSerializerBase::AddProperty duplicated name \"{}\"";
		static constexpr const char* m_cszState = TypedSerializedValue::m_cszTypeDecs[5];
		static constexpr const char* m_cszStateCause = "cause";
		static constexpr const char* m_cszType = "type";
		static constexpr const char* m_cszDataType = "dataType";
		static constexpr const char* m_cszSerializerType = "serializerType";

		ptrdiff_t ValuesCount() noexcept
		{
			return ValueMap.size();
		}

		virtual CDevice* GetDevice() { return nullptr; }

		// начало обновления сериализатора с заданного устройства
		void BeginUpdate()
		{
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
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(this, Type))->get();
				// добавляем свойство в список значений
				return AddValue(Name, mv);
			}
			else
			{
				// обновляем указатель 
				(*UpdateIterator)->Update(TypedSerializedValue(this,Type));
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
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(this, &Val))->get();
				AddValue(Name, mv);
				mv->Multiplier = Multiplier;
				mv->Units = Units;
				return mv;
			}
			else
			{
				// обновляем указатель
				(*UpdateIterator)->Update(TypedSerializedValue(this, &Val));
				return UpdateValue();
			}
		}

		MetaSerializedValue* AddSerializer(std::string_view Name, CSerializerBase* pSerializer)
		{
			if (IsCreate())
			{
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(this, pSerializer))->get();
				AddValue(Name, mv);
				return mv;
			}
			else
			{
				(*UpdateIterator)->Update(TypedSerializedValue(this, pSerializer));
				return UpdateValue();
			}
		}

		MetaSerializedValue* AddEnumProperty(std::string_view Name, CSerializerAdapterBase* pAdapter, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			if (IsCreate())
			{
				// создаем новое значение
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(this, pAdapter))->get();
				return AddValue(Name, mv);
			}
			else
			{
				(*UpdateIterator)->Update(TypedSerializedValue(this, pAdapter));
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

		// поиск значения по ключу
		MetaSerializedValue* operator[](std::string_view Name ) 
		{ 
			return at(Name);
		}

		// поиск значения по ключу
		MetaSerializedValue* at(std::string_view Name)
		{
			if (auto it = ValueMap.find(Name); it != ValueMap.end())
				return it->second;
			else
				return nullptr;
		}

		const SERIALIZERMAP GetUnsetValues() const;

		CSerializerBase()
		{
			UpdateIterator = ValueList.end();
		}

		virtual ~CSerializerBase()
		{

		}

		virtual const char* GetClassName();
		std::string GetVariableName(TypedSerializedValue* pValue) const;

		virtual bool NextItem()
		{
			return false;
		}

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


	class CDeviceSerializer : public CSerializerBase
	{
	protected:
		CDevice* m_pDevice = nullptr;
		ptrdiff_t m_nDeviceIndex = 0;
	public:

		CDeviceSerializer() : CSerializerBase() {}
		CDeviceSerializer(CDevice* pDevice) : CSerializerBase(), m_pDevice(pDevice) {}
		CDevice* GetDevice() override { return m_pDevice; }

		// начало обновления сериализатора с заданного устройства
		void BeginUpdate(CDevice* pDevice)
		{
			// запоминаем устройство
			m_pDevice = pDevice;
			m_nDeviceIndex = 0;
			CSerializerBase::BeginUpdate();
		}

		const char* GetClassName() override;
		bool NextItem() override;

	};

	using SerializerPtr = std::unique_ptr<CSerializerBase>;
	using DeviceSerializerPtr = std::unique_ptr<CDeviceSerializer>;
}

