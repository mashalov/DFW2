#pragma once
#include "DynaGenerator1C.h"
#include "DynaExciterBase.h"
#include "LimiterConst.h"
using namespace std;

namespace DFW2
{
	class CDynaExciterMustang : public CDynaExciterBase
	{
	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		double EqInputValue, EqOutputValue;
		CLimiterConst EqLimit;
		bool SetUpEqLimits(CDynaModel *pDynaModel, CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateInitial, CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateResulting);
	public:

		double Kig, Kif, Imin, Imax;
		CDynaExciterMustang();
		virtual ~CDynaExciterMustang() {}

		virtual void UpdateSerializer(SerializerPtr& Serializer) override;
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		static const CDeviceContainerProperties DeviceProperties();

	};
}
