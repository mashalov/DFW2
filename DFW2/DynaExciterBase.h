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
		VariableIndexExternal EqInput;
		VariableIndexExternal GenId, GenIq, ExtVg;
		VariableIndexExternalOptional ExtUf, ExtUdec;

		CLimitedLag ExcLag;

	public:
		enum VARS
		{
			V_EQE,
			V_EQSUM,
			V_EQEV,
			V_EQELAG,
			V_LAST
		};

		enum CONSTVARS
		{
			C_REGID,
			C_DECID
		};

		double DECId, RegId;

		VariableIndex Eqe, Eqsum, EqeV, EqeLag;
		double Eqe0, Ig0, Eq0, Ug0;


		double Texc, Umin, Umax;
		bool bVoltageDependent;
		CDynaExciterBase();

		virtual ~CDynaExciterBase() = default;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;

		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;

		double GetIg();
		void SetLagTimeConstantRatio(double TexcNew);

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static const char* m_cszUf;
		static const char* m_cszUdec;
		static const char* m_cszExcConId;
		static const char* m_cszDECId;
	};
}

