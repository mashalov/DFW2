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

	if (m_pBranch->m_BranchState != CDynaBranch::BranchState::BRANCH_OFF)
	{
		const auto& Vip{ m_pZeroLFNode ? m_pZeroLFNode->V : pNodeIp->V };
		const auto& Viq{ m_pZeroLFNode ? m_pZeroLFNode->V : pNodeIq->V };
		const auto& Dip{ m_pZeroLFNode ? m_pZeroLFNode->Delta : pNodeIp->Delta };
		const auto& Diq{ m_pZeroLFNode ? m_pZeroLFNode->Delta : pNodeIq->Delta };
		
		const double Cosq{ cos(Diq) }, Cosp{ cos(Dip) }, Sinq{ sin(Diq) }, Sinp{ sin(Dip) };
		const double g{ m_pBranch->Yip.real() }, b{ m_pBranch->Yip.imag() };
		const double ge{ m_pBranch->Yiq.real() }, be{ m_pBranch->Yiq.imag() };
		const double gips{ m_pBranch->Yips.real() }, bips{ m_pBranch->Yips.imag() };
		const double giqs{ m_pBranch->Yiqs.real() }, biqs{ m_pBranch->Yiqs.imag() };

		if (m_pZeroLFNode)
		{
			const auto& vIpre{ pNodeIp->ZeroLF.vRe };
			const auto& vIpim{ pNodeIp->ZeroLF.vIm };
			const auto& vIqre{ pNodeIq->ZeroLF.vRe };
			const auto& vIqim{ pNodeIq->ZeroLF.vIm };

			pDynaModel->SetElement(Ibre, Ibre, 1.0);
			pDynaModel->SetElement(Ibim, Ibim, 1.0);
			pDynaModel->SetElement(Iere, Iere, 1.0);
			pDynaModel->SetElement(Ieim, Ieim, 1.0);
		}
		else
		{
			// dIbre / dIbre
			pDynaModel->SetElement(Ibre, Ibre, 1.0);
			// dIbre / dVq
			pDynaModel->SetElement(Ibre, Viq, b * Sinq - g * Cosq);
			// dIbre / dDeltaQ
			pDynaModel->SetElement(Ibre, Diq, Viq * (b * Cosq + g * Sinq));
			// dIbre / dVp
			pDynaModel->SetElement(Ibre, Vip, gips * Cosp - bips * Sinp);
			// dIbre / dDeltaP
			pDynaModel->SetElement(Ibre, Dip, -Vip * (bips * Cosp + gips * Sinp));


			// dIbim / dIbim
			pDynaModel->SetElement(Ibim, Ibim, 1.0);
			// dIbim / dVq
			pDynaModel->SetElement(Ibim, Viq, -b * Cosq - g * Sinq);
			// dIbim / dDeltaQ
			pDynaModel->SetElement(Ibim, Diq, Viq * (b * Sinq - g * Cosq));
			// dIbim / dVp
			pDynaModel->SetElement(Ibim, Vip, bips * Cosp + gips * Sinp);
			// dIbim / dDeltaP
			pDynaModel->SetElement(Ibim, Dip, Vip * (gips * Cosp - bips * Sinp));


			// dIere / dIere
			pDynaModel->SetElement(Iere, Iere, 1.0);
			// dIere / dVq
			pDynaModel->SetElement(Iere, Viq, biqs * Sinq - giqs * Cosq);
			// dIere / dDeltaQ
			pDynaModel->SetElement(Iere, Diq, Viq * (biqs * Cosq + giqs * Sinq));
			// dIere / dVp
			pDynaModel->SetElement(Iere, Vip, ge * Cosp - be * Sinp);
			// dIere / dDeltaP
			pDynaModel->SetElement(Iere, Dip, -Vip * (be * Cosp + ge * Sinp));


			// dIeim / dIeim
			pDynaModel->SetElement(Ieim, Ieim, 1.0);
			// dIeim / dVq
			pDynaModel->SetElement(Ieim, Viq, -biqs * Cosq - giqs * Sinq);
			// dIeim / dDeltaQ
			pDynaModel->SetElement(Ieim, Diq, Viq * (biqs * Sinq - giqs * Cosq));
			// dIeim / dVp
			pDynaModel->SetElement(Ieim, Vip, be * Cosp + ge * Sinp);
			// dIeim / dDeltaP
			pDynaModel->SetElement(Ieim, Dip, Vip * (ge * Cosp - be * Sinp));
		}


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
		pDynaModel->SetElement(Pb, Vip, -Ibre * Cosp - Ibim * Sinp);
		// dPb / dDeltaP
		pDynaModel->SetElement(Pb, Dip, Vip * (Ibre * Sinp - Ibim * Cosp));
		// dPb / dIbre
		pDynaModel->SetElement(Pb, Ibre, -Vip * Cosp);
		// dPb / dIbim
		pDynaModel->SetElement(Pb, Ibim, -Vip * Sinp);


		// dQb / dQb
		pDynaModel->SetElement(Qb, Qb, 1.0);
		// dQb / dVp
		pDynaModel->SetElement(Qb, Vip, Ibim * Cosp - Ibre * Sinp);
		// dQb / dDeltaP
		pDynaModel->SetElement(Qb, Dip, -Vip * (Ibre * Cosp + Ibim * Sinp));
		// dQb / dIbre
		pDynaModel->SetElement(Qb, Ibre, -Vip * Sinp);
		// dQb / dIbim
		pDynaModel->SetElement(Qb, Ibim, Vip * Cosp);


		// dPe / dPe
		pDynaModel->SetElement(Pe, Pe, 1.0);
		// dPe / dVq
		pDynaModel->SetElement(Pe, Viq, -Iere * Cosq - Ieim * Sinq);
		// dPe / dDeltaQ
		pDynaModel->SetElement(Pe, Diq, Viq * (Iere * Sinq - Ieim * Cosq));
		// dPe / dIere
		pDynaModel->SetElement(Pe, Iere, -Viq * Cosq);
		// dPe / dIeim
		pDynaModel->SetElement(Pe, Ieim, -Viq * Sinq);

		// dQe / dQe
		pDynaModel->SetElement(Qe, Qe, 1.0);
		// dQe / dVq
		pDynaModel->SetElement(Qe, Viq, Ieim * Cosq - Iere * Sinq);
		// dQe / dDeltaQ
		pDynaModel->SetElement(Qe, Diq, -Viq * (Iere * Cosq + Ieim * Sinq));
		// dQe / dIere
		pDynaModel->SetElement(Qe, Iere, -Viq * Sinq);
		// dQe / dIeim
		pDynaModel->SetElement(Qe, Ieim, Viq * Cosq);

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

	if (m_pBranch->m_BranchState != CDynaBranch::BranchState::BRANCH_OFF)
	{
		const cplx Ue{ pNodeIq->Vre, pNodeIq->Vim };
		const cplx Ub{ pNodeIp->Vre, pNodeIp->Vim };

		cplx cIb{ -m_pBranch->Yips * Ub + m_pBranch->Yip * Ue };
		cplx cIe{ -m_pBranch->Yiq * Ub + m_pBranch->Yiqs * Ue };
		cplx cSb{ Ub * conj(cIb) };
		cplx cSe{ Ue * conj(cIe) };

		if (m_pZeroLFNode)
		{
			const auto& pNode1{ m_pBranch->m_pNodeIp->ZeroLF };
			const auto& pNode2{ m_pBranch->m_pNodeIq->ZeroLF };
			cplx Current{ pNode2.vRe - pNode1.vRe, pNode2.vIm - pNode1.vIm };
			cIb += Current;		cIe += Current;
			Current = std::conj(Current) * Ub;
			cSb += Current;		cSe += Current;
		}

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
		const cplx Ue{ pBranch->m_pNodeIq->Vre, pBranch->m_pNodeIq->Vim };
		const cplx Ub{ pBranch->m_pNodeIp->Vre, pBranch->m_pNodeIp->Vim };
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
	if (m_pZeroLFNode)
	{
		const auto& pNode1{ m_pBranch->m_pNodeIp->ZeroLF };
		const auto& pNode2{ m_pBranch->m_pNodeIq->ZeroLF };
		cplx Current{ pNode2.vRe - pNode1.vRe, pNode2.vIm - pNode1.vIm };
		cIb += Current;
		cIe += Current;
		Current = std::conj(Current) * cplx(m_pBranch->m_pNodeIp->Vre, m_pBranch->m_pNodeIq->Vim);
		cSb += Current;
		cSe += Current;
	}
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
	m_pZeroLFNode = nullptr;
	// если ветвь находится внутри суперузла
	// нам потребуется расчет потокораспределения внутри
	// суперузла
	if (m_pBranch->InSuperNode())
	{
		m_pZeroLFNode = m_pBranch->m_pNodeSuperIp;
		// сообщаем контейнеру узлов, что нам нужно потокораспределение
		// внутри суперузла, в котором находится данная ветвь
		m_pZeroLFNode->RequireSuperNodeLF();
	}
	pBranch->m_pMeasure = this;
}