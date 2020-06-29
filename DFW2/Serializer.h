#pragma once
#include "DeviceTypes.h"
#include "DeviceContainerProperties.h"
#include "string"
#include "map"
#include "list"
#include "vector"
#include "cex.h"
#include "dfw2exception.h"
#include "Header.h"
namespace DFW2
{

	union uValue
	{
		double *pDbl;
		ptrdiff_t *pInt;
		bool *pBool;
		cplx *pCplx;
		uValue(double* pDouble) noexcept: pDbl(pDouble) {}
		uValue(ptrdiff_t* pInteger) noexcept : pInt(pInteger) {}
		uValue(bool* pBoolean) noexcept : pBool(pBoolean) {}
		uValue(cplx* pComplex) noexcept : pCplx(pComplex) {}
		uValue() noexcept {}
	};

	struct TypedSerializedValue;
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
		virtual std::wstring GetString()
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
		virtual void SetString(const _TCHAR* cszString)
		{
			throw dfw2error(cszNotImplemented);
		}
		static const _TCHAR* cszNotImplemented;

		virtual ~CSerializerAdapterBase() {}
	};

	struct TypedSerializedValue
	{
		enum class eValueType
		{
			VT_DBL,
			VT_INT,
			VT_BOOL,
			VT_CPLX,
			VT_NAME,
			VT_STATE,
			VT_ID,
			VT_ADAPTER
		}
		ValueType;

		static const _TCHAR* m_cszTypeDecs[8];

		uValue Value;

		std::unique_ptr<CSerializerAdapterBase> Adapter;

		TypedSerializedValue(VariableIndex* pVariable) : Value(&pVariable->Value), ValueType(eValueType::VT_DBL) {}
		TypedSerializedValue(VariableIndexExternalOptional* pVariable) : Value(pVariable->m_pValue), ValueType(eValueType::VT_DBL) {}
		TypedSerializedValue(CSerializerAdapterBase *pAdapter) : Adapter(pAdapter), ValueType(eValueType::VT_ADAPTER) {}
		TypedSerializedValue(double* pDouble) : Value(pDouble), ValueType(eValueType::VT_DBL) {}
		TypedSerializedValue(ptrdiff_t* pInteger) : Value(pInteger), ValueType(eValueType::VT_INT) {}
		TypedSerializedValue(bool* pBoolean) : Value(pBoolean), ValueType(eValueType::VT_BOOL) {}
		TypedSerializedValue(cplx* pComplex) : Value(pComplex), ValueType(eValueType::VT_CPLX) {}
		TypedSerializedValue(eValueType Type) : Value(), ValueType(Type) {}
		/*
		TypedSerializedValue(TypedSerializedValue&& Copy)
		{
			MakeCopy(Copy);
			Adapter = std::move(Copy.Adapter);
		}

		TypedSerializedValue& operator=(const TypedSerializedValue& Copy)
		{
			MakeCopy(Copy);
			//Adapter.reset(new Adapter(Copy.Adapter));
			return *this;
		}

		TypedSerializedValue& operator=(TypedSerializedValue&& Copy)
		{
			MakeCopy(Copy);
			Adapter = std::move(Copy.Adapter);
			return *this;
		}
		*/

	protected:
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


	using StringVector = std::vector<std::wstring>;

	template<typename T>
	class CSerializerAdapterEnumT : public CSerializerAdapterBaseT<T>
	{
	protected:
		const _TCHAR** m_StringRepresentation;
		size_t m_nCount;
	public:
		ptrdiff_t GetInt() override
		{
			return static_cast<ptrdiff_t>(*m_pLeft);
		}
		void SetInt(ptrdiff_t vInt) noexcept override
		{
			*m_pLeft = static_cast<T>(vInt);
		}
		std::wstring GetString() override
		{
			const ptrdiff_t nIndex = static_cast<ptrdiff_t>(*m_pLeft);
			if (nIndex < 0 || nIndex >= static_cast<ptrdiff_t>(m_nCount))
				throw dfw2error(fmt::format(_T("CSerializerAdapterEnumT::GetString - invalid enum index or string representation {}"), nIndex));
			return std::wstring(m_StringRepresentation[nIndex]);
		}
		CSerializerAdapterEnumT(T& Left, const _TCHAR** ppStringRepresentation, size_t nCount) : CSerializerAdapterBaseT<T>(Left), 
																								 m_StringRepresentation(ppStringRepresentation),
																								 m_nCount(nCount)
																								 {}
		virtual ~CSerializerAdapterEnumT() {}
	};

	class CDevice;

	struct MetaSerializedValue
	{
		TypedSerializedValue Value;
		eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET;
		double Multiplier = 1.0;
		bool bState = false;
		MetaSerializedValue(VariableIndex* pVariable) : Value(&pVariable->Value) {}
		MetaSerializedValue(VariableIndexExternalOptional* pVariable) : Value(pVariable->m_pValue){}
		MetaSerializedValue(CSerializerAdapterBase* pAdapter) : Value(pAdapter) {}
		MetaSerializedValue(double* pDouble) : Value(pDouble) {}
		MetaSerializedValue(ptrdiff_t* pInteger) : Value(pInteger) {}
		MetaSerializedValue(bool* pBoolean) : Value(pBoolean) {}
		MetaSerializedValue(cplx* pComplex) : Value(pComplex) {}
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
		std::wstring m_strClassName;
	public:
		CDevice *m_pDevice = nullptr;
		static const _TCHAR* m_cszDupName;
		static const _TCHAR *m_cszV;
		static const _TCHAR *m_cszState;
		static const _TCHAR *m_cszStateCause;
		static const _TCHAR *m_cszType;

		ptrdiff_t ValuesCount() noexcept
		{
			return ValueMap.size();
		}

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

		void SetClassName(const _TCHAR *cszClassName)
		{
			m_strClassName = cszClassName;
		}

		MetaSerializedValue* AddProperty(const _TCHAR* cszName, TypedSerializedValue::eValueType Type)
		{
			if (IsCreate())
			{
				// ������� ����� ��������
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(Type))->get();
				return AddValue(cszName, mv);
			}
			else
			{
				// ��������� ���������
				UpdateIterator->get()->Value = TypedSerializedValue(Type);
				return UpdateValue();
			}
		}

		template<typename T>
		MetaSerializedValue* AddState(const _TCHAR* cszName, T& Val, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			MetaSerializedValue *meta = AddProperty(cszName, Val, Units, Multiplier);
			meta->bState = true;
			return meta;
		}

		template<typename T>
		MetaSerializedValue* AddProperty(const _TCHAR* cszName, T& Val, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			if (IsCreate())
			{
				// ������� ����� ��������
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(&Val))->get();
				AddValue(cszName, mv);
				mv->Multiplier = Multiplier;
				mv->Units = Units;
				return mv;
			}
			else
			{
				// ��������� ���������
				UpdateIterator->get()->Value = TypedSerializedValue(&Val);
				return UpdateValue();
			}
		}

		MetaSerializedValue* AddEnumProperty(const _TCHAR* cszName, CSerializerAdapterBase* pAdapter, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			if (IsCreate())
			{
				// ������� ����� ��������
				MetaSerializedValue* mv = ValueList.emplace(ValueList.end(), std::make_unique<MetaSerializedValue>(pAdapter))->get();
				return AddValue(cszName, mv);
			}
			else
			{
				UpdateIterator->get()->Value = TypedSerializedValue(pAdapter);
				return UpdateValue();
			}
		}

		template<typename T>
		MetaSerializedValue* AddEnumState(const _TCHAR* cszName, CSerializerAdapterBase* pAdapter, eVARUNITS Units = eVARUNITS::VARUNIT_NOTSET, double Multiplier = 1.0)
		{
			MetaSerializedValue *meta = AddEnumProperty(cszName, pAdapter, Units, Multiplier);
			meta->bState = true;
			return meta;
		}

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

		const _TCHAR* GetClassName();

	protected:

		MetaSerializedValue* AddValue(const _TCHAR *cszName, MetaSerializedValue* mv)
		{
			if (!ValueMap.insert(std::make_pair(cszName, mv)).second)
				throw dfw2error(fmt::format(m_cszDupName, cszName));
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

