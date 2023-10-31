#include "stdafx.h"
#include "DynaNode.h"
#include "SVC.h"
#include "DynaModel.h"
using namespace DFW2;

double* CSVC::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaPowerInjector::GetVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(ControlIn.Value, V_CONTIN)
			MAP_VARIABLE(ControlOut.Value, V_CONTOUT)
			MAP_VARIABLE(Bout.Value, V_BOUT)
			MAP_VARIABLE(I.Value, V_IABS)
			MAP_VARIABLE(LowVoltageOut.Value, V_LOWVOLTAGE)
			MAP_VARIABLE(HighVoltageOut.Value, V_HIGHVOLTAGE)
			MAP_VARIABLE(LowCurrentOut.Value, V_LOWCURRENT)
			MAP_VARIABLE(CanDeforceOut.Value, V_CANDEFORCE)
			MAP_VARIABLE(CanEnforceOut.Value, V_CANENFORCE)
			MAP_VARIABLE(HighCurrentOut.Value, V_CURRENTAVAIL)
			MAP_VARIABLE(EnforceOut.Value, V_ENFTRIG)
			MAP_VARIABLE(EnforceS.Value, V_ENFTRIGS)
			MAP_VARIABLE(EnforceR.Value, V_ENFTRIGR)
			MAP_VARIABLE(DeforceOut.Value, V_DEFTRIG)
			MAP_VARIABLE(DeforceS.Value, V_DEFTRIGS)
			MAP_VARIABLE(DeforceR.Value, V_DEFTRIGR)
			MAP_VARIABLE(IfOut.Value, V_IF)
		}
	}
	return p;
}

