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

		CRelay LowVoltage;
		CRelay HighVoltage;
		CRelay LowCurrent;
		CRelay CanDeforce;
		CRelay CanEnforce;
		CRelay HighCurrent;
		CRSTrigger EnforceTrigger;
		CRSTrigger DeforceTrigger;

		struct Output
		{
			double Bunlimited;
			double Blimited;
			double Qunlimited;
			double Qlimited;
			CDynaPrimitiveLimited::eLIMITEDSTATES State;
		} 
			Output_;

		double Vmin_ = 0.0;
		double Vmax_ = 0.0;
		double Ilow_ = 0.0;
		double Ienfb_ = 0.0;
		double Idefb_ = 0.0;
		double Imax_ = 0.0;

	public:

		enum VARS
		{
			V_CONTIN = CDynaPowerInjector::V_LAST,
			V_CONTOUT,
			V_BOUT,
			V_IABS,
			V_LOWVOLTAGE,
			V_HIGHVOLTAGE,
			V_LOWCURRENT,
			V_CANDEFORCE,
			V_CANENFORCE,
			V_CURRENTAVAIL,
			V_ENFTRIG,
			V_ENFTRIGS,
			V_ENFTRIGR,
			V_DEFTRIG,
			V_DEFTRIGS,
			V_DEFTRIGR,
			V_IF,
			V_LAST
		};

		VariableIndex ControlIn, ControlOut, Bout, I;
		VariableIndex LowVoltageOut, HighVoltageOut, LowCurrentOut;
		VariableIndex CanDeforceOut, CanEnforceOut, HighCurrentOut;
		VariableIndex EnforceOut, EnforceS, EnforceR;
		VariableIndex DeforceOut, DeforceS, DeforceR, IfOut;
		
		CSVC() : CDynaPowerInjector(),
			CoilLag_(*this, { Bout }, { IfOut }),
			ControlLag_(*this, { ControlOut }, { ControlIn }),
			LowVoltage(*this, { LowVoltageOut }, { V }),
			HighVoltage(*this, { HighVoltageOut }, { V }),
			LowCurrent(*this, { LowCurrentOut }, { I }),
			CanDeforce(*this, { CanDeforceOut }, { I }),
			CanEnforce(*this, { CanEnforceOut }, { I }),
			HighCurrent(*this, { HighCurrentOut }, { I }),
			EnforceTrigger(*this, { EnforceOut }, { EnforceR, EnforceS }),
			DeforceTrigger(*this, { DeforceOut }, { DeforceR, DeforceS })
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
		double Tcoil_ = 0.9;
		double Tcontrol_ = 0.04;
		double Kenf_ = 2.0;
		double Kdef_ = 2.0;
		double Kienf_ = 0.7;
		double Kidef_ = 0.3;

		double Bcr_;

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
		static constexpr const char* cszTcoil_ = "Tcoil";
		static constexpr const char* cszTcontrol_ = "Tc";
		static constexpr const char* cszKenf_ = "Kenf";
		static constexpr const char* cszKdef_ = "Kdef";
		static constexpr const char* cszKienf_ = "Kienf";
		static constexpr const char* cszKidef_ = "Kidef";
	};
}

