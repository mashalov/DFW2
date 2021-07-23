#pragma once
#include "DynaGeneratorDQBase.h"

namespace DFW2
{
	class CDynaGeneratorPark3C : public CDynaGeneratorDQBase
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		cplx GetIdIq() const;
	public:
		using  CDynaGeneratorDQBase::CDynaGeneratorDQBase;
		virtual ~CDynaGeneratorPark3C() = default;

		enum VARS
		{
			V_PSI_FD= CDynaGeneratorDQBase::V_LAST,
			V_PSI_1D,
			V_PSI_1Q,
			V_PSI_2Q,
			V_LAST
		};

		VariableIndex Psifd, Psi1d, Psi1q, Psi2q;
		double xd2, xq1, xq2, xl, Td01, Tq01, Td02, Tq02;




		// коэффициенты уравнений

		double lad, laq, lrc, lfd, Rfd;

		double Ed_Psi1q, Ed_Psi2q;
		double Eq_Psifd, Eq_Psi1d;
		double Psifd_Psifd, Psifd_Psi1d, Psifd_id;
		double Psi1d_Psifd, Psi1d_Psi1d, Psi1d_id;
		double Psi1q_Psi1q, Psi1q_Psi2q, Psi1q_iq;
		double Psi2q_Psi1q, Psi2q_Psi2q, Psi2q_iq;

		double Psid_id, Psid_Psifd, Psid_Psi1d;
		double Psiq_iq, Psiq_Psi1q, Psiq_Psi2q;

		double lq2, ld2;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void CalculateFundamentalParameters();
		bool BuildDerivatives(CDynaModel* pDynaModel) override;
		bool CalculatePower() override;
		const cplx& CalculateEgen() override;
		double Xgen() const override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		/*
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel * pDynaModel) override;
		*/

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_cszxl	= "xl";
		static constexpr const char* m_csztq01	= "tq01";

	};
}

