#pragma once
#include "DynaExciterBase.h"
#include "Relay.h"
#include "RSTrigger.h"

namespace DFW2
{
	class CDynaDECMustang : public CDevice
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;

		CRelayDelay EnforceOn;
		CRelayDelay EnforceOff;
		CRelayDelay DeforceOn;
		CRelayDelay DeforceOff;
		CRSTrigger  EnfTrigger;
		CRSTrigger  DefTrigger;

		VariableIndex EnforceOnValue, DeforceOnValue;
		VariableIndex EnforceOffValue, DeforceOffValue;
		VariableIndex EnforceTrigValue, DeforceTrigValue;
		VariableIndex Udec;
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

		double Texc, Umin, Umax;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;

		void UpdateSerializer(SerializerPtr& Serializer) override;

		static const CDeviceContainerProperties DeviceProperties();
	};
}

