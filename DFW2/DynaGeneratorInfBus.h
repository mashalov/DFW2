#pragma once
#include "DeviceContainer.h"
#include "DynaVoltageSource.h"

namespace DFW2
{
	class CDynaGeneratorInfBusBase : public CDynaVoltageSource
	{
	protected:
		void SetUpDelta();
		cplx ZgenNet_;			// сопротивление приведенное к pu сети
		cplx ZgenInternal_;		// сопротивление в pu генератора
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
	public:
		VariableIndex Delta, Eqs;
		double xd1;
		double	r = 0.0;	// активное сопротивление статора
		using CDynaVoltageSource::CDynaVoltageSource;
		virtual ~CDynaGeneratorInfBusBase() = default;
		const cplx& Zgen() const { return ZgenNet_; }
		virtual cplx Igen(ptrdiff_t nIteration);
		cplx GetEMF() override { return std::polar((double)Eqs, (double)Delta); }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		void CalculatePower() override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;

		static constexpr const char* m_cszr = "r";
		static constexpr const char* m_cszxd1 = "xd1";
		static constexpr const char* m_cszEqs = "Eqs";
	};
	
	class CDynaGeneratorInfBus : public CDynaGeneratorInfBusBase
	{
	protected:
		double EqsCos, EqsSin;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
	public:
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
	};
}

