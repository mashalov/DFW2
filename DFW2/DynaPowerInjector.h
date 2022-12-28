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
		virtual eDEVICEFUNCTIONSTATUS GetConnection(CDynaModel* pDynaModel);

		VariableIndexExternal V, DeltaV, Vre, Vim, Sv;
		cplx Ynorton_;
		double NodeUnom_ = 0.0;
		double Snom_ = 0.0;
		double Unom_ = 0.0;
		double puV_ = 1.0; // трансформатор напряжения из pu сети в pu модели
		double puI_ = 1.0; // трансформатор тока из pu модели в pu сети 
	public:
		enum VARS
		{
			V_IRE,
			V_IIM,
			V_LAST
		};

		enum CONSTVARS
		{
			C_NODEID,													// номер узла
			C_UNOM,														// номинальное напряжение
			C_SNOM,														// номинальная полная мощность
			C_P,														// исходная активная мощность
			C_Q,														// исходная реактивная мощность
			C_LAST
		};


		VariableIndex Ire, Iim;

		double P, Q;

		double Kgen;
		double LFQmin;
		double LFQmax;

		double NodeId;

		using CDevice::CDevice; 
		virtual ~CDynaPowerInjector() = default;

		virtual bool CalculatePower();
		void FinishStep(const CDynaModel& DynaModel) override;

		// комплекс шунта Нортона в узле подключения	
		virtual const cplx& Ynorton() const { return Ynorton_; }

		virtual double Unom() const { return Unom_; }
		double kI() const { return puI_; }

		eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice = nullptr) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
		static constexpr const char* m_cszP = "P";
		static constexpr const char* m_cszQ = "Q";
		static constexpr const char* m_cszSnom = "Snom";
		static constexpr const char* m_cszIre = "Ire";
		static constexpr const char* m_cszIim = "Iim";
		static constexpr const char* m_cszNodeId = "NodeId";
		static constexpr const char* m_cszKgen = "Kgen";

		static CValidationRuleGeneratorKgen ValidatorKgen;
	};

	class CValidationRuleGeneratorUnom : public CValidationRuleBase
	{
	public:

		using CValidationRuleBase::CValidationRuleBase;

		ValidationResult Validate(MetaSerializedValue* value, CDevice* device, std::string& message) const override
		{
			CheckDevice(device);
			const CDynaNodeBase* pNode { static_cast<const CDynaNodeBase*>(device->GetSingleLink(0))};
			CheckDevice(pNode);
			const CDynaPowerInjector* pGen{ static_cast<const CDynaPowerInjector*>(device) };

			if (pNode && (pGen->Unom() > pNode->Unom * 1.15 || pGen->Unom() < pNode->Unom * 0.85))
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

