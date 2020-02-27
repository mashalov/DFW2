#pragma once
#include "DynaGenerator1C.h"
#include "LimitedLag.h"
#include "Sum.h"
using namespace std;

namespace DFW2
{
	class CDynaDECMustang;
	class CDynaExcConMustang;

	class CDynaExciterBase : public CDevice
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;

		PrimitiveVariable PvEqsum;
		PrimitiveVariableExternal GenId, GenIq, ExtUf, ExtUdec, ExtVg, EqInput;
		CLimitedLag ExcLag;

	public:
		enum VARS
		{
			V_EQE,
			V_EQSUM,
			V_EQEV,
			V_LAST
		};

		enum CONSTVARS
		{
			C_REGID,
			C_DECID
		};

		double DECId, RegId;

		double Eqe, Uexc, Eqsum, EqeV, Udec;
		double Eqe0, Ug0, Ig0, Eq0;


		double Texc, Umin, Umax;
		bool bVoltageDependent;
		CDynaExciterBase();

		virtual ~CDynaExciterBase() { }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;

		double GetIg();
		void SetLagTimeConstantRatio(double TexcNew);

		static const CDeviceContainerProperties DeviceProperties();

		static const _TCHAR *m_cszUf;
		static const _TCHAR *m_cszUdec;
		static const _TCHAR *m_cszExcConId;
		static const _TCHAR *m_cszDECId;
	};
}

