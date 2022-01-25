#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorDQBase.h"

namespace DFW2
{
	class CDynaGenerator1C : public CDynaGeneratorDQBase
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		double zsq;
	public:

		enum VARS
		{
			V_EQS = CDynaGeneratorDQBase::V_LAST,
			V_LAST
		};

		const cplx& CalculateEgen() override;
		bool CalculatePower() override;

		using CDynaGeneratorDQBase::CDynaGeneratorDQBase;
		virtual ~CDynaGenerator1C() = default;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}

