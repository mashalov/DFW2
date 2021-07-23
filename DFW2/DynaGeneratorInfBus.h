#pragma once
#include "DeviceContainer.h"
#include "DynaVoltageSource.h"

namespace DFW2
{
	class CDynaGeneratorInfBusBase : public CDynaVoltageSource
	{
	protected:
		bool SetUpDelta();
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
	public:
		virtual cplx GetXofEqs() { return cplx(0,xd1); }
		VariableIndex Delta, Eqs;
		double xd1;
		double	r = 0.0;	// активное сопротивление статора
		using CDynaVoltageSource::CDynaVoltageSource;
		virtual ~CDynaGeneratorInfBusBase() = default;
		virtual double Xgen() const;
		virtual cplx Igen(ptrdiff_t nIteration);
		cplx GetEMF() override { return std::polar((double)Eqs, (double)Delta); }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
	};

	class CDynaGeneratorInfBus : public CDynaGeneratorInfBusBase
	{
	protected:
		double EqsCos, EqsSin;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
	public:
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
	};
}

