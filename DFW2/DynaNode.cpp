#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"
#include "GraphCycle.h"
#include "DynaGeneratorMotion.h"
#include "BranchMeasures.h"
#include <immintrin.h>

using namespace DFW2;

#define LOW_VOLTAGE 0.1	// может быть сделать в о.е. ? Что будет с узлами с низким Uном ?
#define LOW_VOLTAGE_HYST LOW_VOLTAGE * 0.1

// в узлах используется расширенная функция
// прогноза. После прогноза рассчитывается
// напряжение в декартовых координатах
// и сбрасываются значения по умолчанию перед
// итерационным процессом решения сети

void CDynaNode::Predict(const CDynaModel& DynaModel)
{
	VreVim = { Vre, Vim };
	LRCVicinity = 0.05;
	const double newDelta{ std::atan2(std::sin(Delta), std::cos(Delta)) };
	if (!Consts::Equal(newDelta, Delta))
	{
		RightVector* const pRvDelta{ GetModel()->GetRightVector(Delta.Index) };
		Delta = newDelta;
		pRvDelta->Nordsiek[0] = Delta;
	}
}

void CDynaNode::BuildEquations(CDynaModel* pDynaModel)
{
	CDynaNodeBase::BuildEquations(pDynaModel);

	const double Vre2{ Vre * Vre }, Vim2{ Vim * Vim }, V2{ Vre2 + Vim2 };

	pDynaModel->SetElement(V, V, 1.0);
	pDynaModel->SetElement(Delta, Delta, 1.0);

	if (LowVoltage)
	{
		pDynaModel->SetElement(Delta, Vre, 0.0);
		pDynaModel->SetElement(Delta, Vim, 0.0);
	}
	else
	{
		pDynaModel->SetElement(Delta, Vre, Vim / V2);
		pDynaModel->SetElement(Delta, Vim, -Vre / V2);
	}
}

void CDynaNode::BuildRightHand(CDynaModel* pDynaModel)
{
	CDynaNodeBase::BuildRightHand(pDynaModel);

	// Копируем скольжение в слэйв-узлы суперузла
	// (можно совместить с CDynaNodeBase::FromSuperNode()
	// и сэкономить цикл
	const CLinkPtrCount* const pLink{ GetSuperLink(0) };
	LinkWalker<CDynaNode> pSlaveNode;
	while (pLink->In(pSlaveNode))
		pSlaveNode->S = S;

	double DiffDelta{ Delta };
	double dDelta{ 0.0 };

	if (!LowVoltage)
		dDelta = Delta - std::atan2(Vim, Vre);

	pDynaModel->SetFunction(S, 0.0);
	pDynaModel->SetFunction(Delta, dDelta);
}

eDEVICEFUNCTIONSTATUS CDynaNode::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaNodeBase::Init(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(Status))
		S = 0.0;
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaNode::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice)
{
	eDEVICESTATE OldState{ GetState() };
	eDEVICEFUNCTIONSTATUS Status{ CDevice::SetState(eState, eStateCause) };

	if (OldState != eState)
	{
		ProcessTopologyRequest();

		switch (eState)
		{
		case eDEVICESTATE::DS_OFF:
			V = Delta = S = 0.0;
			break;
		case eDEVICESTATE::DS_ON:
			V = Unom;
			Delta = S = Vre = V = Vim = 0;
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaNode::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	SetLowVoltage(sqrt(Vre * Vre + Vim * Vim) < (LOW_VOLTAGE - LOW_VOLTAGE_HYST));
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

void CDynaNode::FinishStep(const CDynaModel& DynaModel)
{
	SyncDelta_ = pSyncZone->Delta + Delta;
	SyncDelta_ = std::atan2(std::sin(SyncDelta_), std::cos(SyncDelta_));
}

void CDynaNode::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaNodeBase::DeviceProperties(props);
	props.SetClassName(CDeviceContainerProperties::cszNameNode_, CDeviceContainerProperties::cszSysNameNode_);
	props.EquationsCount = CDynaNode::VARS::V_LAST;
	props.VarMap_.insert({ CDynaNode::cszS_, CVarIndex(V_S, VARUNIT_PU) });
	props.VarMap_.insert({ CDynaNodeBase::cszDelta_, CVarIndex(V_DELTA, VARUNIT_RADIANS) });
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaNode>>();

	props.Aliases_.push_back(CDynaNodeBase::cszAliasNode_);
	props.ConstVarMap_.insert({ CDynaNode::cszGsh_, CConstVarIndex(CDynaNode::C_GSH, VARUNIT_SIEMENS, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaNode::cszBsh_, CConstVarIndex(CDynaNode::C_BSH, VARUNIT_SIEMENS, eDVT_INTERNALCONST) });

	props.bUseCOI = true;
	props.SyncDeltaId = CDynaNode::C_SYNCDELTA;
}

void CDynaNode::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaNodeBase::UpdateSerializer(Serializer);
	Serializer->AddState("S", S);
}

VariableIndexRefVec& CDynaNode::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaNodeBase::GetVariables(JoinVariables({ Delta, S }, ChildVec));
}
