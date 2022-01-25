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


void CDynaBranchMeasure::BuildEquations(CDynaModel* pDynaModel)
{
	// если измерение внутри суперузла, выбираем исходные узлы начала и конца,
    // если измерение между суперузлами - выбираем суперузлы начала и конца
	const auto& pNodeIp{ m_pZeroLFNode ? m_pBranch->m_pNodeIp : m_pBranch->m_pNodeSuperIp };
	const auto& pNodeIq{ m_pZeroLFNode ? m_pBranch->m_pNodeIq : m_pBranch->m_pNodeSuperIq };

	const double gips{ m_pBranch->Yips.real() }, bips{ m_pBranch->Yips.imag() };
	const double giqs{ m_pBranch->Yiqs.real() }, biqs{ m_pBranch->Yiqs.imag() };
	const double gip{ m_pBranch->Yip.real() }, bip{ m_pBranch->Yip.imag() };
	const double giq{ m_pBranch->Yiq.real() }, biq{ m_pBranch->Yiq.imag() };

	if (m_pBranch->m_BranchState != CDynaBranch::BranchState::BRANCH_OFF)
	{
		// если измерение внутри суперузла - берем не исходное напряжение
		// в ограничивающих узлах, а напряжение суперузла для расчета поперечных потерь
		const auto& Vbre{ m_pZeroLFNode ? m_pZeroLFNode->Vre : pNodeIp->Vre };
		const auto& Vbim{ m_pZeroLFNode ? m_pZeroLFNode->Vim : pNodeIp->Vim };
		const auto& Vere{ m_pZeroLFNode ? m_pZeroLFNode->Vre : pNodeIq->Vre };
		const auto& Veim{ m_pZeroLFNode ? m_pZeroLFNode->Vim : pNodeIq->Vim };

		if (m_pZeroLFNode)
		{
			const auto& vbRe{ pNodeIp->ZeroLF.vRe };
			const auto& vbIm{ pNodeIp->ZeroLF.vIm };
			const auto& veRe{ pNodeIq->ZeroLF.vRe };
			const auto& veIm{ pNodeIq->ZeroLF.vIm };

			// проводимости в начале и в конце ветви с нулевым сопротивлением, при условии,
			// что напряжения в конце и в начале одинаковы
			const double gsb{ gips - gip }, bsb{ bips - bip }, gse{ giq - giqs }, bse{ biqs - biq };

			// Ibre + gsb * Vbre - bsb * Vbim - veRe + vbRe = 0
			// Ibim + bsb * Vbre + gsb * Vbim - veIm + vbIm = 0
					
			// dIbre / dIbre
			pDynaModel->SetElement(Ibre, Ibre, 1.0);
			// dIbre / dVbre
			pDynaModel->SetElement(Ibre, Vbre, gsb);
			// dIbre / dVbim
			pDynaModel->SetElement(Ibre, Vbim, -bsb);
			// индикаторы могут быть неиндексированы, если узел индикатора
			// был выбран в качестве базисного, в этом случае просто пропускаем
			// элемент матрицы
			// dIbre / dveRe
			if(veRe.Index >= 0) pDynaModel->SetElement(Ibre, veRe, -1.0);
			// dIbre / dvbRe 
			if(vbRe.Index >= 0) pDynaModel->SetElement(Ibre, vbRe, 1.0);

			// dIbim / dIbim
			pDynaModel->SetElement(Ibim, Ibim, 1.0);
			// dIbim / dVbre
			pDynaModel->SetElement(Ibim, Vbre, bsb);
			// dIbim / dVbim
			pDynaModel->SetElement(Ibim, Vbim, gsb);
			// dIbim / dveIm
			if(veIm.Index >= 0) pDynaModel->SetElement(Ibim, veIm, -1.0);
			// dIbim / dvbIm
			if(vbIm.Index >= 0) pDynaModel->SetElement(Ibim, vbIm, 1.0);


			// Iere + gse * Vbre + bse * Vbim - veRe + vbRe = 0
			// Ieim - bse * Vbre + gse * Vbim - veRe + vbRe = 0

			// dIere / dIere
			pDynaModel->SetElement(Iere, Iere, 1.0);
			// dIere / dVbre
			pDynaModel->SetElement(Iere, Vbre, gse);
			// dIere / dVbim
			pDynaModel->SetElement(Iere, Vbim, bse);
			// dIere / dveIm
			if (veRe.Index >= 0) pDynaModel->SetElement(Iere, veIm, -1.0);
			// dIere / dvbIm
			if (vbRe.Index >= 0) pDynaModel->SetElement(Iere, vbIm, 1.0);


			// dIeim / dIeim
			pDynaModel->SetElement(Ieim, Ieim, 1.0);
			// dIbim / dVbre
			pDynaModel->SetElement(Ieim, Vbre, -bse);
			// dIbim / dVbim
			pDynaModel->SetElement(Ieim, Vbim, gse);
			// dIbim / dveIm
			if (veIm.Index >= 0) pDynaModel->SetElement(Ieim, veIm, -1.0);
			// dIbim / dvbIm
			if (vbIm.Index >= 0) pDynaModel->SetElement(Ieim, vbIm, 1.0);
		}
		else
		{
			// Ibre + gips * Vbre - bips * Vbim - gip * Vere + bip * Veim = 0
			// 
			// dIbre / dIbre
			pDynaModel->SetElement(Ibre, Ibre, 1.0);
			// dIbre / dVbre
			pDynaModel->SetElement(Ibre, Vbre, gips);
			// dIbre / dVbim
			pDynaModel->SetElement(Ibre, Vbim, -bips);
			// dIbre / dVere
			pDynaModel->SetElement(Ibre, Vere, -gip);
			// dIbre / dVeim
			pDynaModel->SetElement(Ibre, Veim, bip);

			// Ibim + bips * Vbre + gips * Vbim - bip * Vere - gip * Veim = 0
			// 
			// dIbim / dIbim
			pDynaModel->SetElement(Ibim, Ibim, 1.0);
			// dIbim / dVbre
			pDynaModel->SetElement(Ibim, Vbre, bips);
			// dIbim / dVbim
			pDynaModel->SetElement(Ibim, Vbim, gips);
			// dIbim / dVere
			pDynaModel->SetElement(Ibim, Vere, -bip);
			// dIbim / dVeim
			pDynaModel->SetElement(Ibim, Veim, -gip);

			// Iere + giq * Vbre - biq * Vbim - giqs * Vere + biqs * Veim = 0
			// 
			// dIere / dIere
			pDynaModel->SetElement(Iere, Iere, 1.0);
			// dIere / dVbre
			pDynaModel->SetElement(Iere, Vbre, giq);
			// dIere / dVbim
			pDynaModel->SetElement(Iere, Vbim, -biq);
			// dIere / dVere
			pDynaModel->SetElement(Iere, Vere, -giqs);
			// dIere / dVeim
			pDynaModel->SetElement(Iere, Veim, biqs);


			// Ieim + biq * Vbre + giq * Vbim - biqs * Vere - giqs * Veim = 0
			// 
			// dIeim / dIeim
			pDynaModel->SetElement(Ieim, Ieim, 1.0);
			// dIeim / dVbre
			pDynaModel->SetElement(Ieim, Vbre, biq);
			// dIeim / dVbim
			pDynaModel->SetElement(Ieim, Vbim, giq);
			// dIeim / dVere
			pDynaModel->SetElement(Ieim, Vere, -biqs);
			// dIeim / dVeim
			pDynaModel->SetElement(Ieim, Veim, -giqs);
		}

		double absIb{ sqrt(Ibre * Ibre + Ibim * Ibim) };

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


		// Pb - Vbre * Ibre - Vbim * Ibim = 0
		// 
		// dPb / dPb
		pDynaModel->SetElement(Pb, Pb, 1.0);
		// dPb / dVbre
		pDynaModel->SetElement(Pb, Vbre, -Ibre);
		// dPb / dVbim
		pDynaModel->SetElement(Pb, Vbim, -Ibim);
		// dPb / dIbre
		pDynaModel->SetElement(Pb, Ibre, -Vbre);
		// dPb / dIbim
		pDynaModel->SetElement(Pb, Ibim, -Vbim);

		// Qb - Vbim * Ibre + Vbre * Ibim = 0
		// 
		// dQb / dQb
		pDynaModel->SetElement(Qb, Qb, 1.0);
		// dQb / dVbre
		pDynaModel->SetElement(Qb, Vbre,  Ibim);
		// dQb / dVbim
		pDynaModel->SetElement(Qb, Vbim, -Ibre);
		// dQb / dIbre
		pDynaModel->SetElement(Qb, Ibre, -Vbim);
		// dQb / dIbim
		pDynaModel->SetElement(Qb, Ibim,  Vbre);

		// Pe - Vere * Iere - Veim * Ieim = 0
		// 
		// dPe / dPe
		pDynaModel->SetElement(Pe, Pe, 1.0);
		// dPe / dVere
		pDynaModel->SetElement(Pe, Vere, -Iere);
		// dPe / dVeim
		pDynaModel->SetElement(Pe, Veim, -Ieim);
		// dPe / dIere
		pDynaModel->SetElement(Pe, Iere, -Vere);
		// dPe / dIeim
		pDynaModel->SetElement(Pe, Ieim, -Veim);

		// Qe - Veim * Iere + Vere * Ieim = 0
		// 
		// dQe / dQe
		pDynaModel->SetElement(Qe, Qe, 1.0);
		// dQe / dVere
		pDynaModel->SetElement(Qe, Vere,  Ieim);
		// dQe / dVeim
		pDynaModel->SetElement(Qe, Veim, -Iere);
		// dQe / dIere
		pDynaModel->SetElement(Qe, Iere, -Veim);
		// dQe / dIeim
		pDynaModel->SetElement(Qe, Ieim,  Vere);

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
}


void CDynaBranchMeasure::BuildRightHand(CDynaModel* pDynaModel)
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

void CDynaBranchMeasure::TopologyUpdated()
{
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
}

void CDynaBranchMeasure::SetBranch(CDynaBranch* pBranch)
{
	if (m_pBranch)
		throw dfw2error("CDynaBranchMeasure::SetBranch - branch already set");
	SetId(pBranch->GetId());
	SetName(pBranch->GetVerbalName());
	m_pBranch = pBranch;
	TopologyUpdated();
	pBranch->m_pMeasure = this;
}