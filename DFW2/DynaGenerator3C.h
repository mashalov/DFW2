#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGenerator1C.h"

namespace DFW2
{
	class CDynaGenerator3C : public CDynaGenerator1C
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
	public:

		enum VARS
		{
			V_EQSS = CDynaGenerator1C::V_LAST,
			V_EDSS,
			V_LAST
		};

		VariableIndex Eqss, Edss;

		using CDynaGenerator1C::CDynaGenerator1C;
		virtual ~CDynaGenerator3C() = default;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;
		cplx GetEMF() override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		const cplx& CalculateEgen() override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}
