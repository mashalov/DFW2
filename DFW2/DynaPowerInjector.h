#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaPrimitive.h"

namespace DFW2
{
	class CDynaPowerInjector : public CDevice
	{
	protected:
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		PrimitiveVariableExternal V, DeltaV, Sv;
	public:
		enum VARS
		{
			V_P,
			V_Q,
			V_LAST
		};

		enum CONSTVARS
		{
			C_NODEID,
			C_LAST
		};


		double P;
		double Q;
		double Kgen;
		double LFQmin;
		double LFQmax;

		double NodeId;

		CDynaPowerInjector(); 
		virtual bool CalculatePower() { return true; }
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		static const CDeviceContainerProperties DeviceProperties();
		static const _TCHAR* m_cszNodeId;
	};
}
