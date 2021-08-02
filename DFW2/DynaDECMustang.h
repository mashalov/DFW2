#pragma once
#include "DynaExciterBase.h"
#include "Relay.h"
#include "RSTrigger.h"

namespace DFW2
{
	class CValidationRuleVenfOff;
	class CValidationRuleVdefOn;
	class CValidationRuleVdefOff;

	class CDynaDECMustang : public CDevice
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;

		CRelayDelay EnforceOn;
		CRelayDelay EnforceOff;
		CRelayDelay DeforceOn;
		CRelayDelay DeforceOff;
		CRSTrigger  EnfTrigger;
		CRSTrigger  DefTrigger;

		double m_dEnforceValue, m_dDeforceValue;
		
	public:
		CDynaDECMustang();
		virtual ~CDynaDECMustang() = default;


		VariableIndexExternal Vnode;
		VariableIndex Udec, EnforceOnOut, DeforceOnOut, EnforceOffOut, DeforceOffOut, EnforceTrigOut, DeforceTrigOut;
		double VEnfOn, VEnfOff, VDefOn, VDefOff, EnfRatio, DefRatio, EnfTexc, DefTexc, TdelOn, TdelOff;

		enum VARS
		{
			V_ENFONRELAY,
			V_ENFOFFRELAY,
			V_DEFONRELAY,
			V_DEFOFFRELAY,
			V_ENFTRIG,
			V_DEFTRIG,
			V_DEC,
			V_LAST
		};

		double Texc, Umin, Umax;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		bool DetectZeroCrossingFine(const CDynaPrimitive* primitive) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;

		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_cszUbf = "Ubf";
		static constexpr const char* m_cszUef = "Uef";
		static constexpr const char* m_cszUbrf = "Ubrf";
		static constexpr const char* m_cszUerf = "Uerf";
		static constexpr const char* m_cszRf = "Rf";
		static constexpr const char* m_cszRrf = "Rrf";
		static constexpr const char* m_cszTexc_f = "Texc_f";
		static constexpr const char* m_cszTexc_rf = "Texc_rf";
		static constexpr const char* m_cszTz_in = "Tz_in";
		static constexpr const char* m_cszTz_out = "Tz_out";

		static CValidationRuleVenfOff ValidatorVenfOff;
		static CValidationRuleVdefOn ValidatorVdefOn;
		static CValidationRuleVdefOff ValidatorVdefOff;
	
	};

	// VdefOn
	// VdefOff
	// VenfOff
	// VenfOn

	// проверяем напряжение снятия форсировки больше чем напряжение ввода
	class CValidationRuleVenfOff: public CValidationRuleBase
	{
	public:

		using CValidationRuleBase::CValidationRuleBase;

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CheckDevice(device);
			const CDynaDECMustang* pDEC = static_cast<const CDynaDECMustang*>(device);

			if (value->Double() < pDEC->VEnfOn)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationBiggerThanNamed, CDynaDECMustang::m_cszUbf, pDEC->VEnfOn);
				return ValidationResult::Error;
			}
			return ValidationResult::Ok;
		}
	};

	// проверяем напряжение ввода расфорсировки больше напряжения снятия расфорсировки
	class CValidationRuleVdefOn : public CValidationRuleBase
	{
	public:

		using CValidationRuleBase::CValidationRuleBase;

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CheckDevice(device);
			const CDynaDECMustang* pDEC = static_cast<const CDynaDECMustang*>(device);
			if (value->Double() < pDEC->VDefOff)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationBiggerThanNamed, CDynaDECMustang::m_cszUerf, pDEC->VDefOff);
				return ValidationResult::Error;
			}
			return ValidationResult::Ok;
		}
	};

	// проверяем напряжение снятия расфорсировки больше напряжения снятия форсировки
	class CValidationRuleVdefOff: public CValidationRuleBase
	{
	public:

		using CValidationRuleBase::CValidationRuleBase;

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CheckDevice(device);
			const CDynaDECMustang* pDEC = static_cast<const CDynaDECMustang*>(device);
			if (value->Double() < pDEC->VEnfOff)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationBiggerThanNamed, CDynaDECMustang::m_cszUef, pDEC->VEnfOff);
				return ValidationResult::Error;
			}
			return ValidationResult::Ok;
		}
	};

}

