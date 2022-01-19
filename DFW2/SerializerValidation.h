#pragma once
#include "Serializer.h"


namespace DFW2
{
	class CDynaModel;

	enum class ValidationResult
	{
		Ok,
		Replaced,
		Warning,
		Error
	};

	class CValidationRuleBase
	{
	protected:
		ValidationResult DefaultResult = ValidationResult::Error;
		std::optional<double> replaceValue;
		void CheckDevice(const CDevice* device) const
		{
			if(!device)
				throw dfw2error("CValidationRuleCompareBase::Validate - no device available");
		}
	public:
		CValidationRuleBase() {}
		CValidationRuleBase(std::optional<double> Replace) : replaceValue(Replace) {}
		virtual ~CValidationRuleBase() = default;
		virtual ValidationResult Validate(MetaSerializedValue* value, CDevice *device, std::string& message) const
		{
			return ValidationResult::Ok;
		}

		ValidationResult ReplaceValue(MetaSerializedValue* value) const
		{
			if (replaceValue.has_value())
			{
				value->SetDouble(ReplaceValue());
				return ValidationResult::Replaced;
			}
			else
				return DefaultResult;
		}

		double ReplaceValue() const
		{
			if (!replaceValue.has_value())
				throw dfw2error("CValidationRuleBase::ReplaceValue - no value specified");
			else
				return replaceValue.value();
		}
	};

