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

		cplx m_Egen; // эквивалентна€ Ёƒ— генератора дл€ учета €внополюсности в стартовом методе
					 // используетс€ всеми dq генераторами

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
		VariableIndexVec& GetVariables(VariableIndexVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		double Xgen() override;
		cplx Igen(ptrdiff_t nIteration) override;
		virtual const cplx& CalculateEgen();

		void UpdateSerializer(SerializerPtr& Serializer) override;

		static const _TCHAR *m_cszEqe;
		static const _TCHAR *m_cszEq;
		static const _TCHAR *m_cszId;
		static const _TCHAR *m_cszIq;
		static const _TCHAR *m_cszExciterId;
		static const _TCHAR *m_cszEqnom;
		static const _TCHAR *m_cszSnom;
		static const _TCHAR *m_cszInom;
		static const _TCHAR *m_cszQnom;

		static const CDeviceContainerProperties DeviceProperties();

	};
}

