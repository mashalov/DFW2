#pragma once
#include "DynaExciterBase.h"
#include "Relay.h"
#include "RSTrigger.h"

namespace DFW2
{
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
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		bool DetectZeroCrossingFine(const CDynaPrimitive* primitive) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;

		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* cszUbf_ = "Ubf";
		static constexpr const char* cszUef_ = "Uef";
		static constexpr const char* cszUbrf_ = "Ubrf";
		static constexpr const char* cszUerf_ = "Uerf";
		static constexpr const char* cszRf_ = "Rf";
		static constexpr const char* cszRrf_ = "Rrf";
		static constexpr const char* cszTexc_f_ = "Texc_f";
		static constexpr const char* cszTexc_rf_ = "Texc_rf";
		static constexpr const char* cszTz_in_ = "Tz_in";
		static constexpr const char* cszTz_out_ = "Tz_out";
		static constexpr const char* cszEnfOnRly_ = "EnfOnRly";
		static constexpr const char* cszDefOnRly_ = "DefOnRly";
		static constexpr const char* cszEnfOffRly_ = "EnfOffRly";
		static constexpr const char* cszDefOffRly_ = "DefOffRly";
		static constexpr const char* cszEnfTrig_ = "EnfTrg";
		static constexpr const char* cszDefTrig_ = "DefTrg";

		// допускаются равные уставки на ввод и снятие форсировки/дефорсировки (см. приоритет триггера)
		static inline CValidationRuleBiggerOrEqualT<CDynaDECMustang, &CDynaDECMustang::VEnfOn> ValidatorVenfOff = { CDynaDECMustang::cszUbf_ };
		static inline CValidationRuleBiggerOrEqualT<CDynaDECMustang, &CDynaDECMustang::VDefOff> ValidatorVdefOn = { CDynaDECMustang::cszUerf_ };
		// полосы форсировки/расфорсировки не должны пересекаться
		static inline CValidationRuleBiggerT<CDynaDECMustang, &CDynaDECMustang::VEnfOff> ValidatorVdefOff = { CDynaDECMustang::cszUef_ };

	};
}

