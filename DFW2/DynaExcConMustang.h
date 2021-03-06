#pragma once
#include "DynaExciterBase.h"
#include "DerlagContinuous.h"
#include "LimiterConst.h"

namespace DFW2
{
	// Версия АРВ Мустанг - точная копия модели из Мустанг
	// с ограничениями перед апериодическим звеном

	class CDynaExcConMustang : public CDevice
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

		double Eqnom_ = 0.0;

		// Единичные масштабы (совместимость с нормальными АРВ)
		static constexpr GainScales UnityGains = { 1.0, 1.0, 1.0, 1.0 };
		// Станадартные масштабы для (совместимость с Мустанг)
		static constexpr GainScales DefaultGains = { 0.72, 0.2, 1.3, 0.5 };


		static void ScaleGains(CDynaExcConMustang& excon);


		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		CLimiterConst Limiter;

		CDerlagContinuousSmooth dVdt, dEqdt, dSdt;

		VariableIndexExternal dVdtIn, dEqdtIn, dSdtIn;
		VariableIndex dVdtOut, dEqdtOut, dSdtOut;
		VariableIndex dVdtOut1, dEqdtOut1, dSdtOut1;

	public:

		enum VARS
		{
			V_UF,
			V_USUM,
			V_USUMLMT,
			V_SVT,
			V_DVDT,
			V_EQDT = V_DVDT + 2,
			V_SDT = V_EQDT + 2,
			V_LAST = V_SDT + 2
		};

		double K0u, K1u, Alpha, K1if, K0f, K1f, Umin, Umax, Tf, Tr;
		double Vref;
		VariableIndex Uf, Usum, UsumLmt, Svt;

		CDynaExcConMustang();
		virtual ~CDynaExcConMustang() = default;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_cszAlpha = "Alpha";
		static constexpr const char* m_cszTrv = "Trv";
		static constexpr const char* m_cszKu = "Ku";
		static constexpr const char* m_cszKu1 = "Ku1";
		static constexpr const char* m_cszKif1 = "Kif1";
		static constexpr const char* m_cszKf = "Kf";
		static constexpr const char* m_cszKf1 = "Kf1";
		static constexpr const char* m_cszTf = "Tf";
		static constexpr const char* m_cszUrv_min = "Urv_min";
		static constexpr const char* m_cszUrv_max = "Urv_max";

		static inline CValidationRuleRange ValidatorAlpha = CValidationRuleRange(-1,1);
		static inline CValidationRuleBiggerThanZero ValidatorTf = CValidationRuleBiggerThanZero(0.9);
	};
}

