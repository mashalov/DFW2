#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorMotion.h"

namespace DFW2
{
	class CDynaGenerator1C : public CDynaGeneratorMotion
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		cplx GetXofEqs() override { return cplx(0, xq); };
		double zsq;
	public:

		enum CONSTVARS
		{
			C_EXCITERID = CDynaGeneratorMotion::CONSTVARS::C_LAST,
			C_EQNOM,
			C_SNOM,
			C_QNOM,
			C_INOM,
			C_EQE,
			C_LAST

		};

		enum VARS
		{
			V_VD = CDynaGeneratorMotion::V_LAST,
			V_VQ,
			V_ID,
			V_IQ,
			V_EQS,
			V_EQ,
			V_LAST
		};

		cplx m_Egen; // эквивалентная ЭДС генератора для учета явнополюсности в стартовом методе
					 // используется всеми dq генераторами

		VariableIndex Vd, Vq, Id, Iq, Eq;
		VariableIndexExternalOptional ExtEqe;

		double	Td01, xd, r;

		double Eqnom, Snom, Qnom, Inom;
		double m_ExciterId;

		void IfromDQ();
		bool BuildIfromDQEquations(CDynaModel *pDynaModel);
		bool BuildIfromDQRightHand(CDynaModel *pDynaModel);
		
		CDynaGenerator1C();
		virtual ~CDynaGenerator1C() {}

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		double Xgen() override;
		cplx Igen(ptrdiff_t nIteration) override;
		virtual const cplx& CalculateEgen();

		void UpdateSerializer(DeviceSerializerPtr& Serializer) override;

		static constexpr const char* m_cszEqe = "Eqe";
		static constexpr const char* m_cszEq = "Eq";
		static constexpr const char* m_cszId = "Id";
		static constexpr const char* m_cszIq = "Iq";
		static constexpr const char* m_cszExciterId = "ExciterId";
		static constexpr const char* m_cszEqnom = "Eqnom";
		static constexpr const char* m_cszSnom = "Snom";
		static constexpr const char* m_cszInom = "Inom";
		static constexpr const char* m_cszQnom = "Qnom";

		static void DeviceProperties(CDeviceContainerProperties& properties);

	};
}

