#pragma once
#include "DynaExciterBase.h"
#include "DerlagContinuous.h"

namespace DFW2
{

	class CDynaExcConMustang : public CDevice
	{

	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		CLimitedLag Lag;

		CDerlag dVdt;
		CDerlag dEqdt;
		CDerlag dSdt;

		PrimitiveVariable LagIn;

		PrimitiveVariableExternal dVdtIn;
		PrimitiveVariableExternal dEqdtIn;
		PrimitiveVariableExternal dSdtIn;

		VariableIndex dVdtOutValue[2], dEqdtOutValue[2], dSdtOutValue[2];

	public:

		enum VARS
		{
			V_UF,
			V_USUM,
			V_SVT,
			V_DVDT,
			V_EQDT = V_DVDT + 2,
			V_SDT = V_EQDT + 2,
			V_LAST = V_SDT + 2
		};

		double K0u, K1u, Alpha, K1if, K0f, K1f, Umin, Umax, Tf, Tr;
		double Vref;
		VariableIndex Uf, Usum, Svt;

		CDynaExcConMustang();
		virtual ~CDynaExcConMustang() {}
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;

		void UpdateSerializer(SerializerPtr& Serializer) override;

		static const CDeviceContainerProperties DeviceProperties();
	};
}

