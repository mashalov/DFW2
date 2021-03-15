#pragma once
#include "DeviceContainer.h"
#include "DynaVoltageSource.h"

namespace DFW2
{
	class CDynaGeneratorInfBusBase : public CDynaVoltageSource
	{
	protected:
		bool SetUpDelta();
	public:
		virtual cplx GetXofEqs() { return cplx(0,xd1); }
		VariableIndex Delta, Eqs;
		double xd1;
		CDynaGeneratorInfBusBase();
		virtual double Xgen();
		virtual cplx Igen(ptrdiff_t nIteration);
		virtual ~CDynaGeneratorInfBusBase() {}
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		cplx GetEMF() override { return std::polar((double)Eqs, (double)Delta); }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		void UpdateSerializer(SerializerPtr& Serializer) override;
		static const CDeviceContainerProperties DeviceProperties();
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
	};

	class CDynaGeneratorInfBus : public CDynaGeneratorInfBusBase
	{
	protected:
		double EqsCos, EqsSin;
	public:
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		void UpdateSerializer(SerializerPtr& Serializer) override;
	};
}

