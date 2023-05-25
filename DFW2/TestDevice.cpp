#include "stdafx.h"
#include "TestDevice.h"
#include "DynaModel.h"

using namespace DFW2;

CTestDevice::CTestDevice() : CDevice(),
	OutputLag_(*this, { LagOut }, { x }),
	Der_(*this, {DerOut, DerOut1}, {x})
{
	
}

void CTestDevice::DeviceProperties(CDeviceContainerProperties& props)
{
	props.eDeviceType = DEVTYPE_TESTDEVICE;
	props.SetType(DEVTYPE_TESTDEVICE);
	props.SetClassName("Тестовое устройство", "Test device");
	props.DeviceFactory = std::make_unique<CDeviceFactory<CTestDevice>>();
	props.VarMap_.insert({ "x", CVarIndex(V_X, VARUNIT_PU) });
	props.VarMap_.insert({ "v", CVarIndex(V_V, VARUNIT_PU) });
	props.VarMap_.insert({ "xref", CVarIndex(V_XREF, VARUNIT_PU) });
	props.VarMap_.insert({ "Lag", CVarIndex(V_LAG , VARUNIT_PU) });
	props.VarMap_.insert({ "Der", CVarIndex(V_DER , VARUNIT_PU) });
	props.EquationsCount = CTestDevice::VARS::V_XREF;
	props.bFinishStep = true;
}

void CTestDevice::BuildEquations(CDynaModel* pDynaModel)
{
	pDynaModel->SetElement(v, v, 0.0);
	pDynaModel->SetElement(x, x, 0.0);
	pDynaModel->SetElement(v, x, k / m);
	pDynaModel->SetElement(x, v, -1.0);
	CDevice::BuildEquations(pDynaModel);
}

void CTestDevice::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunctionDiff(x, v);
	pDynaModel->SetFunctionDiff(v, -k / m * x);
	CDevice::BuildRightHand(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CTestDevice::Init(CDynaModel* pDynaModel)
{
	A = x = 1.0;
	omega = std::sqrt(k / m);
	phi = -pDynaModel->GetCurrentTime() * omega;
	//OutputLag_.SetMinMaxTK(pDynaModel, -A / 2.0, A / 2.0, 0.5, 1.0);
	OutputLag_.SetMinMax(pDynaModel, -A * 2.0, A * 2.0);
	LagOut = A / 2.0;
	x = 1.0;
	v = 0.0;
	DerOut = v;
	Der_.SetTK(1e-4, 1.0);
	CDevice::Init(pDynaModel);
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


VariableIndexRefVec& CTestDevice::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ x, v, LagOut, DerOut, DerOut1 }, ChildVec));
}

double* CTestDevice::GetVariablePtr(ptrdiff_t nVarIndex)
{
	switch (nVarIndex)
	{
	case V_XREF:
		return &xref.Value;
	}
	return &GetVariable(nVarIndex).Value;
}

void CTestDevice::FinishStep(const CDynaModel& DynaModel)
{
	xref = A * cos(omega * DynaModel.GetCurrentTime() + phi);
}