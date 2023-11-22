#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGenerator1C.h"

namespace DFW2
{
	// Federico Milano. 2010. Power System Modelling and Scripting DOI 10.1007/978-3-642-13669-6
	// p.340 One d-and One q-Axis Model (current components have inverse signs due to 
	// reverse current positive direction)
	// Modelica reference https://github.com/OpenIPSL/OpenIPSL/blob/master/OpenIPSL/Electrical/Machines/PSAT/Order4.mo

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


