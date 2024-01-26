#include "stdafx.h"
#include "SyncroZone.h"
#include "DynaModel.h"
#include "DynaGeneratorMotion.h"

using namespace DFW2;

VariableIndexRefVec& CSynchroZone::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ S, Delta }, ChildVec));
}

double* CSynchroZone::GetVariablePtr(ptrdiff_t nVarIndex)
{
	return &GetVariable(nVarIndex).Value;
}

void CSynchroZone::BuildEquations(CDynaModel* pDynaModel)
{
	pDynaModel->SetElement(Delta, Delta, 0.0);
	if (InfPower)
	{
		pDynaModel->SetElement(S, S, 1.0);
	}
	else
	{
		pDynaModel->SetElement(S, S, 1.0);
		pDynaModel->SetElement(Delta, S, -1.0);
		for (auto&& it : LinkedGenerators)
		{
			if (it->IsKindOfType(DEVTYPE_GEN_MOTION))
			{
				const auto& pGenMj{ static_cast<CDynaGeneratorMotion*>(it) };
				pDynaModel->SetElement(S, pGenMj->s, -pGenMj->Mj / Mj);
			}
		}
	}
}

double CSynchroZone::CalculateS() const
{
	double CurrentS{ 0.0 };
	if (!InfPower)
	{
		for (auto&& it : LinkedGenerators)
		{
			if (it->IsKindOfType(DEVTYPE_GEN_MOTION))
			{
				const auto& pGenMj{ static_cast<CDynaGeneratorMotion*>(it) };
				CurrentS += pGenMj->Mj * pGenMj->s / Mj;
			}
		}
	}
	return CurrentS;
}

void CSynchroZone::BuildRightHand(CDynaModel* pDynaModel)
{
	double dS{ S };
	if (InfPower)
	{
		pDynaModel->SetFunction(S, 0.0);
		pDynaModel->SetFunctionDiff(Delta, 0.0);
	}
	else
	{
		pDynaModel->SetFunction(S, S - CalculateS());
		pDynaModel->SetFunctionDiff(Delta, S * pDynaModel->GetOmega0());
	}
}


eDEVICEFUNCTIONSTATUS CSynchroZone::Init(CDynaModel* pDynaModel)
{
	Delta = S = 0.0;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CSynchroZone::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	S = CalculateS();
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

void CSynchroZone::AnglesToSyncReference()
{
	for (auto&& gen : LinkedGenerators)
		static_cast<CDynaGeneratorInfBus*>(gen)->AngleToSyncReference();
}

void CSynchroZone::DeviceProperties(CDeviceContainerProperties& props)
{
	props.bVolatile = true;
	props.eDeviceType = DEVTYPE_SYNCZONE;
	props.EquationsCount = CSynchroZone::VARS::V_LAST;
	props.VarMap_.insert(std::make_pair(CDynaNode::cszSz_, CVarIndex(CSynchroZone::VARS::V_S, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair(CDynaNode::cszSyncDelta_, CVarIndex(CSynchroZone::VARS::V_DELTA, VARUNIT_PU)));
	props.DeviceFactory = std::make_unique<CDeviceFactory<CSynchroZone>>();
}
