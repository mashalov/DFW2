#include "stdafx.h"
#include "DynaNode.h"
#include "SVC.h"
#include "DynaModel.h"
using namespace DFW2;

double* CDynaSVCBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaPowerInjector::GetVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(ControlIn.Value, V_CONTIN)
			MAP_VARIABLE(ControlOut.Value, V_CONTOUT)
			MAP_VARIABLE(Bout.Value, V_BOUT)
		}
	}
	return p;
}

bool CDynaSVCBase::CalculatePower()
{
	FromComplex(P, Q, V * V * cplx(0.0, Bout * 1e-6));\
	//  задаем проводимость для LULF
	Ygen_ = cplx(0.0, -Bout * 1e-6);
	return true;
}

cplx CDynaSVCBase::Igen(ptrdiff_t nIteration)
{
	// для расчета LULF Igen = 0 для УШР на проводимости
	// он учитывается проводимостью в Y
	return { 0.0 };
}

double* CDynaSVCDEC::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ CDynaSVCBase::GetVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
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


VariableIndexRefVec& CDynaSVCBase::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaPowerInjector::GetVariables(JoinVariables({ ControlIn, ControlOut, Bout }, ChildVec));
}

VariableIndexRefVec& CDynaSVCDEC::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaSVCBase::GetVariables(JoinVariables({ I, LowVoltageOut, HighVoltageOut, LowCurrentOut, 
														 CanDeforceOut, CanEnforceOut, HighCurrentOut,
														 EnforceOut, EnforceS, EnforceR,
														 DeforceOut, DeforceS, DeforceR, IfOut }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CDynaSVCBase::PreInit(CDynaModel* pDynaModel)
{
	const double Unom2{ Unom * Unom };
	xsl_ = Droop_ * Unom2 / Qnom_;
	Bmin_ = LFQmin / Unom2;
	Bmax_ = LFQmax / Unom2;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaSVCDEC::PreInit(CDynaModel* pDynaModel)
{
	auto Status{ CDynaSVCBase::PreInit(pDynaModel) };
	Bcr_ = 1e6 * (Bmax_ - Bmin_);
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaSVCBase::InitModel(CDynaModel* pDynaModel)
{
	V0_ = 1.0 / xsl_ / V * 1e6;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaSVC::InitModel(CDynaModel* pDynaModel)
{
	auto Status{ CDynaSVCBase::InitModel(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(Status))
	{
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
		Bout = Q / V / V * 1e6;
		CalculateIreIm();
		ControlOut = Bout;
		CoilLag_.SetMinMaxTK(pDynaModel, 1e6 * Bmin_, 1e6 * Bmax_, Tcoil_);
		ControlLag_.SetMinMaxTK(pDynaModel, 1e6 * Bmin_, 1e6 * Bmax_, Tcontrol_);
		if (!CoilLag_.Init(pDynaModel) || !ControlLag_.Init(pDynaModel))
			Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
		ControlIn = (V - Vref_) * V0_;
		Status = ProcessDiscontinuity(pDynaModel);
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaSVCDEC::InitModel(CDynaModel* pDynaModel)
{
	auto Status{ CDynaSVCBase::InitModel(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(Status))
	{
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
		Bout = Q / V / V * 1e6;
		//FromComplex(Ire, Iim, cplx(Vre, Vim) * cplx(0.0, Bout));
		CalculateIreIm();
		I = V * Bout;
		ControlOut = Bout;
		CoilLag_.SetMinMaxTK(pDynaModel, 1e6 * Bmin_, 1e6 * Bmax_, Tcoil_);
		ControlLag_.SetMinMaxTK(pDynaModel, 1e6 * Bmin_, 1e6 * Bmax_, Tcontrol_);
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

void CDynaSVC::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunction(ControlIn, ControlIn - (V - Vref_) * V0_);
	pDynaModel->SetFunction(Ire, Ire + Vim * 1e-6 * Bout);
	pDynaModel->SetFunction(Iim, Iim - Vre * 1e-6 * Bout);
	SetFunctionsDiff(pDynaModel);
	CDynaSVCBase::BuildRightHand(pDynaModel);
	CDevice::BuildRightHand(pDynaModel);
}

void CDynaSVCDEC::BuildRightHand(CDynaModel* pDynaModel)
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
	CDynaSVCBase::BuildRightHand(pDynaModel);
	CDevice::BuildRightHand(pDynaModel);
}

void CDynaSVC::BuildEquations(CDynaModel* pDynaModel)
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
	CDynaSVCBase::BuildEquations(pDynaModel);
	CDevice::BuildEquations(pDynaModel);
}

void CDynaSVCDEC::BuildEquations(CDynaModel* pDynaModel)
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

	CDynaSVCBase::BuildEquations(pDynaModel);
	CDevice::BuildEquations(pDynaModel);
}

void CDynaSVCBase::CalculateIreIm()
{
	if (IsStateOn())
	{
		Ire = -Vim * 1e-6 * Bout;
		Iim = Vre * 1e-6 * Bout;
	}
	else
		Ire = Iim = 0.0;
}

eDEVICEFUNCTIONSTATUS CDynaSVCBase::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	CalculateIreIm();
	auto Status(CDynaPowerInjector::ProcessDiscontinuity(pDynaModel));
	ControlIn = (V - Vref_) * V0_;
	CDevice::ProcessDiscontinuity(pDynaModel);
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaSVC::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	auto Status(CDynaSVCBase::ProcessDiscontinuity(pDynaModel));
	return Status;
}


eDEVICEFUNCTIONSTATUS CDynaSVCDEC::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	auto Status(CDynaSVCBase::ProcessDiscontinuity(pDynaModel));
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
	return Status;
}

void CDynaSVCBase::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaPowerInjector::DeviceProperties(props);
	props.SetType(DEVTYPE_SVC);
	props.bStoreStates = true;

	props.VarMap_.insert(std::make_pair("ConIn", CVarIndex(CDynaSVCBase::V_CONTIN, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("ConOut", CVarIndex(CDynaSVCBase::V_CONTOUT, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("B", CVarIndex(CDynaSVCBase::V_BOUT, VARUNIT_SIEMENS)));
}

void CDynaSVC::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaSVCBase::DeviceProperties(props);
	props.SetType(DEVTYPE_SVCPROP);
	props.SetClassName(CDeviceContainerProperties::cszNameSVC_, CDeviceContainerProperties::cszSysNameSVC_);
	props.EquationsCount = CDynaSVC::VARS::V_LAST;
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaSVC>>();
}

void CDynaSVCDEC::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaSVCBase::DeviceProperties(props);
	props.SetType(DEVTYPE_SVCPROPDEC);
	props.SetClassName(CDeviceContainerProperties::cszNameSVCDEC_, CDeviceContainerProperties::cszSysNameSVCDEC_);
	props.EquationsCount = CDynaSVCDEC::VARS::V_LAST;
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaSVCDEC>>();

	props.VarMap_.insert(std::make_pair("LoV", CVarIndex(CDynaSVCDEC::V_LOWVOLTAGE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("HiV", CVarIndex(CDynaSVCDEC::V_HIGHVOLTAGE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("|I|", CVarIndex(CDynaSVCDEC::V_IABS, VARUNIT_KAMPERES)));
	props.VarMap_.insert(std::make_pair("LoI", CVarIndex(CDynaSVCDEC::V_LOWCURRENT, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("CanDef", CVarIndex(CDynaSVCDEC::V_CANDEFORCE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("CanEnf", CVarIndex(CDynaSVCDEC::V_CANENFORCE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("HighI", CVarIndex(CDynaSVCDEC::V_CURRENTAVAIL, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("Enforce", CVarIndex(CDynaSVCDEC::V_ENFTRIG, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("Deforce", CVarIndex(CDynaSVCDEC::V_DEFTRIG, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("IF", CVarIndex(CDynaSVCDEC::V_IF, VARUNIT_PU)));
}

void CDynaSVCBase::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaPowerInjector::UpdateSerializer(Serializer);
	Serializer->AddProperty(cszDroop_, Droop_, eVARUNITS::VARUNIT_PERCENT, 0.01);
	Serializer->AddProperty(CDynaPowerInjector::cszUnom_, Unom, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty(CDynaPowerInjector::cszQnom_, Qnom_, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty(CDynaPowerInjector::cszVref_, Vref_, eVARUNITS::VARUNIT_KVOLTS);
}

void CDynaSVC::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaSVCBase::UpdateSerializer(Serializer);
}


void CDynaSVCDEC::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaSVCBase::UpdateSerializer(Serializer);
}

void CDynaSVCBase::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaPowerInjector::UpdateValidator(Validator);
	Validator->AddRule(CDynaPowerInjector::cszUnom_, &CDynaPowerInjector::ValidatorUnom);
	Validator->AddRule({ cszDroop_, CDynaPowerInjector::cszQnom_ }, &CSerializerValidatorRules::BiggerThanZero);
}

void CDynaSVC::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaSVCBase::UpdateValidator(Validator);
}


void CDynaSVCDEC::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaSVCBase::UpdateValidator(Validator);
}