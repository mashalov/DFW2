#include "stdafx.h"
#include "BranchMeasures.h"
#include "DynaModel.h"

using namespace DFW2;

#define ABS_GUARD 0.0 // идея - добавить к вычислению производной от модуля небольшую константу, чтобы гарантированно не делить на ноль

double* CDynaBranchMeasure::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Ibre.Value, V_IBRE)
			MAP_VARIABLE(Ibim.Value, V_IBIM)
			MAP_VARIABLE(Iere.Value, V_IERE)
			MAP_VARIABLE(Ieim.Value, V_IEIM)
			MAP_VARIABLE(Ib.Value, V_IB)
			MAP_VARIABLE(Ie.Value, V_IE)
			MAP_VARIABLE(Pb.Value, V_PB)
			MAP_VARIABLE(Qb.Value, V_QB)
			MAP_VARIABLE(Pe.Value, V_PE)
			MAP_VARIABLE(Qe.Value, V_QE)
			MAP_VARIABLE(Sb.Value, V_SB)
			MAP_VARIABLE(Se.Value, V_SE)
	}
	return p;
}


bool CDynaBranchMeasure::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;

	const auto& pNodeIp{ m_pBranch->m_pNodeIp };
	const auto& pNodeIq{ m_pBranch->m_pNodeIq };

	if (pNodeIp->InMatrix() && pNodeIq->InMatrix() && m_pBranch->m_BranchState != CDynaBranch::BranchState::BRANCH_OFF)
	{
		double Cosq = cos(m_pBranch->m_pNodeIq->Delta);
		double Cosp = cos(m_pBranch->m_pNodeIp->Delta);
		double Sinq = sin(m_pBranch->m_pNodeIq->Delta);
		double Sinp = sin(m_pBranch->m_pNodeIp->Delta);

		double& Vq = m_pBranch->m_pNodeIq->V;
		double& Vp = m_pBranch->m_pNodeIp->V;

		double g = m_pBranch->Yip.real();
		double b = m_pBranch->Yip.imag();

		double ge = m_pBranch->Yiq.real();
		double be = m_pBranch->Yiq.imag();

		double gips = m_pBranch->Yips.real();
		double bips = m_pBranch->Yips.imag();

		double giqs = m_pBranch->Yiqs.real();
		double biqs = m_pBranch->Yiqs.imag();


		// dIbre / dIbre
		pDynaModel->SetElement(Ibre, Ibre, 1.0);
		// dIbre / dVq
		pDynaModel->SetElement(Ibre, pNodeIq->V, b * Sinq - g * Cosq);
		// dIbre / dDeltaQ
		pDynaModel->SetElement(Ibre, pNodeIq->Delta, Vq * b * Cosq + Vq * g * Sinq);
		// dIbre / dVp
		pDynaModel->SetElement(Ibre, pNodeIp->V, gips * Cosp - bips * Sinp);
		// dIbre / dDeltaP
		pDynaModel->SetElement(Ibre, pNodeIp->Delta, -Vp * bips * Cosp - Vp * gips * Sinp);


		// dIbim / dIbim
		pDynaModel->SetElement(Ibim, Ibim, 1.0);
		// dIbim / dVq
		pDynaModel->SetElement(Ibim, pNodeIq->V, -b * Cosq - g * Sinq);
		// dIbim / dDeltaQ
		pDynaModel->SetElement(Ibim, pNodeIq->Delta, Vq * b * Sinq - Vq * g * Cosq);
		// dIbim / dVp
		pDynaModel->SetElement(Ibim, pNodeIp->V, bips * Cosp + gips * Sinp);
		// dIbim / dDeltaP
		pDynaModel->SetElement(Ibim, pNodeIp->Delta, Vp * gips * Cosp - Vp * bips * Sinp);


		// dIere / dIere
		pDynaModel->SetElement(Iere, Iere, 1.0);
		// dIere / dVq
		pDynaModel->SetElement(Iere, pNodeIq->V, biqs * Sinq - giqs * Cosq);
		// dIere / dDeltaQ
		pDynaModel->SetElement(Iere, pNodeIq->Delta, Vq * biqs * Cosq + Vq * giqs * Sinq);
		// dIere / dVp
		pDynaModel->SetElement(Iere, pNodeIp->V, ge * Cosp - be * Sinp);
		// dIere / dDeltaP
		pDynaModel->SetElement(Iere, pNodeIp->Delta, -Vp * be * Cosp - Vp * ge * Sinp);


		// dIeim / dIeim
		pDynaModel->SetElement(Ieim, Ieim, 1.0);
		// dIeim / dVq
		pDynaModel->SetElement(Ieim, pNodeIq->V, -biqs * Cosq - giqs * Sinq);
		// dIeim / dDeltaQ
		pDynaModel->SetElement(Ieim, pNodeIq->Delta, Vq * biqs * Sinq - Vq * giqs * Cosq);
		// dIeim / dVp
		pDynaModel->SetElement(Ieim, pNodeIp->V, be * Cosp + ge * Sinp);
		// dIeim / dDeltaP
		pDynaModel->SetElement(Ieim, pNodeIp->Delta, Vp * ge * Cosp - Vp * be * Sinp);


		double absIb = sqrt(Ibre * Ibre + Ibim * Ibim);

		// dIb / dIb
		pDynaModel->SetElement(Ib, Ib, 1.0);
		// dIb / dIbre
		pDynaModel->SetElement(Ib, Ibre, -CDevice::ZeroDivGuard(Ibre, absIb));
		// dIb / dIbim
		pDynaModel->SetElement(Ib, Ibim, -CDevice::ZeroDivGuard(Ibim, absIb));

		absIb = sqrt(Iere * Iere + Ieim * Ieim);

		// dIe / dIe
		pDynaModel->SetElement(Ie, Ie, 1.0);
		// dIe / dIbre
		pDynaModel->SetElement(Ie, Iere, -CDevice::ZeroDivGuard(Iere, absIb));
		// dIe / dIbim
		pDynaModel->SetElement(Ie, Ieim, -CDevice::ZeroDivGuard(Ieim, absIb));


		// dPb / dPb
		pDynaModel->SetElement(Pb, Pb, 1.0);
		// dPb / dVp
		pDynaModel->SetElement(Pb, pNodeIp->V, -Ibre * Cosp - Ibim * Sinp);
		// dPb / dDeltaP
		pDynaModel->SetElement(Pb, pNodeIp->Delta, Ibre * Vp * Sinp - Ibim * Vp * Cosp);
		// dPb / dIbre
		pDynaModel->SetElement(Pb, Ibre, -Vp * Cosp);
		// dPb / dIbim
		pDynaModel->SetElement(Pb, Ibim, -Vp * Sinp);


		// dQb / dQb
		pDynaModel->SetElement(Qb, Qb, 1.0);
		// dQb / dVp
		pDynaModel->SetElement(Qb, pNodeIp->V, Ibim * Cosp - Ibre * Sinp);
		// dQb / dDeltaP
		pDynaModel->SetElement(Qb, pNodeIp->Delta, -Ibre * Vp * Cosp - Ibim * Vp * Sinp);
		// dQb / dIbre
		pDynaModel->SetElement(Qb, Ibre, -Vp * Sinp);
		// dQb / dIbim
		pDynaModel->SetElement(Qb, Ibim, Vp * Cosp);


		// dPe / dPe
		pDynaModel->SetElement(Pe, Pe, 1.0);
		// dPe / dVq
		pDynaModel->SetElement(Pe, pNodeIq->V, -Iere * Cosq - Ieim * Sinq);
		// dPe / dDeltaQ
		pDynaModel->SetElement(Pe, pNodeIq->Delta, Iere * Vq * Sinq - Ieim * Vq * Cosq);
		// dPe / dIere
		pDynaModel->SetElement(Pe, Iere, -Vq * Cosq);
		// dPe / dIeim
		pDynaModel->SetElement(Pe, Ieim, -Vq * Sinq);

		// dQe / dQe
		pDynaModel->SetElement(Qe, Qe, 1.0);
		// dQe / dVq
		pDynaModel->SetElement(Qe, pNodeIq->V, Ieim * Cosq - Iere * Sinq);
		// dQe / dDeltaQ
		pDynaModel->SetElement(Qe, pNodeIq->Delta, -Iere * Vq * Cosq - Ieim * Vq * Sinq);
		// dQe / dIere
		pDynaModel->SetElement(Qe, Iere, -Vq * Sinq);
		// dQe / dIeim
		pDynaModel->SetElement(Qe, Ieim, Vq * Cosq);

		absIb = std::sqrt(Pb * Pb + Qb * Qb + ABS_GUARD);

		// dSb / dSb
		pDynaModel->SetElement(Sb, Sb, 1.0);
		// dSb / dPb
		pDynaModel->SetElement(Sb, Pb, -CDevice::ZeroDivGuard(Pb, absIb));
		// dSb / dQb
		pDynaModel->SetElement(Sb, Qb, -CDevice::ZeroDivGuard(Qb, absIb));

		absIb = std::sqrt(Pe * Pe + Qe * Qe + ABS_GUARD);

		if (absIb < DFW2_EPSILON)
			absIb = DFW2_EPSILON;

		// dSe / dSe
		pDynaModel->SetElement(Se, Se, 1.0);
		// dSe / dPe
		pDynaModel->SetElement(Se, Pe, -CDevice::ZeroDivGuard(Pe, absIb));
		// dSe / dQe
		pDynaModel->SetElement(Se, Qe, -CDevice::ZeroDivGuard(Qe, absIb));
	}
	else
	{
		pDynaModel->SetElement(Ibre, Ibre, 1.0);
		pDynaModel->SetElement(Ibim, Ibim, 1.0);
		pDynaModel->SetElement(Iere, Iere, 1.0);
		pDynaModel->SetElement(Ieim, Ieim, 1.0);
		pDynaModel->SetElement(Ib, Ib, 1.0);
		pDynaModel->SetElement(Ie, Ie, 1.0);
		pDynaModel->SetElement(Pb, Pb, 1.0);
		pDynaModel->SetElement(Qb, Qb, 1.0);
		pDynaModel->SetElement(Pe, Pe, 1.0);
		pDynaModel->SetElement(Qe, Qe, 1.0);
		pDynaModel->SetElement(Sb, Sb, 1.0);
		pDynaModel->SetElement(Se, Se, 1.0);
	}


	return true;
}


