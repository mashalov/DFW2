#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorInfBus.h"

namespace DFW2
{
	class CValidationRuleGeneratorUnom;
	class CValidationRuleGeneratorMj;
	class CValidationRuleGeneratorPnom;
	class CDynaGeneratorMotion : public CDynaGeneratorInfBusBase
	{
	protected:
		 eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		 eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		 void CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn) override;
	public:
		enum VARS
		{
			V_S = CDynaGeneratorInfBusBase::V_LAST,
			V_DELTA,
			V_LAST
		};

		enum CONSTVARS
		{
			C_UNOM = CDynaGeneratorInfBusBase::CONSTVARS::C_LAST,
			C_LAST
		};
		
		VariableIndex s;

		double Eqsxd1, Unom, Kdemp, xq, Mj, Pt, Pnom, cosPhinom, deltaDiff = 0.0;

		using CDynaGeneratorInfBusBase::CDynaGeneratorInfBusBase;
		virtual ~CDynaGeneratorMotion() = default;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		static double ZeroGuardSlip(double Omega) { return (Omega > 0) ? Omega : Consts::epsilon; }
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		void BuildAngleEquationBlock(CDynaModel* pDynaModel);
		void BuildAngleEquation(CDynaModel* pDynaModel, CDevice::fnDerivative fn);
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_cszKdemp = "Kdemp";
		static constexpr const char* m_cszxq = "xq";
		static constexpr const char* m_cszMj = "Mj";
		static constexpr const char* m_cszPnom = "Pnom";
		static constexpr const char* m_cszUnom = "Unom";
		static constexpr const char* m_cszcosPhinom = "cosPhinom";

		static CValidationRuleGeneratorUnom ValidatorUnom;
		static CValidationRuleGeneratorPnom ValidatorPnom;
		static CValidationRuleGeneratorMj ValidatorMj;
		static inline CValidationRuleRange ValidatorCos = { -1, 1 };
	};



	class CValidationRuleGeneratorUnom : public CValidationRuleBase
	{
	public:

		using CValidationRuleBase::CValidationRuleBase;

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CheckDevice(device);
			const CDynaNodeBase* pNode = static_cast<const CDynaNodeBase*>(device->GetSingleLink(0));
			CheckDevice(pNode);
			const CDynaGeneratorMotion* pGen = static_cast<const CDynaGeneratorMotion*>(device);

			if (pNode && (pGen->Unom > pNode->Unom * 1.15 || pGen->Unom < pNode->Unom * 0.85))
			{
				message = fmt::format(CDFW2Messages::m_cszUnomMismatch, pNode->GetVerbalName(), pNode->Unom);
				return ValidationResult::Warning;
			}
			return ValidationResult::Ok;
		}
	};

	class CValidationRuleGeneratorMj : public CValidationRuleBase
	{
	public:

		using CValidationRuleBase::CValidationRuleBase;

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CheckDevice(device);
			const CDynaGeneratorMotion* pGen = static_cast<const CDynaGeneratorMotion*>(device);
			if (pGen->Pnom > 0)
			{
				if (pGen->Mj / pGen->Pnom < 0.01)
				{
					message = fmt::format(CDFW2Messages::m_cszValidationSuspiciousLow);
					return ValidationResult::Warning;
				}
			}
			return ValidationResult::Ok;
		}
	};

	class CValidationRuleGeneratorPnom : public CValidationRuleBase
	{
	public:

		using CValidationRuleBase::CValidationRuleBase;

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CheckDevice(device);
			const CDynaGeneratorMotion* pGen = static_cast<const CDynaGeneratorMotion*>(device);
			const cplx Slf{ pGen->P, pGen->Q };
			const double Srated = 1.05 * (Consts::Equal(pGen->cosPhinom, 0.0) ? pGen->Pnom : pGen->Pnom / pGen->cosPhinom);
			// предполагаем что прошел валидатор Kgen
			const double absSlf(std::abs(Slf) / pGen->Kgen);

			if (absSlf > Srated)
			{
				const std::string perCents(Srated > 0 ? fmt::format("{:.1f}", absSlf / Srated * 100.0) : "???");
				message = fmt::format(CDFW2Messages::m_cszGeneratorPowerExceedsRated, Slf, absSlf, Srated, perCents);
				return ValidationResult::Warning;
			}
			return ValidationResult::Ok;
		}
	};
}

