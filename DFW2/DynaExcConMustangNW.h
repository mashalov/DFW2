﻿#pragma once
#include "DynaExciterBase.h"
#include "DerlagContinuous.h"

namespace DFW2
{
	// Версия АРВ Мустанг с апериодическим звеном с ограничениями
	// обладает более быстрой и адекватной реакцией по сранвению с оригиналом

	class CValidationRuleExcControlNwTf;
	class CDynaExcConMustangNonWindup : public CDevice
	{
	protected:

		// Масштабы коэффициентов советских АРВ
		struct GainScales
		{
			const double K1u = 0.72;
			const double K1if = 0.2;
			const double K0f = 1.3;
			const double K1f = 0.5;
		};

		// Единичные масштабы (совместимость с нормальными АРВ)
		static constexpr GainScales UnityGains = { 1.0, 1.0, 1.0, 1.0 };
		// Станадартные масштабы для (совместимость с Мустанг)
		static constexpr GainScales DefaultGains = { 0.72, 0.2, 1.3, 0.5 };


		static void ScaleGains(CDynaExcConMustangNonWindup& excon);


		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		CLimitedLag Lag;

		CDerlag dVdt;
		CDerlag dEqdt;
		CDerlag dSdt;

		VariableIndexExternal dVdtIn, dEqdtIn, dSdtIn;
		VariableIndex dVdtOut, dEqdtOut, dSdtOut;
		VariableIndex dVdtOut1, dEqdtOut1, dSdtOut1;

	public:

		enum VARS
		{
			V_UF,
			V_USUM,
			V_SVT,
			V_DVDT,
			V_EQDT = V_DVDT + 2,
			V_SDT = V_EQDT + 2,
			V_LAST = V_SDT + 2
		};

		double K0u, K1u, Alpha, K1if, K0f, K1f, Umin, Umax, Tf, Tr;
		double Vref;
		VariableIndex Uf, Usum, Svt;

		CDynaExcConMustangNonWindup();
		virtual ~CDynaExcConMustangNonWindup() = default;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel* pDynaModel) override;
		double CheckZeroCrossing(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel* pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		static CValidationRuleExcControlNwTf ValidatorTf;
	};


	class CValidationRuleExcControlNwTf : public CValidationRuleBase
	{
	public:

		CValidationRuleExcControlNwTf() : CValidationRuleBase()
		{
			replaceValue = 0.9;
		}

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			const CDynaExcConMustangNonWindup* pExcCon = static_cast<const CDynaExcConMustangNonWindup*>(device);
			if (pExcCon->K0f > 0 && pExcCon->Tf <= 0.0)
			{
				message = fmt::format(CDFW2Messages::m_cszValidationTfOfMustangExcCon, pExcCon->K0f);
				return ReplaceValue(value);
			}
			return ValidationResult::Ok;
		}
	};
}


