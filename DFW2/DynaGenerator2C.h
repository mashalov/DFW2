#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGenerator1C.h"

namespace DFW2
{
	class CDynaGenerator2C : public CDynaGenerator1C
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		void CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn) override;
		cplx GetIdIq() const;
	public:

		enum VARS
		{
			V_EDS = CDynaGenerator1C::V_LAST,
			V_LAST
		};

		VariableIndex Eds;

		const cplx& CalculateEgen() override;
		bool CalculatePower() override;

		using CDynaGenerator1C::CDynaGenerator1C;
		virtual ~CDynaGenerator2C() = default;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
		cplx GetEMF() override;
		static constexpr const char* m_cszEds = "Eds";
	};
}


