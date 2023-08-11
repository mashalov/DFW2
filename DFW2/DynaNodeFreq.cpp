#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"
#include "GraphCycle.h"
#include "DynaGeneratorMotion.h"
#include "BranchMeasures.h"
#include <immintrin.h>

using namespace DFW2;

void CDynaNodeDerlag::Predict(const CDynaModel& DynaModel)
{
	VreVim = { Vre, Vim };
	LRCVicinity = 0.05;
	const double newDelta{ std::atan2(std::sin(Delta), std::cos(Delta)) };
	if (!Consts::Equal(newDelta, Delta))
	{
		RightVector* const pRvDelta{ GetModel()->GetRightVector(Delta.Index) };
		RightVector* const pRvLag{ GetModel()->GetRightVector(Lag.Index) };
		const double dDL{ Delta - Lag };
		Delta = newDelta;
		Lag = Delta - dDL;
		if (DynaModel.UseCOI())
			Lag += pSyncZone->Delta;
		pRvDelta->Nordsiek[0] = Delta;
		pRvLag->Nordsiek[0] = Lag;
	}
}

void CDynaNodeDerlag::BuildDerivatives(CDynaModel* pDynaModel)
{
	double DiffDelta{ Delta };
	if (pDynaModel->UseCOI())
		DiffDelta += pSyncZone->Delta;
	pDynaModel->SetDerivative(Lag, (DiffDelta - Lag) / pDynaModel->GetFreqTimeConstant());
}

void CDynaNodeDerlag::BuildEquations(CDynaModel* pDynaModel)
{
	CDynaNode::BuildEquations(pDynaModel);

	const double T{ pDynaModel->GetFreqTimeConstant() }, w0{ pDynaModel->GetOmega0() };
	
	pDynaModel->SetElement(Lag, Delta, -1.0 / T);
	if (pDynaModel->UseCOI())
		pDynaModel->SetElement(Lag, pSyncZone->Delta, -1.0 / T);
	pDynaModel->SetElement(Lag, Lag, -1.0 / T);

	if (!pDynaModel->IsInDiscontinuityMode())
	{
		pDynaModel->SetElement(S, Delta, -1.0 / T / w0);
		if (pDynaModel->UseCOI())
			pDynaModel->SetElement(S, pSyncZone->Delta, -1.0 / T / w0);
		pDynaModel->SetElement(S, Lag, 1.0 / T / w0);
		if (pDynaModel->UseCOI())
			pDynaModel->SetElement(S, pSyncZone->S, -1.0);
		pDynaModel->SetElement(S, S, 1.0);
	}
	else
	{
		pDynaModel->SetElement(S, Delta, 0.0);
		if (pDynaModel->UseCOI())
			pDynaModel->SetElement(S, pSyncZone->Delta, 0.0);
		pDynaModel->SetElement(S, Lag, 0.0);
		if (pDynaModel->UseCOI())
			pDynaModel->SetElement(S, pSyncZone->S, 0.0);
		pDynaModel->SetElement(S, S, 1.0);
	}
}

void CDynaNodeDerlag ::BuildRightHand(CDynaModel* pDynaModel)
{
	CDynaNode::BuildRightHand(pDynaModel);

	const double T{ pDynaModel->GetFreqTimeConstant() };
	const double w0{ pDynaModel->GetOmega0() };

	double DiffDelta{ Delta };

	if (pDynaModel->UseCOI())
		DiffDelta += pSyncZone->Delta;

	const double dLag{ (DiffDelta - Lag) / T };
	double dS{ S - (DiffDelta - Lag) / T / w0 };

	double dDelta{ 0.0 };

	if (pDynaModel->IsInDiscontinuityMode())
		dS = 0.0;

	if (!LowVoltage)
		dDelta = Delta - std::atan2(Vim, Vre);

	pDynaModel->SetFunctionDiff(Lag, dLag);
	pDynaModel->SetFunction(S, dS);
	pDynaModel->SetFunction(Delta, dDelta);
}

eDEVICEFUNCTIONSTATUS CDynaNodeDerlag ::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaNode::Init(pDynaModel) };
	if (CDevice::IsFunctionStatusOK(Status))
		Lag = Delta;
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaNodeDerlag::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice)
{
	eDEVICESTATE OldState{ GetState() };
	eDEVICEFUNCTIONSTATUS Status{ CDynaNode::SetState(eState, eStateCause) };

	if (OldState != eState)
		Lag = 0.0;
	return Status;
}


eDEVICEFUNCTIONSTATUS CDynaNodeDerlag::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	Lag = Delta - S * pDynaModel->GetFreqTimeConstant() * pDynaModel->GetOmega0();
	if (pDynaModel->UseCOI())
		Lag += pSyncZone->Delta;

	return CDynaNode::ProcessDiscontinuity(pDynaModel);
}

void CDynaNodeDerlag::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaNode::UpdateSerializer(Serializer);
	Serializer->AddState("SLag", Lag);
}

VariableIndexRefVec& CDynaNodeDerlag::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaNode::GetVariables(JoinVariables({ Lag }, ChildVec));
}

void CDynaNodeDerlag::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaNode::DeviceProperties(props);
	props.EquationsCount = VARS::V_LAST;
	props.DeviceFactory = std::make_unique <CDeviceFactory<CDynaNodeDerlag>> ();
}