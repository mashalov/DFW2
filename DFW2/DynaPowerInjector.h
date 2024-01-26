#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaPrimitive.h"

namespace DFW2
{
	class CValidationRuleGeneratorKgen;
	class CValidationRuleGeneratorUnom;

	class CDynaPowerInjector : public CDevice
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		virtual eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel);
		VariableIndexExternal V, DeltaV, Vre, Vim, Sv, Scoi, Dcoi;
		cplx Ynorton_, Ygen_, Zgen_;
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
		double Unom;
		double NodeId;


		using CDevice::CDevice; 
		virtual ~CDynaPowerInjector() = default;

		virtual bool CalculatePower();
		// комплекс шунта Нортона в узле подключения	
		virtual const cplx& Ynorton() const { return Ynorton_; }
		inline const cplx& Ygen() const { return Ygen_; }
		inline const cplx& Zgen() const { return Zgen_; }
		virtual cplx Igen(ptrdiff_t nIteration) { return 0.0; }

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
		static constexpr const char* cszP_ = "P";
		static constexpr const char* cszQ_ = "Q";
		static constexpr const char* cszIre_ = "Ire";
		static constexpr const char* cszIim_ = "Iim";
		static constexpr const char* cszNodeId_ = "NodeId";
		static constexpr const char* cszKgen_ = "Kgen";
		static constexpr const char* cszQmin_ = "Qmin";
		static constexpr const char* cszQmax_ = "Qmax";
		static constexpr const char* cszUnom_ = "Unom";
		static constexpr const char* cszQnom_ = "Qnom";
		static constexpr const char* cszSnom_ = "Snom";
		static constexpr const char* cszVref_ = "Vref";
		static constexpr const char* cszAliasGenerator_ = "Generator";

		static CValidationRuleGeneratorKgen ValidatorKgen;
		static CValidationRuleGeneratorUnom ValidatorUnom;
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
			const CDynaPowerInjector* pGen{ static_cast<const CDynaPowerInjector*>(device) };

			if (pNode && (pGen->Unom > pNode->Unom * 1.15 || pGen->Unom < pNode->Unom * 0.85))
			{
				message = fmt::format(CDFW2Messages::m_cszUnomMismatch, pNode->GetVerbalName(), pNode->Unom);
				return ValidationResult::Warning;
			}
			return ValidationResult::Ok;
		}
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

