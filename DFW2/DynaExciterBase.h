#pragma once
#include "DynaGenerator1C.h"
#include "LimitedLag.h"
#include "Sum.h"

namespace DFW2
{
	class CDynaDECMustang;
	class CDynaExcConMustang;

	class CDynaExciterBase : public CDevice
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		PrimitiveVariableExternal EqInput;
		VariableIndexExternal GenId, GenIq, ExtVg;
		VariableIndexExternalOptional ExtUf, ExtUdec;

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

		VariableIndex Eqe, Eqsum;
		double Eqe0, Ig0, Eq0, Ug0;


		double Texc, Umin, Umax;
		bool bVoltageDependent;
		CDynaExciterBase();

		virtual ~CDynaExciterBase() { }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;

		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;

		double GetIg();
		void SetLagTimeConstantRatio(double TexcNew);

		static const CDeviceContainerProperties DeviceProperties();

		static const _TCHAR *m_cszUf;
		static const _TCHAR *m_cszUdec;
		static const _TCHAR *m_cszExcConId;
		static const _TCHAR *m_cszDECId;
	};
}