VariableIndexRefVec& CSVC::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaPowerInjector::GetVariables(JoinVariables({ ControlIn, ControlOut, Bout, I,
															LowVoltageOut, HighVoltageOut, LowCurrentOut, 
															CanDeforceOut, CanEnforceOut, HighCurrentOut,
															EnforceOut, EnforceS, EnforceR,
															DeforceOut, DeforceS, DeforceR, IfOut }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CSVC::PreInit(CDynaModel* pDynaModel)
{
	const double Unom2{ Unom * Unom };
	xsl_ = Droop_ * Unom2 / Qnom_;
	Bmin_ = LFQmin / Unom2;
	Bmax_ = LFQmax / Unom2;
	Bcr_ = 1e6 * (Bmax_ - Bmin_);
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CSVC::InitModel(CDynaModel* pDynaModel)
{
	auto Status{ CDynaPowerInjector::InitModel(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(Status))
	{
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
		Bout = 1e6 * Output_.Blimited;
		//FromComplex(Ire, Iim, cplx(Vre, Vim) * cplx(0.0, Bout));
		Ire = -Vim * 1e-6 * Bout;
		Iim =  Vre * 1e-6 * Bout;
		I = V * Bout;
		V0_ = 1.0 / xsl_ / V;
		ControlOut = Bout;
		CoilLag_.SetMinMaxTK(pDynaModel, 1e6 * Bmin_, 1e6 * Bmax_, Tcoil_, 1.0);
		ControlLag_.SetMinMaxTK(pDynaModel, 1e6 * Bmin_, 1e6 * Bmax_, Tcontrol_, 1e6);
		if (!CoilLag_.Init(pDynaModel) || !ControlLag_.Init(pDynaModel))
			Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
		ControlIn = (V - Vref_) * V0_;

		const double Imin{ Bmin_ * Unom };
		const double Imax{ Bmax_ * Unom };
		const double Icr{ Imax - Imin };

		LowVoltage.SetRefs(pDynaModel , Vref_ * (1.0 - xsl_ * Bmin_), false);
		HighVoltage.SetRefs(pDynaModel, Vref_ * (1.0 + xsl_ * Bmax_), true);
		CanDeforce.SetRefs(pDynaModel, Imin + Icr * Kidef_, true);
		CanEnforce.SetRefs(pDynaModel, Imin + Icr * Kienf_, false);
		LowCurrent.SetRefs(pDynaModel, Imin, true);
		HighCurrent.SetRefs(pDynaModel, Imax, false);

		LowVoltage.Init(pDynaModel);
		HighVoltage.Init(pDynaModel);
		LowCurrent.Init(pDynaModel);
		CanDeforce.Init(pDynaModel);
		CanEnforce.Init(pDynaModel);
		HighCurrent.Init(pDynaModel);
		EnforceTrigger.Init(pDynaModel);
		DeforceTrigger.Init(pDynaModel);

		Status = ProcessDiscontinuity(pDynaModel);
	}
	return Status;
}

void CSVC::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunction(ControlIn, ControlIn - (V - Vref_) * V0_);
	pDynaModel->SetFunction(Ire, Ire + Vim * 1e-6 * Bout);
	pDynaModel->SetFunction(Iim, Iim - Vre * 1e-6 * Bout);
	pDynaModel->SetFunction(I, I - V * 1e-6 * Bout);
	pDynaModel->SetFunction(EnforceS, 0.0);
	pDynaModel->SetFunction(EnforceR, 0.0);
	pDynaModel->SetFunction(DeforceS, 0.0);
	pDynaModel->SetFunction(DeforceR, 0.0);

	double dIf{ IfOut - ControlOut };

	if (EnforceOut > 0)
		dIf = IfOut - Kenf_ * Bcr_;
	else if (DeforceOut > 0)
		dIf = IfOut + Kdef_ * Bcr_;

	pDynaModel->SetFunction(IfOut, dIf);

	SetFunctionsDiff(pDynaModel);
	CDynaPowerInjector::BuildRightHand(pDynaModel);
	CDevice::BuildRightHand(pDynaModel);
}

void CSVC::BuildEquations(CDynaModel* pDynaModel)
{
	pDynaModel->SetElement(ControlIn, ControlIn, 1.0);
	pDynaModel->SetElement(ControlIn, V, -V0_);
	// Ire + Vim * Bout = 0
	// Iim - Vre * Bout = 0
	pDynaModel->SetElement(Ire, Ire, 1.0);
	pDynaModel->SetElement(Ire, Vim, 1e-6 * Bout);
	pDynaModel->SetElement(Ire, Bout, 1e-6 * Vim);
	pDynaModel->SetElement(Iim, Iim, 1.0);
	pDynaModel->SetElement(Iim, Vre, -1e-6 * Bout);
	pDynaModel->SetElement(Iim, Bout, -1e-6 * Vre);
	// I - V * Bout * xsl_ = 0

	pDynaModel->SetElement(I, I, 1.0);
	pDynaModel->SetElement(I, V, -1e-6 * Bout);
	pDynaModel->SetElement(I, Bout, -1e-6 * V);
	pDynaModel->SetElement(EnforceS, EnforceS, 1.0);
	pDynaModel->SetElement(EnforceR, EnforceR, 1.0);
	pDynaModel->SetElement(DeforceS, DeforceS, 1.0);
	pDynaModel->SetElement(DeforceR, DeforceR, 1.0);
	pDynaModel->SetElement(IfOut, IfOut, 1.0);

	double IfByControlOut{ 0.0 };

	if (EnforceOut <= 0 && DeforceOut <= 00)
		IfByControlOut = -1.0;
	pDynaModel->SetElement(IfOut, ControlOut, IfByControlOut);

	CDynaPowerInjector::BuildEquations(pDynaModel);
	CDevice::BuildEquations(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CSVC::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	ControlIn = (V - Vref_) * V0_;
	I = V * 1e-6 * Bout;
	EnforceR = HighVoltageOut <= 0.0 && HighCurrentOut > 0.0;
	EnforceS = CanEnforceOut > 0.0 && HighVoltageOut > 0.0;
	DeforceR = LowVoltageOut <= 0.0 && LowCurrentOut > 0.0;
	DeforceS = CanDeforceOut > 0.0 && LowVoltageOut > 0.0;
	CDevice::ProcessDiscontinuity(pDynaModel);
	if (EnforceOut > 0)
		IfOut = Kenf_ * Bcr_;
	else if (DeforceOut > 0)
		IfOut = -Kdef_ * Bcr_;
	else
		IfOut = ControlOut;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

const CSVC::Output& CSVC::B(const CDynaNodeBase& pNode)
{
	Output_.Bunlimited =(pNode.V - Vref_) / xsl_ / pNode.V;

	if (Output_.Bunlimited > Bmax_)
	{
		Output_.Blimited = Bmax_;
		Output_.State = CDynaPrimitiveLimited::eLIMITEDSTATES::Max;
	}
	else if (Output_.Bunlimited < Bmin_)
	{
		Output_.Blimited = Bmin_;
		Output_.State = CDynaPrimitiveLimited::eLIMITEDSTATES::Min;
	}
	else
	{
		Output_.Blimited = Output_.Bunlimited;
		Output_.State = CDynaPrimitiveLimited::eLIMITEDSTATES::Mid;
	}

	const double V2{ pNode.V * pNode.V };

	Q = -(Output_.Qlimited = Output_.Blimited * V2);
	Output_.Qunlimited = Output_.Bunlimited * V2;

	return Output_;
} 


void CSVC::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaPowerInjector::DeviceProperties(props);
	props.SetType(DEVTYPE_SVC);
	props.SetClassName(CDeviceContainerProperties::m_cszNameSVC, CDeviceContainerProperties::m_cszSysNameSVC);
	props.EquationsCount = CSVC::VARS::V_LAST;
	props.DeviceFactory = std::make_unique<CDeviceFactory<CSVC>>();
	props.bStoreStates = true;

	props.VarMap_.insert(std::make_pair("ConIn", CVarIndex(CSVC::V_CONTIN, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("ConOut", CVarIndex(CSVC::V_CONTOUT, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("B", CVarIndex(CSVC::V_BOUT, VARUNIT_SIEMENS)));
	props.VarMap_.insert(std::make_pair("LoV", CVarIndex(CSVC::V_LOWVOLTAGE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("HiV", CVarIndex(CSVC::V_HIGHVOLTAGE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("|I|", CVarIndex(CSVC::V_IABS, VARUNIT_KAMPERES)));
	props.VarMap_.insert(std::make_pair("LowI", CVarIndex(CSVC::V_LOWCURRENT, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("CanDef", CVarIndex(CSVC::V_CANDEFORCE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("CanEnf", CVarIndex(CSVC::V_CANENFORCE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("HighI", CVarIndex(CSVC::V_CURRENTAVAIL, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("Enforce", CVarIndex(CSVC::V_ENFTRIG, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("Deforce", CVarIndex(CSVC::V_DEFTRIG, VARUNIT_DIGITAL)));
}

void CSVC::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaPowerInjector::UpdateSerializer(Serializer);
	Serializer->AddProperty(cszDroop_, Droop_, eVARUNITS::VARUNIT_PERCENT, 0.01);
	Serializer->AddProperty(CDynaPowerInjector::m_cszUnom, Unom, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty(CDynaPowerInjector::m_cszQnom, Qnom_, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty(CDynaPowerInjector::cszVref_, Vref_, eVARUNITS::VARUNIT_KVOLTS);
}

void CSVC::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaPowerInjector::UpdateValidator(Validator);
	Validator->AddRule(CDynaPowerInjector::m_cszUnom, &CDynaPowerInjector::ValidatorUnom);
	Validator->AddRule({ cszDroop_, CDynaPowerInjector::m_cszQnom }, &CSerializerValidatorRules::BiggerThanZero);
}