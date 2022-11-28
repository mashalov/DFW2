#include "stdafx.h"
#include "TestDevice.h"
#include "DynaModel.h"

using namespace DFW2;


void CTestDevice::DeviceProperties(CDeviceContainerProperties& props)
{
	props.eDeviceType = DEVTYPE_TESTDEVICE;
	props.SetType(DEVTYPE_TESTDEVICE);
	props.SetClassName("Тестовое устройство", "Test device");
	props.DeviceFactory = std::make_unique<CDeviceFactory<CTestDevice>>();
	props.VarMap_.insert({ "x", CVarIndex(V_X, VARUNIT_PU) });
	props.VarMap_.insert({ "v", CVarIndex(V_V, VARUNIT_PU) });
	props.VarMap_.insert({ "xref", CVarIndex(V_XREF, VARUNIT_PU) });
	props.EquationsCount = CTestDevice::VARS::V_XREF;
	props.bFinishStep = true;
}

void CTestDevice::BuildEquations(CDynaModel* pDynaModel)
{
	pDynaModel->SetElement(v, v, 0.0);
	pDynaModel->SetElement(x, x, 0.0);
	pDynaModel->SetElement(v, x, k / m);
	pDynaModel->SetElement(x, v, -1.0);
}

void CTestDevice::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunctionDiff(x, v);
	pDynaModel->SetFunctionDiff(v, -k / m * x);
}

eDEVICEFUNCTIONSTATUS CTestDevice::Init(CDynaModel* pDynaModel)
{
	A = x = 10.0;
	omega = std::sqrt(k / m);
	phi = 0;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}


VariableIndexRefVec& CTestDevice::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ x, v }, ChildVec));
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