bool CDynaBranchMeasure::BuildRightHand(CDynaModel* pDynaModel)
{
	const auto& pNodeIp{ m_pBranch->m_pNodeIp };
	const auto& pNodeIq{ m_pBranch->m_pNodeIq };
	pNodeIp->UpdateVreVim();	pNodeIq->UpdateVreVim();

	if (pNodeIp->InMatrix() && pNodeIq->InMatrix() && m_pBranch->m_BranchState != CDynaBranch::BranchState::BRANCH_OFF)
	{
		const cplx& Ue = cplx(pNodeIq->Vre, pNodeIq->Vim);
		const cplx& Ub = cplx(pNodeIp->Vre, pNodeIp->Vim);

		cplx cIb = -m_pBranch->Yips * Ub + m_pBranch->Yip * Ue;
		cplx cIe = -m_pBranch->Yiq * Ub + m_pBranch->Yiqs * Ue;
		cplx cSb = Ub * conj(cIb);
		cplx cSe = Ue * conj(cIe);

		pDynaModel->SetFunction(Ibre, Ibre - cIb.real());
		pDynaModel->SetFunction(Ibim, Ibim - cIb.imag());
		pDynaModel->SetFunction(Iere, Iere - cIe.real());
		pDynaModel->SetFunction(Ieim, Ieim - cIe.imag());
		pDynaModel->SetFunction(Ib, Ib - std::sqrt(Ibre * Ibre + Ibim * Ibim + ABS_GUARD));
		pDynaModel->SetFunction(Ie, Ie - std::sqrt(Iere * Iere + Ieim * Ieim + ABS_GUARD));
		pDynaModel->SetFunction(Pb, Pb - cSb.real());
		pDynaModel->SetFunction(Qb, Qb - cSb.imag());
		pDynaModel->SetFunction(Pe, Pe - cSe.real());
		pDynaModel->SetFunction(Qe, Qe - cSe.imag());
		pDynaModel->SetFunction(Sb, Sb - std::sqrt(Pb * Pb + Qb * Qb + ABS_GUARD));
		pDynaModel->SetFunction(Se, Se - std::sqrt(Pe * Pe + Qe * Qe + ABS_GUARD));
	}
	else
	{
		pDynaModel->SetFunction(Ibre, 0.0);
		pDynaModel->SetFunction(Ibim, 0.0);
		pDynaModel->SetFunction(Iere, 0.0);
		pDynaModel->SetFunction(Ieim, 0.0);
		pDynaModel->SetFunction(Ib, 0.0);
		pDynaModel->SetFunction(Ie, 0.0);
		pDynaModel->SetFunction(Pb, 0.0);
		pDynaModel->SetFunction(Qb, 0.0);
		pDynaModel->SetFunction(Pe, 0.0);
		pDynaModel->SetFunction(Qe, 0.0);
		pDynaModel->SetFunction(Sb, 0.0);
		pDynaModel->SetFunction(Se, 0.0);
	}

	return true;
}

