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
		}
	}
	return p;
}

VariableIndexRefVec& CSVC::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaPowerInjector::GetVariables(JoinVariables({ ControlIn, ControlOut, Bout }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CSVC::PreInit(CDynaModel* pDynaModel)
{
	const double Unom2{ Unom * Unom };
	xsl_ = Droop_ * Unom2 / Qnom_;
	Bmin_ = LFQmin / Unom2;
	Bmax_ = LFQmax / Unom2;
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
		V0_ = 1.0 / xsl_ / V;
		ControlOut = Bout;
		CoilLag_.SetMinMaxTK(pDynaModel, Bmin_, Bmax_, 0.9, 1.0);
		CoilLag_.Init(pDynaModel);
		ControlLag_.SetMinMaxTK(pDynaModel, Bmin_, Bmax_, 0.04, 1.0);
		ControlLag_.Init(pDynaModel);
		ControlIn = (V - Vref_) * V0_;
	}
	return Status;
}

void CSVC::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunction(ControlIn, ControlIn - (V - Vref_) * V0_);
	pDynaModel->SetFunction(Ire, Ire + Vim * Bout);
	pDynaModel->SetFunction(Iim, Iim - Vre * Bout);
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
	CDynaPowerInjector::BuildEquations(pDynaModel);
	CDevice::BuildEquations(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CSVC::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	ControlIn = (V - Vref_) * V0_;
	CDevice::ProcessDiscontinuity(pDynaModel);
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
	props.VarMap_.insert(std::make_pair("B", CVarIndex(CSVC::V_BOUT, VARUNIT_PU)));
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