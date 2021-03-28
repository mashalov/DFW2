#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorInfBus.h"

namespace DFW2
{
	class CDynaGeneratorMotion : public CDynaGeneratorInfBusBase
	{
	protected:
		 eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
	public:
		enum VARS
		{
			V_S = CDynaGeneratorInfBusBase::V_LAST,
			V_DELTA,
			V_LAST
		};

		enum CONSTVARS
		{
			C_UNOM = CDynaGeneratorInfBusBase::CONSTVARS::C_LAST,
			C_LAST
		};


		VariableIndex s;

		double	Unom, Kdemp,xq, Mj, Pt, Pnom, cosPhinom;

		CDynaGeneratorMotion();
		virtual ~CDynaGeneratorMotion() {}

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		static double ZeroGuardSlip(double Omega) { return (Omega > 0) ? Omega : DFW2_EPSILON; }
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		void UpdateSerializer(DeviceSerializerPtr& Serializer) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static const char* m_cszUnom;
	};
}