eDEVICEFUNCTIONSTATUS CDynaBranchMeasure::Init(CDynaModel* pDynaModel)
{
	return ProcessDiscontinuity(pDynaModel);
}

void CDynaBranchMeasure::CalculateFlows(const CDynaBranch* pBranch, cplx& cIb, cplx& cIe, cplx& cSb, cplx& cSe)
{
	// !!!!!!!!!!!!!   здесь рассчитываем на то, что для узлов начала и конца были сделаны UpdateVreVim !!!!!!!!!!!!!!
	if (pBranch->m_BranchState != CDynaBranch::BranchState::BRANCH_OFF)
	{
		const cplx& Ue = cplx(pBranch->m_pNodeIq->Vre, pBranch->m_pNodeIq->Vim);
		const cplx& Ub = cplx(pBranch->m_pNodeIp->Vre, pBranch->m_pNodeIp->Vim);
		cIb = -pBranch->Yips * Ub + pBranch->Yip * Ue;
		cIe = -pBranch->Yiq * Ub + pBranch->Yiqs * Ue;
		cSb = Ub * conj(cIb);
		cSe = Ue * conj(cIe);
	}
	else
		cIb = cIe = cSb = cSe = 0.0;
}

VariableIndexRefVec& CDynaBranchMeasure::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Ibre, Ibim, Iere, Ieim, Ib, Ie, Pb, Qb, Pe, Qe, Sb, Se }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CDynaBranchMeasure::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	const auto& pNodeIp{ m_pBranch->m_pNodeIp };
	const auto& pNodeIq{ m_pBranch->m_pNodeIq };
	pNodeIq->UpdateVreVim();	pNodeIp->UpdateVreVim();
	cplx cIb, cIe, cSb, cSe;
	CDynaBranchMeasure::CalculateFlows(m_pBranch, cIb, cIe, cSb, cSe);
	FromComplex(Ibre, Ibim, cIb);
	FromComplex(Iere, Ieim, cIe);
	FromComplex(Pb, Qb, cSb);
	FromComplex(Pe, Qe, cSe);
	Sb = std::sqrt(Pb * Pb + Qb * Qb + ABS_GUARD);
	Se = std::sqrt(Pe * Pe + Qe * Qe + ABS_GUARD);
	Ib = std::sqrt(Ibre * Ibre + Ibim * Ibim + ABS_GUARD);
	Ie = std::sqrt(Iere * Iere + Ieim * Ieim + ABS_GUARD);
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

