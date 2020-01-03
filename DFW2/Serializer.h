#pragma once
#include "string"
#include "map"
namespace DFW2
{
	struct SerializedValue
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

		SerializedValue(double* pDouble) : Value(pDouble), ValueType(eValueType::VT_DBL) {}
		SerializedValue(ptrdiff_t* pInteger) : Value(pInteger), ValueType(eValueType::VT_INT) {}
		SerializedValue(bool* pBoolean) : Value(pBoolean), ValueType(eValueType::VT_BOOL) {}
	};

	using SERIALIZERMAP = std::map<std::wstring, SerializedValue>;

	class CSerializerBase
	{
	protected:
		SERIALIZERMAP ValueMap;
	public:
		void AddProperty(const _TCHAR* cszName, double& pDbl)
		{
			ValueMap.insert(std::make_pair(cszName, SerializedValue(&pDbl)));
		}
		void AddProperty(const _TCHAR* cszName, ptrdiff_t& pInt)
		{
			ValueMap.insert(std::make_pair(cszName, SerializedValue(&pInt)));
		}
		void AddProperty(const _TCHAR* cszName, bool& pBool)
		{
			ValueMap.insert(std::make_pair(cszName, SerializedValue(&pBool)));
		}

		SERIALIZERMAP::const_iterator begin() { return ValueMap.begin(); }
		SERIALIZERMAP::const_iterator end() {   return ValueMap.end(); }

		CSerializerBase();
		virtual ~CSerializerBase();
	};
}

