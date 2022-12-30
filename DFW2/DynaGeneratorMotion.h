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
		 eDEVICEFUNCTIONSTATUS GetConnection(CDynaModel* pDynaModel) override;
		 double Eqsxd1;
	public:
		enum VARS
		{
			V_S = CDynaGeneratorInfBusBase::V_LAST,
			V_DELTA,
			V_LAST
		};

		enum CONSTVARS
		{
			C_QNOM = CDynaGeneratorInfBusBase::CONSTVARS::C_LAST,		// номинальная реактивная мощность
			C_INOM,														// номинальный ток
			C_LAST
		};
		
		VariableIndex s;

		double Kdemp, xq, Mj, Pt, Pnom, cosPhinom, Qnom, Inom, deltaDiff = 0.0;

		using CDynaGeneratorInfBusBase::CDynaGeneratorInfBusBase;
		virtual ~CDynaGeneratorMotion() = default;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		static double ZeroGuardSlip(double Omega) { return (Omega > 0) ? Omega : DFW2_EPSILON; }
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		void BuildAngleEquationBlock(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_cszKdemp = "Kdemp";
		static constexpr const char* m_cszxq = "xq";
		static constexpr const char* m_cszMj = "Mj";
		static constexpr const char* m_cszPnom = "Pnom";
		static constexpr const char* m_cszcosPhinom = "cosPhinom";
		static constexpr const char* m_cszInom = "Inom";
		static constexpr const char* m_cszQnom = "Qnom";

		static CValidationRuleGeneratorUnom ValidatorUnom;
		static CValidationRuleGeneratorPnom ValidatorPnom;
		static CValidationRuleGeneratorMj ValidatorMj;
		static inline CValidationRuleRange ValidatorCos = { -1, 1 };
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
			const double Srated = 1.05 * (Equal(pGen->cosPhinom, 0.0) ? pGen->Pnom : pGen->Pnom / pGen->cosPhinom);
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