// описание переменных расчетных потоков по ветви
void CDynaBranchMeasure::DeviceProperties(CDeviceContainerProperties& props)
{
	// линковка делается только с ветвями, поэтому описание
	// правил связывания не нужно
	props.SetType(DEVTYPE_BRANCHMEASURE);
	props.SetClassName(CDeviceContainerProperties::m_cszNameBranchMeasure, CDeviceContainerProperties::m_cszSysNameBranchMeasure);
	props.nEquationsCount = CDynaBranchMeasure::VARS::V_LAST;
	props.m_VarMap.insert({ m_cszIbre,  CVarIndex(CDynaBranchMeasure::V_IBRE, VARUNIT_KAMPERES) });
	props.m_VarMap.insert({ m_cszIbim,  CVarIndex(CDynaBranchMeasure::V_IBIM, VARUNIT_KAMPERES) });
	props.m_VarMap.insert({ m_cszIere,  CVarIndex(CDynaBranchMeasure::V_IERE, VARUNIT_KAMPERES) });
	props.m_VarMap.insert({ m_cszIeim,  CVarIndex(CDynaBranchMeasure::V_IEIM, VARUNIT_KAMPERES) });
	props.m_VarMap.insert({ m_cszIb,	CVarIndex(CDynaBranchMeasure::V_IB, VARUNIT_KAMPERES) });
	props.m_VarMap.insert({ m_cszIe,	CVarIndex(CDynaBranchMeasure::V_IE, VARUNIT_KAMPERES) });
	props.m_VarMap.insert({ m_cszPb,	CVarIndex(CDynaBranchMeasure::V_PB, VARUNIT_MW) });
	props.m_VarMap.insert({ m_cszQb,	CVarIndex(CDynaBranchMeasure::V_QB, VARUNIT_MVAR) });
	props.m_VarMap.insert({ m_cszPe,	CVarIndex(CDynaBranchMeasure::V_PE, VARUNIT_MW) });
	props.m_VarMap.insert({ m_cszQe,	CVarIndex(CDynaBranchMeasure::V_QE, VARUNIT_MVAR) });
	props.m_VarMap.insert({ m_cszSb,	CVarIndex(CDynaBranchMeasure::V_SB, VARUNIT_MVA) });
	props.m_VarMap.insert({ m_cszSe,	CVarIndex(CDynaBranchMeasure::V_SE, VARUNIT_MVA) });

	props.m_VarAliasMap.insert({ { "ib", m_cszIb }, { "ie", m_cszIe } });

	// измерения создаются индивидуально с ветвью в конструкторе
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaBranchMeasure>>();
}

void CDynaBranchMeasure::SetBranch(CDynaBranch* pBranch)
{
	if (m_pBranch)
		throw dfw2error("CDynaBranchMeasure::SetBranch - branch already set");
	SetId(pBranch->GetId());
	SetName(pBranch->GetVerbalName());
	m_pBranch = pBranch;
	pBranch->m_pMeasure = this;
}