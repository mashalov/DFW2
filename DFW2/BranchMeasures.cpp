#include "stdafx.h"
#include "BranchMeasures.h"
#include "DynaModel.h"

using namespace DFW2;

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

	CDynaNodeBase* pNodeIp = m_pBranch->m_pNodeIp;
	CDynaNodeBase* pNodeIq = m_pBranch->m_pNodeIq;

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

	absIb = sqrt(Pb * Pb + Qb * Qb);

	// dSb / dSb
	pDynaModel->SetElement(Sb, Sb, 1.0);
	// dSb / dPb
	pDynaModel->SetElement(Sb, Pb, -CDevice::ZeroDivGuard(Pb, absIb));
	// dSb / dQb
	pDynaModel->SetElement(Sb, Qb, -CDevice::ZeroDivGuard(Qb, absIb));

	absIb = sqrt(Pe * Pe + Qe * Qe);

	if (absIb < DFW2_EPSILON)
		absIb = DFW2_EPSILON;

	// dSe / dSe
	pDynaModel->SetElement(Se, Se, 1.0);
	// dSe / dPe
	pDynaModel->SetElement(Se, Pe, -CDevice::ZeroDivGuard(Pe, absIb));
	// dSe / dQe
	pDynaModel->SetElement(Se, Qe, -CDevice::ZeroDivGuard(Qe, absIb));


	return true;
}


bool CDynaBranchMeasure::BuildRightHand(CDynaModel* pDynaModel)
{

	m_pBranch->m_pNodeIq->UpdateVreVim();
	m_pBranch->m_pNodeIp->UpdateVreVim();

	const cplx& Ue = cplx(m_pBranch->m_pNodeIq->Vre, m_pBranch->m_pNodeIq->Vim);
	const cplx& Ub = cplx(m_pBranch->m_pNodeIp->Vre, m_pBranch->m_pNodeIp->Vim);

	cplx cIb = -m_pBranch->Yips * Ub + m_pBranch->Yip * Ue;
	cplx cIe = -m_pBranch->Yiq * Ub + m_pBranch->Yiqs * Ue;
	cplx cSb = Ub * conj(cIb);
	cplx cSe = Ue * conj(cIe);


	pDynaModel->SetFunction(Ibre, Ibre - cIb.real());
	pDynaModel->SetFunction(Ibim, Ibim - cIb.imag());
	pDynaModel->SetFunction(Iere, Iere - cIe.real());
	pDynaModel->SetFunction(Ieim, Ieim - cIe.imag());
	pDynaModel->SetFunction(Ib, Ib - abs(cIb));
	pDynaModel->SetFunction(Ie, Ie - abs(cIe));
	pDynaModel->SetFunction(Pb, Pb - cSb.real());
	pDynaModel->SetFunction(Qb, Qb - cSb.imag());
	pDynaModel->SetFunction(Pe, Pe - cSe.real());
	pDynaModel->SetFunction(Qe, Qe - cSe.imag());
	pDynaModel->SetFunction(Sb, Sb - abs(cSb));
	pDynaModel->SetFunction(Se, Se - abs(cSe));

	return true;
}


bool CDynaBranch::BranchAndNodeConnected(CDynaNodeBase* pNode)
{

	if (pNode == m_pNodeIp)
	{
		return m_BranchState == CDynaBranch::BranchState::BRANCH_ON || m_BranchState == CDynaBranch::BranchState::BRANCH_TRIPIQ;
	}
	else if (pNode == m_pNodeIq)
	{
		return m_BranchState == CDynaBranch::BranchState::BRANCH_ON || m_BranchState == CDynaBranch::BranchState::BRANCH_TRIPIP;
	}
	else
		_ASSERTE(!"CDynaNodeContainer::BranchAndNodeConnected - branch and node mismatch");

	return false;
}

