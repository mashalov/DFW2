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
		void CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn) override;
	public:
		using  CDynaGeneratorDQBase::CDynaGeneratorDQBase;
		virtual ~CDynaGeneratorPark3C() = default;

		enum VARS
		{
			V_PSI_FD= CDynaGeneratorDQBase::V_LAST,
			V_PSI_1D,
			V_PSI_1Q,
			V_LAST
		};

		VariableIndex Psifd, Psi1d, Psi1q;

		// коэффициенты уравнений

		double lad, laq, lrc, lfd, Rfd;

		double Ed_Psi1q;
		double Eq_Psifd, Eq_Psi1d;
		double Psifd_Psifd, Psifd_Psi1d, Psifd_id;
		double Psi1d_Psifd, Psi1d_Psi1d, Psi1d_id;
		double Psi1q_Psi1q, Psi1q_iq;

		double Psid_Psifd, Psid_Psi1d;
		double Psiq_Psi1q;

		double lq2, ld2;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel* pDynaModel) override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		bool CalculateFundamentalParameters(PARK_PARAMETERS_DETERMINATION_METHOD Method);
		void CalculatePower() override;
		cplx GetEMF() override;
		const cplx& CalculateEgen() override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;

		/*
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel * pDynaModel) override;
		*/

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static bool GetCanayTimeConstants(double Xa, double X1s, double X2s, double Xrc, double& T1, double& T2);


		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xq2> ValidatorXqXq2 = { CDynaGeneratorDQBase::m_cszxq2 };

		static constexpr const char* m_cszxl	= "xl";
		static constexpr const char* m_cszPsifd = "Psifd";
		static constexpr const char* m_cszPsi1d = "Psi1d";
		static constexpr const char* m_cszPsi1q = "Psi1q";
		static constexpr const char* m_cszPsi2q = "Psi2q";
	};
}


