#pragma once
#include "DynaGenerator1C.h"
#include "DynaExciterBase.h"
#include "LimiterConst.h"

namespace DFW2
{
	class CDynaExciterMustang : public CDynaExciterBase
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		double EqInputValue, EqOutputValue;
		CLimiterConst EqLimit;
		bool SetUpEqLimits(CDynaModel *pDynaModel, CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateInitial, CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateResulting);
	public:

		double Kig, Kif, Imin, Imax;
		CDynaExciterMustang();
		virtual ~CDynaExciterMustang() {}

		void UpdateSerializer(SerializerPtr& Serializer) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		static const CDeviceContainerProperties DeviceProperties();

	};
}
