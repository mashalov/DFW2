#pragma once
#include "DynaPowerInjector.h"
#include "LimitedLag.h"
#include "Relay.h"
#include "RSTrigger.h"

namespace DFW2
{
	class CDynaSVCBase : public CDynaPowerInjector
	{
	protected:
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		CLimitedLag ControlLag_;
		void CalculateIreIm();
	public:
		enum VARS
		{
			V_CONTIN = CDynaPowerInjector::V_LAST,
			V_CONTOUT,
			V_BOUT,
			V_LAST
		};
		VariableIndex ControlIn, ControlOut, Bout;

		CDynaSVCBase() : CDynaPowerInjector(),
			ControlLag_(*this, { ControlOut }, { ControlIn })
		{}

		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;

		double Droop_ = 0.03;
		double Qnom_ = 100.0;
		double Vref_ = 220.0;
		double xsl_ = 0.01;
		double Bmin_ = -1;
		double Bmax_ = -1;
		double V0_ = 220.0;
		double Tcontrol_ = 0.04;

		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		bool CalculatePower() override;
		cplx Igen(ptrdiff_t nIteration) override;

		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;

		static constexpr const char* cszDroop_ = "Droop";
		static constexpr const char* cszTcontrol_ = "Tc";

	};


	class CDynaSVC : public CDynaSVCBase
	{
	protected:
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
	public:
		CLimitedLag CoilLag_;
		CDynaSVC() : CDynaSVCBase(),
			CoilLag_(*this, { Bout }, { ControlOut })
		{
			P = 0.0;
			Kgen = 1.0;
		}

		double Tcoil_ = 0.9;
	
		void BuildRightHand(CDynaModel* pDynaModel);
		void BuildEquations(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;

		static constexpr const char* cszTcoil_ = "Tcoil";
	};

	class CDynaSVCDEC : public CDynaSVCBase
	{
	protected:
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		CLimitedLag CoilLag_;
		CRelay LowVoltage;
		CRelay HighVoltage;
		CRelay LowCurrent;
		CRelay CanDeforce;
		CRelay CanEnforce;
		CRelay HighCurrent;
		CRSTrigger EnforceTrigger;
		CRSTrigger DeforceTrigger;

		double Vmin_ = 0.0;
		double Vmax_ = 0.0;
		double Ilow_ = 0.0;
		double Ienfb_ = 0.0;
		double Idefb_ = 0.0;
		double Imax_ = 0.0;

	public:

		enum VARS
		{
			V_IABS = CDynaSVCBase::V_LAST,
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

		VariableIndex I;
		VariableIndex LowVoltageOut, HighVoltageOut, LowCurrentOut;
		VariableIndex CanDeforceOut, CanEnforceOut, HighCurrentOut;
		VariableIndex EnforceOut, EnforceS, EnforceR;
		VariableIndex DeforceOut, DeforceS, DeforceR, IfOut;
		
		CDynaSVCDEC() : CDynaSVCBase(),
			CoilLag_(*this, { Bout }, { IfOut }),
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

		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;

		double Tcoil_ = 0.9;
		double Kenf_ = 2.0;
		double Kdef_ = 2.0;
		double Kienf_ = 0.7;
		double Kidef_ = 0.3;

		double Bcr_;

		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;

		void BuildRightHand(CDynaModel* pDynaModel);
		void BuildEquations(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static constexpr const char* cszKenf_ = "Kenf";
		static constexpr const char* cszKdef_ = "Kdef";
		static constexpr const char* cszKienf_ = "Kienf";
		static constexpr const char* cszKidef_ = "Kidef";
	};
}

