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
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);

		PrimitiveVariable PvEqsum;
		PrimitiveVariableExternal GenId, GenIq, ExtUf, ExtUdec, ExtVg, EqInput;
		CLimitedLag ExcLag;
		CPrimitives<1> Prims;

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
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);

		double GetIg();
		void SetLagTimeConstantRatio(double TexcNew);

		static const CDeviceContainerProperties DeviceProperties();

		static const _TCHAR *m_cszUf;
		static const _TCHAR *m_cszUdec;
		static const _TCHAR *m_cszExcConId;
		static const _TCHAR *m_cszDECId;
	};
}

