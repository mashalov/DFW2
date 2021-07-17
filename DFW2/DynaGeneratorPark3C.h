#pragma once
#include "DynaGeneratorDQBase.h"

namespace DFW2
{
	class CDynaGeneratorPark3C : public CDynaGeneratorDQBase
	{
	protected:
		//eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
	public:
		using  CDynaGeneratorDQBase::CDynaGeneratorDQBase;
		virtual ~CDynaGeneratorPark3C() = default;

		enum VARS
		{
			V_PSI_D= CDynaGeneratorDQBase::V_LAST,
			V_PSI_Q,
			V_PSI_FD,
			V_LAST
		};

		VariableIndex Psid, Psiq, Psifd;
		double xd, xd2, xq1, xq2, xl, Td01, Tq01, Td02, Tq02;


		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void CalculateFundamentalParameters();

		/*
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		bool BuildDerivatives(CDynaModel * pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel * pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel * pDynaModel) override;
		bool CalculatePower() override;
		const cplx& CalculateEgen() override;
		static void DeviceProperties(CDeviceContainerProperties & properties);
		*/

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_cszxl	= "xl";
		static constexpr const char* m_csztq01	= "tq01";

	};
}

