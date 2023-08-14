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

void CDynaNodeDerlag::BuildRightHand(CDynaModel* pDynaModel)
{
	CDynaNode::BuildRightHand(pDynaModel);

	const double T{ pDynaModel->GetFreqTimeConstant() };
	const double w0{ pDynaModel->GetOmega0() };

	double DiffDelta{ Delta };

	if (pDynaModel->UseCOI())
		DiffDelta += pSyncZone->Delta;

	const double dLag{ (DiffDelta - Lag) / T };
	double dS{ S - (DiffDelta - Lag) / T / w0 };

	if (pDynaModel->IsInDiscontinuityMode())
		dS = 0.0;

	pDynaModel->SetFunctionDiff(Lag, dLag);
	pDynaModel->SetFunction(S, dS);
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

void CDynaNodeFreqDivider::Predict(const CDynaModel& DynaModel)
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

void CDynaNodeFreqDivider::BuildEquations(CDynaModel* pDynaModel)
{
	CDynaNode::BuildEquations(pDynaModel);

	if (pDynaModel->FillConstantElements())
	{
		// все коэффициенты в матрице Якоби постоянные :thumb

		const CLinkPtrCount* const pGenLink{ GetSuperLink(2) };
		LinkWalker<CDynaPowerInjector> pGen;

		double dS{ 0.0 };

		while (pGenLink->InMatrix(pGen) && pGen->IsKindOfType(DEVTYPE_GEN_MOTION))
		{
			const CDynaGeneratorMotion* pSlipGen{ static_cast<const CDynaGeneratorMotion*>(static_cast<const CDynaPowerInjector*>(pGen)) };
			const double Bg{ 1.0 / pSlipGen->Zgen().imag() };
			pDynaModel->SetElement(S, pSlipGen->s, -Bg);
		}


		for (VirtualBranch* pV = pVirtualBranchBegin_; pV < pVirtualBranchEnd_; pV++)
			pDynaModel->SetElement(S, static_cast<const CDynaNodeFreqDivider*>(pV->pNode)->S, pV->Y.imag());

		pDynaModel->SetElement(S, S, BBranchSum_);
		pDynaModel->SetElement(S, pSyncZone->S, -BGenSum_);

		pDynaModel->CountConstElementsToSkip(S);

	}
	else
		pDynaModel->SkipConstElements(S);
}

void CDynaNodeFreqDivider::BuildRightHand(CDynaModel* pDynaModel)
{
	CDynaNode::BuildRightHand(pDynaModel);

	const CLinkPtrCount* const pGenLink{ GetSuperLink(2) };
	LinkWalker<CDynaPowerInjector> pGen;

	double dS{ 0.0 };

	/*
	if (GetId() == 10041 && pDynaModel->GetIntegrationStepNumber() > 6327)
		DebugLog("");
	*/



	// FrequencyDivider на самом деле аналогичен решениею узловых напряжений.
	// Вместо напряжений берем скольжения в узлах, вместо токов - скольжения СМ.
	// Знаки проводимостей везде "+", как в задаче на DC

	// собираем скольжения со взвешиванием по проводимости
	// ШБМ - предполагаем s=0, проводимость ШБМ - в BranchGenSum_

	while (pGenLink->InMatrix(pGen) && pGen->IsKindOfType(DEVTYPE_GEN_MOTION))
	{
		const CDynaGeneratorMotion* pSlipGen{ static_cast<const CDynaGeneratorMotion*>(static_cast<const CDynaPowerInjector*>(pGen)) };
		const double Bg{ 1.0 / pSlipGen->Zgen().imag() };
		dS -= (static_cast<double>(pSlipGen->s)  + pSyncZone->S) * Bg;
	}

	for (VirtualBranch* pV = pVirtualBranchBegin_; pV < pVirtualBranchEnd_; pV++)
		dS += static_cast<const CDynaNodeFreqDivider*>(pV->pNode)->S * pV->Y.imag();

	dS += BBranchSum_ * S;
	dS -= BGenSum_ * pSyncZone->S;

	pDynaModel->SetFunction(S, dS);
}

eDEVICEFUNCTIONSTATUS CDynaNodeFreqDivider::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaNode::Init(pDynaModel) };
	return Status;
}

void CDynaNodeFreqDivider::CalculateShuntParts()
{
	CDynaNodeBase::CalculateShuntParts();
	// сумма проводимостей генераторов в узлах
	// включая ШБМ
	BGenSum_ = 0.0;

	const CLinkPtrCount* const pGenLink{ GetSuperLink(2) };
	LinkWalker<CDynaPowerInjector> pGen;
	while (pGenLink->InMatrix(pGen) && pGen->IsKindOfType(DEVTYPE_GEN_INFPOWER))
	{
		const CDynaGeneratorInfBus* pXGen{ static_cast<const CDynaGeneratorInfBus*>(static_cast<const CDynaPowerInjector*>(pGen)) };
		BGenSum_ += 1.0 / pXGen->Zgen().imag();
	}

	// сумма проводимостей генераторов и проводимостей ветвей
	// (знаки везде "+")
	BBranchSum_ = BGenSum_;

	for (VirtualBranch* pV = pVirtualBranchBegin_; pV < pVirtualBranchEnd_; pV++)
		BBranchSum_ -= pV->Y.imag();
}

eDEVICEFUNCTIONSTATUS CDynaNodeFreqDivider::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	/*Lag = Delta - S * pDynaModel->GetFreqTimeConstant() * pDynaModel->GetOmega0();
	if (pDynaModel->UseCOI())
		Lag += pSyncZone->Delta;
	*/
	return CDynaNode::ProcessDiscontinuity(pDynaModel);
}

void CDynaNodeFreqDivider::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaNode::DeviceProperties(props);
	props.DeviceFactory = std::make_unique <CDeviceFactory<CDynaNodeFreqDivider>>();
}