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
			V_VD = 4,
			V_VQ,
			V_ID,
			V_IQ,
			V_EQS,
			V_EQ,
			V_LAST
		};

		double Vd, Vq;
		double Id, Iq;
		double Eq;

		double Eqe;
		PrimitiveVariableExternal ExtEqe;

		double	Td01, xd, r;

		double Eqnom, Snom, Qnom, Inom;
		double m_ExciterId;
		
		CDynaGenerator1C();
		virtual ~CDynaGenerator1C() {}

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual bool InitExternalVariables(CDynaModel *pDynaModel);
		virtual bool CalculatePower();

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

