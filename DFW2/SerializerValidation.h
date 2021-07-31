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
		std::string message;
		ValidationResult DefaultResult = ValidationResult::Error;
		std::optional<double> replaceValue;
	public:
		CValidationRuleBase(std::string_view Message) : message(Message) {}
		virtual ~CValidationRuleBase() = default;
		virtual ValidationResult Validate(MetaSerializedValue* value) const
		{
			return ValidationResult::Ok;
		}
		virtual const std::string& Message() const
		{
			return message;
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
		CValidationRuleBiggerThanZero() : CValidationRuleBase(CDFW2Messages::m_cszValidationBiggerThanZero) 
		{
			DefaultResult = ValidationResult::Warning;
		}
		ValidationResult Validate(MetaSerializedValue* value) const override
		{
			ValidationResult res(value->Double() > 0.0 ? ValidationResult::Ok : DefaultResult);
			if (res != ValidationResult::Ok && replaceValue.has_value())
			{
				value->SetDouble(ReplaceValue());
				res = ValidationResult::Replaced;
			}
			return res;
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

		VarRuleMapT::const_iterator begin() { return m_RulesMap.begin(); }
		VarRuleMapT::const_iterator end()   { return m_RulesMap.end(); }

		static inline CValidationRuleBiggerThanZero BiggerThanZero;
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
