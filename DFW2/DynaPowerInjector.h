#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaPrimitive.h"

namespace DFW2
{
	class CValidationRuleGeneratorKgen;

	class CDynaPowerInjector : public CDevice
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		virtual eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel);
		VariableIndexExternal V, DeltaV, Vre, Vim, Sv, Scoi, Dcoi;
		cplx Ynorton_;
	public:
		enum VARS
		{
			V_IRE,
			V_IIM,
			V_P,
			V_Q,
			V_LAST
		};

		enum CONSTVARS
		{
			C_NODEID,
			C_SYNCDELTA,
			C_LAST
		};


		VariableIndex Ire, Iim, P, Q;

		double SyncDelta_;

		double Kgen;
		double LFQmin;
		double LFQmax;

		double NodeId;

		using CDevice::CDevice; 
		virtual ~CDynaPowerInjector() = default;

		virtual bool CalculatePower();
		// комплекс шунта Нортона в узле подключения	
		virtual const cplx& Ynorton() const { return Ynorton_; }

		eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice = nullptr) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
		static constexpr const char* m_cszP = "P";
		static constexpr const char* m_cszQ = "Q";
		static constexpr const char* m_cszIre = "Ire";
		static constexpr const char* m_cszIim = "Iim";
		static constexpr const char* m_cszNodeId = "NodeId";
		static constexpr const char* m_cszKgen = "Kgen";

		static CValidationRuleGeneratorKgen ValidatorKgen;
	};

	class CValidationRuleGeneratorKgen : public CValidationRuleBiggerThanZero
	{
	public:
		CValidationRuleGeneratorKgen() : CValidationRuleBiggerThanZero()
		{
			DefaultResult = ValidationResult::Warning;
			replaceValue = 1.0;
		}
	};
}

