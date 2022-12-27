#pragma once
#include "DeviceContainer.h"
#include "DynaVoltageSource.h"

namespace DFW2
{
	class CDynaGeneratorInfBusBase : public CDynaVoltageSource
	{
	protected:
		bool SetUpDelta();
		cplx Zgen_;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
	public:
		virtual double GetXofEqs() const{ return xd1; }
		VariableIndex Delta, Eqs;
		double Eqsxd1;
		double xd1;
		double	r = 0.0;	// активное сопротивление статора
		using CDynaVoltageSource::CDynaVoltageSource;
		virtual ~CDynaGeneratorInfBusBase() = default;
		const cplx& Zgen() const;
		virtual cplx Igen(ptrdiff_t nIteration);
		cplx GetEMF() override { return std::polar((double)Eqs, (double)Delta); }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
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

