#pragma once
#include "DynaExciterBase.h"
#include "Relay.h"
#include "RSTrigger.h"

using namespace std;

namespace DFW2
{
	class CDynaDECMustang : public CDevice
	{
	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);

		CRelayDelay EnforceOn;
		CRelayDelay EnforceOff;
		CRelayDelay DeforceOn;
		CRelayDelay DeforceOff;
		CRSTrigger  EnfTrigger;
		CRSTrigger  DefTrigger;

		double EnforceOnValue, DeforceOnValue;
		double EnforceOffValue, DeforceOffValue;
		double EnforceTrigValue, DeforceTrigValue;

		double m_dEnforceValue, m_dDeforceValue;

		
	public:
		CDynaDECMustang();
		virtual ~CDynaDECMustang() {}
		//CDynaExciterBase *m_pExciter;


		PrimitiveVariableExternal Vnode;
		PrimitiveVariable EnforceOnOut, DeforceOnOut;
		PrimitiveVariable EnforceOffOut, DeforceOffOut;

		double VEnfOn, VEnfOff, VDefOn, VDefOff, EnfRatio, DefRatio, EnfTexc, DefTexc, TdelOn, TdelOff;

		enum VARS
		{
			V_ENFONRELAY,
			V_ENFOFFRELAY,
			V_DEFONRELAY,
			V_DEFOFFRELAY,
			V_ENFTRIG,
			V_DEFTRIG,
			V_DEC,
			V_LAST
		};

		double Udec;
		double Texc, Umin, Umax;

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual bool BuildEquations(CDynaModel* pDynaModel);
		virtual bool BuildRightHand(CDynaModel* pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);

		virtual void UpdateSerializer(SerializerPtr& Serializer) override;

		static const CDeviceContainerProperties DeviceProperties();
	};
}

