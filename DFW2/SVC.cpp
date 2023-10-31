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
			MAP_VARIABLE(LoV.Value, V_LOWVOLTAGE)
			MAP_VARIABLE(HiV.Value, V_HIGHVOLTAGE)
			MAP_VARIABLE(NZ.Value, V_LOWCURRENT)
			MAP_VARIABLE(CD.Value, V_CANDEFORCE)
			MAP_VARIABLE(CE.Value, V_CANENFORCE)
			MAP_VARIABLE(CA.Value, V_CURRENTAVAIL)
			MAP_VARIABLE(EnfOut.Value, V_ENFTRIG)
			MAP_VARIABLE(EnfS.Value, V_ENFTRIGS)
			MAP_VARIABLE(EnfR.Value, V_ENFTRIGR)
			MAP_VARIABLE(DefOut.Value, V_DEFTRIG)
			MAP_VARIABLE(DefS.Value, V_DEFTRIGS)
			MAP_VARIABLE(DefR.Value, V_DEFTRIGR)
			MAP_VARIABLE(If.Value, V_IF)
		}
	}
	return p;
}

VariableIndexRefVec& CSVC::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaPowerInjector::GetVariables(JoinVariables({ ControlIn, ControlOut, Bout, I,
															LoV, HiV, NZ, CD, CE, CA,
															EnfOut, EnfS, EnfR,
															DefOut, DefS, DefR, If }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CSVC::PreInit(CDynaModel* pDynaModel)
{
	const double Unom2{ Unom * Unom };
	xsl_ = Droop_ * Unom2 / Qnom_;
	Bmin_ = LFQmin / Unom2;
	Bmax_ = LFQmax / Unom2;
	Bcr_ = Bmax_ - Bmin_;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CSVC::InitModel(CDynaModel* pDynaModel)
{
	auto Status{ CDynaPowerInjector::InitModel(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(Status))
	{
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
		Bout = Output_.Blimited;
		//FromComplex(Ire, Iim, cplx(Vre, Vim) * cplx(0.0, Bout));
		Ire = -Vim * Bout;
		Iim =  Vre * Bout;
		I = V * Bout;
		V0_ = 1.0 / xsl_ / V;
		ControlOut = Bout;
		CoilLag_.SetMinMaxTK(pDynaModel, Bmin_, Bmax_, Tcoil_, 1.0);
		ControlLag_.SetMinMaxTK(pDynaModel, Bmin_, Bmax_, Tcontrol_, 1.0);
		if (!CoilLag_.Init(pDynaModel) || !ControlLag_.Init(pDynaModel))
			Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
		ControlIn = (V - Vref_) * V0_;

		const double Imin{ Bmin_ * Unom };
		const double Imax{ Bmax_ * Unom };
		const double Icr{ Imax - Imin };

		LowVoltage.SetRefs(pDynaModel , Vref_ * (1.0 - xsl_ * Bmin_), false);
		HighVoltage.SetRefs(pDynaModel, Vref_ * (1.0 + xsl_ * Bmax_), true);
		CanDeforce.SetRefs(pDynaModel, Imin + Icr * Kidef_, true);
		CanEnforce.SetRefs(pDynaModel, Imax - Icr * (1.0 - Kienf_), false);
		LowCurrent.SetRefs(pDynaModel, Imin, true);
		CurrentAvailable.SetRefs(pDynaModel, Imax, false);

		LowVoltage.Init(pDynaModel);
		HighVoltage.Init(pDynaModel);
		LowCurrent.Init(pDynaModel);
		CanDeforce.Init(pDynaModel);
		CanEnforce.Init(pDynaModel);
		CurrentAvailable.Init(pDynaModel);
		EnforceTrigger.Init(pDynaModel);
		DeforceTrigger.Init(pDynaModel);

		Status = ProcessDiscontinuity(pDynaModel);
	}
	return Status;
}

void CSVC::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunction(ControlIn, ControlIn - (V - Vref_) * V0_);
	pDynaModel->SetFunction(Ire, Ire + Vim * Bout);
	pDynaModel->SetFunction(Iim, Iim - Vre * Bout);
	pDynaModel->SetFunction(I, I - V * Bout);
	pDynaModel->SetFunction(EnfS, 0.0);
	pDynaModel->SetFunction(EnfR, 0.0);
	pDynaModel->SetFunction(DefS, 0.0);
	pDynaModel->SetFunction(DefR, 0.0);

	if (EnfOut > 0)
		pDynaModel->SetFunction(If, If - Kenf_ * Bcr_);
	else if (DefOut > 0)
		pDynaModel->SetFunction(If, If + Kdef_ * Bcr_);
	else
		pDynaModel->SetFunction(If, If - ControlOut);

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
	pDynaModel->SetElement(Ire, Vim, Bout);
	pDynaModel->SetElement(Ire, Bout, Vim);
	pDynaModel->SetElement(Iim, Iim, 1.0);
	pDynaModel->SetElement(Iim, Vre, -Bout);
	pDynaModel->SetElement(Iim, Bout, -Vre);
	// I - V * Bout * xsl_ = 0

	pDynaModel->SetElement(I, I, 1.0);
	pDynaModel->SetElement(I, V, -Bout);
	pDynaModel->SetElement(I, Bout, -V);
	pDynaModel->SetElement(EnfS, EnfS, 1.0);
	pDynaModel->SetElement(EnfR, EnfR, 1.0);
	pDynaModel->SetElement(DefS, DefS, 1.0);
	pDynaModel->SetElement(DefR, DefR, 1.0);
	pDynaModel->SetElement(If, If, 1.0);
	if (EnfOut <= 0 && DefOut <= 00)
		pDynaModel->SetElement(If, ControlOut, -1.0);

	CDynaPowerInjector::BuildEquations(pDynaModel);
	CDevice::BuildEquations(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CSVC::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	ControlIn = (V - Vref_) * V0_;
	I = V * Bout;
	EnfR = HiV <= 0.0 && CA > 0.0;
	EnfS = CE > 0.0 && HiV > 0.0;
	DefR = LoV <= 0.0 && NZ > 0.0;
	DefS = CD > 0.0 && LoV > 0.0;
	CDevice::ProcessDiscontinuity(pDynaModel);
	if (EnfOut > 0)
		If = Kenf_ * Bcr_;
	else if (DefOut > 0)
		If = -Kdef_ * Bcr_;
	else
		If = ControlOut;
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

	props.VarMap_.insert(std::make_pair("ControlIn", CVarIndex(CSVC::V_CONTIN, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("ControlOut", CVarIndex(CSVC::V_CONTOUT, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("B", CVarIndex(CSVC::V_BOUT, VARUNIT_SIEMENS)));
	props.VarMap_.insert(std::make_pair("LoV", CVarIndex(CSVC::V_LOWVOLTAGE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("HiV", CVarIndex(CSVC::V_HIGHVOLTAGE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("|I|", CVarIndex(CSVC::V_IABS, VARUNIT_KAMPERES)));
	props.VarMap_.insert(std::make_pair("LowI", CVarIndex(CSVC::V_LOWCURRENT, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("CD", CVarIndex(CSVC::V_CANDEFORCE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("CE", CVarIndex(CSVC::V_CANENFORCE, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("CA", CVarIndex(CSVC::V_CURRENTAVAIL, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("Enf", CVarIndex(CSVC::V_ENFTRIG, VARUNIT_DIGITAL)));
	props.VarMap_.insert(std::make_pair("Def", CVarIndex(CSVC::V_DEFTRIG, VARUNIT_DIGITAL)));
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
	Validator->AddRule(CDynaPowerInjector::m_cszQnom, &CSerializerValidatorRules::BiggerThanZero);
}