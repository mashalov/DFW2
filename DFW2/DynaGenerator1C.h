#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorMotion.h"

namespace DFW2
{
	class CDynaGenerator1C : public CDynaGeneratorMotion
	{
	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual cplx GetXofEqs() { return cplx(0,xq); }
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

		double Vd, Vq;
		double Id, Iq;
		double Eq;

		double Eqe;
		PrimitiveVariableExternal ExtEqe;

		double	Td01, xd, r;

		double Eqnom, Snom, Qnom, Inom;
		double m_ExciterId;

		void IfromDQ();
		bool BuildIfromDQEquations(CDynaModel *pDynaModel);
		bool BuildIfromDQRightHand(CDynaModel *pDynaModel);
		
		CDynaGenerator1C();
		virtual ~CDynaGenerator1C() {}

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		virtual bool CalculatePower() override;
		virtual double Xgen();
		virtual cplx Igen(ptrdiff_t nIteration);
		virtual const cplx& CalculateEgen();

		virtual void UpdateSerializer(SerializerPtr& Serializer);

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

