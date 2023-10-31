#pragma once
#include "DynaPowerInjector.h"
#include "LimitedLag.h"
#include "Relay.h"
#include "RSTrigger.h"

namespace DFW2
{
	class CSVC : public CDynaPowerInjector
	{
	protected:
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		CLimitedLag CoilLag_;
		CLimitedLag ControlLag_;
		struct Output
		{
			double Bunlimited;
			double Blimited;
			double Qunlimited;
			double Qlimited;
			CDynaPrimitiveLimited::eLIMITEDSTATES State;
		} Output_;

	public:

		enum VARS
		{
			V_CONTIN = CDynaPowerInjector::V_LAST,
			V_CONTOUT,
			V_BOUT,
			V_LAST
		};

		VariableIndex ControlIn, ControlOut, Bout;
		
		CSVC() : CDynaPowerInjector(),
			CoilLag_(*this, { Bout }, { ControlOut }),
			ControlLag_(*this, { ControlOut }, { ControlIn })
		{
			P = 0.0;
			Kgen = 1.0;
		}

		virtual eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel);

		double Droop_ = 0.03;
		double Qnom_ = 100.0;
		double Vref_ = 220.0;
		double xsl_ = 0.01;
		double Bmin_ = -1;
		double Bmax_ = -1;
		double V0_ = 220.0;

		const Output& B(const CDynaNodeBase& pNode);

		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;

		void BuildRightHand(CDynaModel* pDynaModel);
		void BuildEquations(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static constexpr const char* m_cszr = "r";
		static constexpr const char* m_cszxd1 = "xd1";
		static constexpr const char* m_cszEqs = "Eqs";
		static constexpr const char* cszDroop_ = "Droop";
	};
}

