#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaPrimitive.h"

namespace DFW2
{
	class CDynaPowerInjector : public CDevice
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		VariableIndexExternal V, DeltaV, Vre, Vim, Sv;
	public:
		enum VARS
		{
			V_P,
			V_Q,
			V_IRE,
			V_IIM,
			V_LAST
		};

		enum CONSTVARS
		{
			C_NODEID,
			C_LAST
		};


		VariableIndex P, Q, Ire, Iim;

		double Kgen;
		double LFQmin;
		double LFQmax;

		double NodeId;

		CDynaPowerInjector(); 
		virtual bool CalculatePower() { return true; }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void UpdateSerializer(SerializerPtr& Serializer) override;
		static const CDeviceContainerProperties DeviceProperties();
		static const _TCHAR* m_cszNodeId;
	};
}