bool CDynaBranch::DisconnectBranchFromNode(CDynaNodeBase* pNode)
{
	bool bDisconnected(false);
	_ASSERTE(pNode->GetState() == eDEVICESTATE::DS_OFF);

	const char* pSwitchOffMode(CDFW2Messages::m_cszSwitchedOffBranchComplete);

	if (pNode == m_pNodeIp)
	{
		switch (m_BranchState)
		{
			// если ветвь включена - отключаем ее со стороны отключенного узла
		case CDynaBranch::BranchState::BRANCH_ON:

			pSwitchOffMode = CDFW2Messages::m_cszSwitchedOffBranchHead;
			m_BranchState = CDynaBranch::BranchState::BRANCH_TRIPIP;
			bDisconnected = true;
			break;
			// если ветвь отключена с обратной стороны - отключаем полностью
		case CDynaBranch::BranchState::BRANCH_TRIPIQ:
			m_BranchState = CDynaBranch::BranchState::BRANCH_OFF;
			bDisconnected = true;
			break;
		}
	}
	else if (pNode == m_pNodeIq)
	{
		switch (m_BranchState)
		{
			// если ветвь включена - отключаем ее со стороны отключенного узла
		case CDynaBranch::BranchState::BRANCH_ON:
			pSwitchOffMode = CDFW2Messages::m_cszSwitchedOffBranchTail;
			m_BranchState = CDynaBranch::BranchState::BRANCH_TRIPIQ;
			bDisconnected = true;
			break;
			// если ветвь отключена с обратной стороны - отключаем полностью
		case CDynaBranch::BranchState::BRANCH_TRIPIP:
			m_BranchState = CDynaBranch::BranchState::BRANCH_OFF;
			bDisconnected = true;
			break;
		}
	}
	else
		_ASSERTE(!"CDynaNodeContainer::DisconnectBranchFromNode - branch and node mismatch");

	if (bDisconnected)
	{
		Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszSwitchedOffBranch,
			m_pContainer->GetTypeName(),
			GetVerbalName(),
			pSwitchOffMode,
			pNode->GetVerbalName()));
	}

	return bDisconnected;
}


eDEVICEFUNCTIONSTATUS CDynaBranchMeasure::Init(CDynaModel* pDynaModel)
{
	return ProcessDiscontinuity(pDynaModel);
}

void CDynaBranchMeasure::CalculateFlows(const CDynaBranch* pBranch, cplx& cIb, cplx& cIe, cplx& cSb, cplx& cSe)
{
	// !!!!!!!!!!!!!   здесь рассчитываем на то, что для узлов начала и конца были сделаны UpdateVreVim !!!!!!!!!!!!!!

	const cplx& Ue = cplx(pBranch->m_pNodeIq->Vre, pBranch->m_pNodeIq->Vim);
	const cplx& Ub = cplx(pBranch->m_pNodeIp->Vre, pBranch->m_pNodeIp->Vim);
	cIb = -pBranch->Yips * Ub + pBranch->Yip * Ue;
	cIe = -pBranch->Yiq * Ub + pBranch->Yiqs * Ue;
	cSb = Ub * conj(cIb);
	cSe = Ue * conj(cIe);
}

VariableIndexRefVec& CDynaBranchMeasure::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Ibre, Ibim, Iere, Ieim, Ib, Ie, Pb, Qb, Pe, Qe, Sb, Se }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CDynaBranchMeasure::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	m_pBranch->m_pNodeIq->UpdateVreVim();
	m_pBranch->m_pNodeIp->UpdateVreVim();
	cplx cIb, cIe, cSb, cSe;
	CDynaBranchMeasure::CalculateFlows(m_pBranch, cIb, cIe, cSb, cSe);

	FromComplex(Ibre, Ibim, cIb);
	FromComplex(Iere, Ieim, cIe);
	FromComplex(Pb, Qb, cSb);
	FromComplex(Pe, Qe, cSe);
	Sb = abs(cSb);				Se = abs(cSe);
	Ib = abs(cIb);				Ie = abs(cIe);
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
	//props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaBranchMeasure>>();
}