	class CValidationRuleBiggerThanZero : public CValidationRuleBase
	{
	public:
		using CValidationRuleBase::CValidationRuleBase;
		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			ValidationResult res(value->Double() > 0.0 ? ValidationResult::Ok : DefaultResult);
			if (res != ValidationResult::Ok)
			{
				message = CDFW2Messages::m_cszValidationBiggerThanZero;
				res = ReplaceValue(value);
			}
			return res;
		}
	};

	class CValidationRuleBiggerThanUnity : public CValidationRuleBase
	{
	public:
		using CValidationRuleBase::CValidationRuleBase;
		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			ValidationResult res(value->Double() > 1.0 ? ValidationResult::Ok : DefaultResult);
			if (res != ValidationResult::Ok)
			{
				message = CDFW2Messages::m_cszValidationBiggerThanZero;
				res = ReplaceValue(value);
			}
			return res;
		}
	};

	class CValidationRuleNegative : public CValidationRuleBase
	{
	public:
		using CValidationRuleBase::CValidationRuleBase;
		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			ValidationResult res(value->Double() < 0.0 ? ValidationResult::Ok : DefaultResult);
			if (res != ValidationResult::Ok)
			{
				message = CDFW2Messages::m_cszValidationNegative;
				res = ReplaceValue(value);
			}
			return res;
		}
	};

	class CValidationRuleNonNegative : public CValidationRuleBase
	{
	public:

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			ValidationResult res(value->Double() >= 0.0 ? ValidationResult::Ok : DefaultResult);
			if (res != ValidationResult::Ok)
			{
				message = CDFW2Messages::m_cszValidationBiggerThanZero;
				res = ReplaceValue(value);
			}
			return res;
		}
	};


	template<class T, double T::*member>
	class CValidationRuleMemberT : public CValidationRuleBase
	{
	protected:
		const char* m_cszName;
	public:
		CValidationRuleMemberT(const char* cszName) : CValidationRuleBase(), m_cszName(cszName) {}
	};

	template<class T, double T::*member>
	class CValidationRuleBiggerT : public CValidationRuleMemberT<T, member>
	{
	public:
		using CValidationRuleMemberT<T, member>::CValidationRuleMemberT;
		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CValidationRuleBase::CheckDevice(device);
			const T* pDev = static_cast<const T*>(device);
			if (value->Double() <= pDev->*member)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationBiggerThanNamed, CValidationRuleMemberT<T, member>::m_cszName, pDev->*member);
				return ValidationResult::Error;
			}
			return ValidationResult::Ok;
		}
	};

	template<class T, double T::* member>
	class CValidationRuleBiggerOrEqualT : public CValidationRuleMemberT<T, member>
	{
	public:
		using CValidationRuleMemberT<T, member>::CValidationRuleMemberT;
		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CValidationRuleBase::CheckDevice(device);
			const T* pDev = static_cast<const T*>(device);
			if (value->Double() < pDev->*member)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationBiggerOrEqualThanNamed, CValidationRuleMemberT<T, member>::m_cszName, pDev->*member);
				return ValidationResult::Error;
			}
			return ValidationResult::Ok;
		}
	};


	template<class T, double T::* member>
	class CValidationRuleLessOrEqualT : public CValidationRuleMemberT<T, member>
	{
	public:
		using CValidationRuleMemberT<T, member>::CValidationRuleMemberT;
		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CValidationRuleBase::CheckDevice(device);
			const T* pDev = static_cast<const T*>(device);
			if (value->Double() > pDev->*member)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationLessOrEqualThanNamed, CValidationRuleMemberT<T, member>::m_cszName, pDev->*member);
				return ValidationResult::Error;
			}
			return ValidationResult::Ok;
		}
	};

	template<class T, double T::* member>
	class CValidationRuleLess : public CValidationRuleMemberT<T, member>
	{
	public:
		using CValidationRuleMemberT<T, member>::CValidationRuleMemberT;
		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CValidationRuleBase::CheckDevice(device);
			const T* pDev = static_cast<const T*>(device);
			if (value->Double() >= pDev->*member)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationLessThanNamed, CValidationRuleMemberT<T, member>::m_cszName, pDev->*member);
				return ValidationResult::Error;
			}
			return ValidationResult::Ok;
		}
	};


	class CValidationRuleRange: public CValidationRuleBase
	{
	protected:
		double m_Min, m_Max;
	public:

		CValidationRuleRange(double Min, double Max) : CValidationRuleBase(), m_Min(Min), m_Max(Max) {}

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			const double val(value->Double());
			if (val > m_Max || val < m_Min)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationRange, m_Min, m_Max);
				return ReplaceValue(value);
			}
			return ValidationResult::Ok;
		}
	};

	using RuleSetT = std::set<const CValidationRuleBase*>;
	using VarRuleMapT = std::map<std::string, RuleSetT, std::less<> >;

	class CSerializerValidatorRules
	{
	protected:
		VarRuleMapT m_RulesMap;
		const RuleSetT empty;
	public:
		void AddRule(std::string_view VariableName, const CValidationRuleBase* pRule)
		{
			auto it = m_RulesMap.find(VariableName);
			if (it == m_RulesMap.end())
				m_RulesMap.insert(std::make_pair(VariableName, RuleSetT{ pRule }));
			else
				it->second.insert(pRule);
		}
		void AddRule(std::initializer_list<std::string_view> VariableNames, const CValidationRuleBase* pRule)
		{
			for (const auto& var : VariableNames)
				AddRule(var, pRule);
		}

		const RuleSetT& GetRules(std::string_view VariableName)
		{
			if (const auto it(m_RulesMap.find(VariableName)); it != m_RulesMap.end())
				return it->second;
			else
				return empty;
		}

		inline VarRuleMapT::const_iterator begin() { return m_RulesMap.begin(); }
		inline VarRuleMapT::const_iterator end()   { return m_RulesMap.end(); }

		static inline CValidationRuleBiggerThanUnity BiggerThanUnity;
		static inline CValidationRuleBiggerThanZero BiggerThanZero;
		static inline CValidationRuleNonNegative NonNegative;
		static inline CValidationRuleNegative Negative;
	};

	using SerializerValidatorRulesPtr = std::unique_ptr<CSerializerValidatorRules>;

	class CSerializerValidator
	{
		SerializerPtr m_Serializer;
		SerializerValidatorRulesPtr m_Rules;
		const CDynaModel* m_pDynaModel;
	protected:
		void Log(DFW2MessageStatus Status, std::string_view Message);
		const char* ClassName() const;
	public:
		CSerializerValidator(const CDynaModel* pDynaModel, SerializerPtr&& serializer, SerializerValidatorRulesPtr&& rules) :
			m_pDynaModel(pDynaModel),
			m_Serializer(std::move(serializer)),
			m_Rules(std::move(rules))
		{}
		eDEVICEFUNCTIONSTATUS Validate();
	};

	
}
