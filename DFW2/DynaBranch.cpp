#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"


using namespace DFW2;

CDynaBranch::CDynaBranch() : CDevice(), m_pMeasure(nullptr)
{

}

// Обновляет имя ветви
void CDynaBranch::UpdateVerbalName()
{
	m_strVerbalName = Cex(_T("%d - %d%s %s"), Ip, Iq, static_cast<const _TCHAR*>(Np ? Cex(_T(" (%d)"), Np) : _T("")), GetName());
}

// Расчет проводимостей ветви для матрицы проводимостей
// bFixNegativeZ указаывает на необходимость корректировки
// отрицательных сопротивлений для устойчивого Зейделя
cplx CDynaBranch::GetYBranch(bool bFixNegativeZ)
{
	// если связь с нулевым сопротивлением
	// возвращаем проводимость равную нулю

	// если связь находится в суперузле (суперузлы начала и конца совпадают)
	// то возвращаем нулевое сопротивление, так как либо сопротивление этой
	// ветви меньше минимального, либо связь параллельна такой ветви

#ifdef _DEBUG
	bool bZ1 = m_pNodeSuperIp == m_pNodeSuperIq;
	bool bZ2 = IsZeroImpedance();
	// Ассертит случай, когда ветвь в суперузле но имеет большее чем пороговое сопротивление
	//_ASSERTE(bZ1 == bZ2);
	if(bZ2||bZ1)
		return cplx(0.0);
#else
	if(m_pNodeSuperIp == m_pNodeSuperIq)
		return cplx(0.0);
#endif 

	double Rf = R;
	double Xf = X;
	// Для ветвей с нулевым сопротивлением
	// задаем сопротивление в о.е. относительно напряжения
	double Xfictive = m_pNodeIp->Unom;
	Xfictive *= Xfictive;
	Xfictive *= 0.0000002;
	
	if (bFixNegativeZ)
	{
		if (R < 0)
			Rf = Xfictive;
		if (X < 0)
			Xf = Xfictive;
	}
	return 1.0 / cplx(Rf, Xf);
}


bool CDynaBranch::LinkToContainer(CDeviceContainer *pContainer, CDeviceContainer *pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom)
{
	bool bRes = false;
	if (m_pContainer)
	{
		bRes = true;

		DEVICEVECTORITR it;

		// идем по контейнеру ветвей
		for (it = m_pContainer->begin(); it != m_pContainer->end(); it++)
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);

			pBranch->m_pNodeIp = pBranch->m_pNodeIq = nullptr;

			// оба узла ветви обрабатываем в последовательных
			// итерациях цикла
			for (int i = 0; i < 2; i++)
			{
				CDynaNodeBase *pNode;
				ptrdiff_t NodeId = i ? pBranch->Ip : pBranch->Iq;

				// ищем узел по номеру
				if (pContainer->GetDevice(NodeId, pNode))
				{
					// если узел найден, резервируем в нем
					// связь для текущей ветви	
					pNode->IncrementLinkCounter(0);

					// жестко связываем ветвь и узлы
					if (i)
						pBranch->m_pNodeIp = pNode;
					else
						pBranch->m_pNodeIq = pNode;
				}
				else
				{
					pBranch->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszBranchNodeNotFound, NodeId));
					bRes = false;
				}
			}

			if (pBranch->m_pNodeIp && pBranch->m_pNodeIq)
			{
				// если оба узла найдены
				// формируем имя ветви по найденным узлам
				pBranch->SetName(Cex(_T("%s - %s %s"),
					pBranch->m_pNodeIp->GetName(),
					pBranch->m_pNodeIq->GetName(),
					static_cast<const _TCHAR*>(pBranch->Np ? Cex(_T(" öåïü %d"), pBranch->Np) : _T(""))));
			}
		}

		if (bRes)
		{
			// формируем буфер под ссылки в контейнере узлов
			pContainer->AllocateLinks(ptrdiff_t(0));
			// проходим по контейнеру ветвей
			for (it = m_pContainer->begin(); it != m_pContainer->end(); it++)
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);
				// и добавляем к узлам в контейнере связи с ветвями
				pContainer->AddLink(ptrdiff_t(0), pBranch->m_pNodeIp->m_nInContainerIndex, pBranch);
				pContainer->AddLink(ptrdiff_t(0), pBranch->m_pNodeIq->m_nInContainerIndex, pBranch);
			}
			// сбрасываем счетчики ссылок (заканчиваем связывание)
			pContainer->RestoreLinks(ptrdiff_t(0));
		}
	}
	return bRes;
}


// изменяет состояние ветви на заданное
eDEVICEFUNCTIONSTATUS CDynaBranch::SetBranchState(CDynaBranch::BranchState eBranchState, eDEVICESTATECAUSE eStateCause)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;
	if (eBranchState != m_BranchState)
	{
		// если заданное состояние ветви не 
		// совпадает с текущим
		m_BranchState = eBranchState;
		// информируем модель о необходимости 
		// обработки разрыва
		m_pNodeIp->ProcessTopologyRequest();
	}
	return Status;
}

// shortcut для установки состояния ветви в терминах CDevice
eDEVICEFUNCTIONSTATUS CDynaBranch::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause)
{
	BranchState eBranchState = BRANCH_OFF;
	if (eState == eDEVICESTATE::DS_ON)
		eBranchState = BRANCH_ON;
	return SetBranchState(eBranchState, eStateCause);
}

// shortcut для получения состояния ветви в терминах CDevice
eDEVICESTATE CDynaBranch::GetState() const
{
	eDEVICESTATE State = eDEVICESTATE::DS_ON;
	if (m_BranchState == BRANCH_OFF)
		State = DS_OFF;
	return State;
}

// рассчитывает проводимости ветви
// в зависимости от заданного состояния
void CDynaBranch::CalcAdmittances(bool bSeidell)
{
	// продольную проводимость рассчитываем 
	// в режиме корректировки отрицательных сопротивления
	// в зависимость от параметра (Зейдель или нет)
	
	// если ветвь имеет сопротивление ниже минимального, проводимость
	// этой ветви будет трактоваться как нулевая. Такие ветви нужно обрабатывать
	// в суперузлах. Проводимости в начале в конце (Yip/Yiq) и проводимости в начале в конце 
	// с учетом шунтов (Yips/Yiqs) при этом будут пригодны для расчета балансовых соотношений
	// в суперузле

	cplx Ybranch = GetYBranch(bSeidell);
	cplx Ktr(Ktr, Kti);

	switch (m_BranchState)
	{
	case CDynaBranch::BRANCH_OFF:
		// ветвь полностью отключена
		// все проводимости равны нулю
		Yip = Yiq = Yips = Yiqs = cplx(0.0, 0.0);
		break;
	case CDynaBranch::BRANCH_ON:
		// ветвь включена
		// проводимости рассчитываем по "П"-схеме
		Yip = Ybranch / Ktr;
		Yiq = Ybranch / conj(Ktr);
		Yips = cplx(GIp, BIp) + Ybranch;
		Yiqs = cplx(GIq, BIq) + Ybranch / norm(Ktr);
		break;
	case CDynaBranch::BRANCH_TRIPIP:
	{
		// ветвь отключена в начале		
		Yip = Yiq = Yips = 0.0;		// взаимные и собственная со стороны узла начала - 0
		Yiqs = cplx(GIq, BIq);

		cplx Yip1(GIp, BIp);
		cplx Ysum = Yip1 + Ybranch;

		_ASSERTE(!Equal(abs(Ysum), 0.0));

		// собстванная со стороны узла конца
		if (!Equal(abs(Ysum), 0.0))
			Yiqs += (Yip1 * Ybranch / Ysum) / norm(Ktr);
	}
	break;
	case CDynaBranch::BRANCH_TRIPIQ:
	{
		// отключена в конце
		Yiq = Yip = Yiqs = 0.0;		// взаимные и собственная со стороны узла конца - 0
		Yips = cplx(GIp, BIp);

		cplx Yip1(GIq, BIq);

		// собственная со стороны узла начала
		cplx Ysum = Yip1 / norm(Ktr) + Ybranch;

		_ASSERTE(!Equal(abs(Ysum), 0.0));

		if (!Equal(abs(Ysum), 0.0))
			Yips += Yip1 * Ybranch / Ysum;
	}
	break;
	}
}

bool CDynaBranch::IsZeroImpedance()
{
	const CDynaModel *pModel = GetModel();

	if (m_BranchState == CDynaBranch::BRANCH_ON && Equal(Ktr,1.0) && Equal(Kti,0.0))
	{
		double Zmin = pModel->GetZeroBranchImpedance();
		if (R < Zmin && X < Zmin)
			return true;
	}

	return false;
}


void CDynaBranch::UpdateSerializer(SerializerPtr& Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty(_T("ip"), Ip);
	Serializer->AddProperty(_T("iq"), Iq);
	Serializer->AddProperty(_T("np"), Np);
	Serializer->AddProperty(_T("r"), R, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(_T("x"), X, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(_T("ktr"), Ktr, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("kti"), Kti, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("g"), G, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty(_T("b"), B, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty(_T("gr_ip"), GrIp, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty(_T("gr_iq"), GrIq, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty(_T("br_ip"), BrIp, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty(_T("br_iq"), BrIq, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty(_T("nr_ip"), NrIp, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty(_T("nr_iq"), NrIq, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddEnumProperty(_T("sta"), new CSerializerAdapterEnumT<CDynaBranch::BranchState>(m_BranchState, m_cszBranchStateNames, _countof(m_cszBranchStateNames)));
	Serializer->AddState(_T("Gip"), GIp, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState(_T("Giq"), GIq, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState(_T("Bip"), BIp, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddState(_T("Biq"), BIq, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddState(_T("Yip"), Yip, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState(_T("Yiq"), Yiq, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState(_T("Yips"), Yips, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState(_T("Yiqs"), Yiqs, eVARUNITS::VARUNIT_SIEMENS);
}

CDynaNodeBase* CDynaBranch::GetOppositeNode(CDynaNodeBase* pOriginNode)
{
	_ASSERTE(pOriginNode == m_pNodeIq || pOriginNode == m_pNodeIp);
	return m_pNodeIp == pOriginNode ? m_pNodeIq : m_pNodeIp;
}

CDynaNodeBase* CDynaBranch::GetOppositeSuperNode(CDynaNodeBase* pOriginNode)
{
	_ASSERTE(pOriginNode == m_pNodeSuperIq || pOriginNode == m_pNodeSuperIp);
	return m_pNodeSuperIp == pOriginNode ? m_pNodeSuperIq : m_pNodeSuperIp;
}

CDynaBranchMeasure::CDynaBranchMeasure(CDynaBranch *pBranch) : CDevice(), m_pBranch(pBranch) 
{
	_ASSERTE(pBranch);
}

double* CDynaBranchMeasure::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Ibre, V_IBRE)
		MAP_VARIABLE(Ibim, V_IBIM)
		MAP_VARIABLE(Iere, V_IERE)
		MAP_VARIABLE(Ieim, V_IEIM)
		MAP_VARIABLE(Ib, V_IB)
		MAP_VARIABLE(Ie, V_IE)
		MAP_VARIABLE(Pb, V_PB)
		MAP_VARIABLE(Qb, V_QB)
		MAP_VARIABLE(Pe, V_PE)
		MAP_VARIABLE(Qe, V_QE)
		MAP_VARIABLE(Sb, V_SB)
		MAP_VARIABLE(Se, V_SE)
	}
	return p;
}


bool CDynaBranchMeasure::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;

	CDynaNodeBase *pNodeIp = m_pBranch->m_pNodeIp;
	CDynaNodeBase *pNodeIq = m_pBranch->m_pNodeIq;

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
	pDynaModel->SetElement(A(V_IBRE), A(V_IBRE), 1.0);
	// dIbre / dVq
	pDynaModel->SetElement(A(V_IBRE), pNodeIq->A(CDynaNodeBase::V_V), b * Sinq - g * Cosq);
	// dIbre / dDeltaQ
	pDynaModel->SetElement(A(V_IBRE), pNodeIq->A(CDynaNodeBase::V_DELTA), Vq * b * Cosq + Vq * g * Sinq);
	// dIbre / dVp
	pDynaModel->SetElement(A(V_IBRE), pNodeIp->A(CDynaNodeBase::V_V), gips * Cosp - bips * Sinp);
	// dIbre / dDeltaP
	pDynaModel->SetElement(A(V_IBRE), pNodeIp->A(CDynaNodeBase::V_DELTA), -Vp * bips * Cosp - Vp * gips * Sinp );


	// dIbim / dIbim
	pDynaModel->SetElement(A(V_IBIM), A(V_IBIM), 1.0);
	// dIbim / dVq
	pDynaModel->SetElement(A(V_IBIM), pNodeIq->A(CDynaNodeBase::V_V), -b * Cosq - g * Sinq);
	// dIbim / dDeltaQ
	pDynaModel->SetElement(A(V_IBIM), pNodeIq->A(CDynaNodeBase::V_DELTA), Vq * b * Sinq - Vq * g * Cosq);
	// dIbim / dVp
	pDynaModel->SetElement(A(V_IBIM), pNodeIp->A(CDynaNodeBase::V_V), bips * Cosp + gips * Sinp);
	// dIbim / dDeltaP
	pDynaModel->SetElement(A(V_IBIM), pNodeIp->A(CDynaNodeBase::V_DELTA), Vp * gips * Cosp - Vp * bips * Sinp);


	// dIere / dIere
	pDynaModel->SetElement(A(V_IERE), A(V_IERE), 1.0);
	// dIere / dVq
	pDynaModel->SetElement(A(V_IERE), pNodeIq->A(CDynaNodeBase::V_V), biqs * Sinq - giqs * Cosq);
	// dIere / dDeltaQ
	pDynaModel->SetElement(A(V_IERE), pNodeIq->A(CDynaNodeBase::V_DELTA), Vq * biqs * Cosq + Vq * giqs * Sinq);
	// dIere / dVp
	pDynaModel->SetElement(A(V_IERE), pNodeIp->A(CDynaNodeBase::V_V), ge * Cosp - be * Sinp);
	// dIere / dDeltaP
	pDynaModel->SetElement(A(V_IERE), pNodeIp->A(CDynaNodeBase::V_DELTA), -Vp * be * Cosp - Vp * ge * Sinp);


	// dIeim / dIeim
	pDynaModel->SetElement(A(V_IEIM), A(V_IEIM), 1.0);
	// dIeim / dVq
	pDynaModel->SetElement(A(V_IEIM), pNodeIq->A(CDynaNodeBase::V_V), -biqs * Cosq - giqs * Sinq);
	// dIeim / dDeltaQ
	pDynaModel->SetElement(A(V_IEIM), pNodeIq->A(CDynaNodeBase::V_DELTA), Vq * biqs * Sinq - Vq * giqs * Cosq);
	// dIeim / dVp
	pDynaModel->SetElement(A(V_IEIM), pNodeIp->A(CDynaNodeBase::V_V), be * Cosp + ge * Sinp);
	// dIeim / dDeltaP
	pDynaModel->SetElement(A(V_IEIM), pNodeIp->A(CDynaNodeBase::V_DELTA), Vp * ge * Cosp - Vp * be * Sinp);


	double absIb = sqrt(Ibre * Ibre + Ibim * Ibim);

	// dIb / dIb
	pDynaModel->SetElement(A(V_IB), A(V_IB), 1.0);
	// dIb / dIbre
	pDynaModel->SetElement(A(V_IB), A(V_IBRE), -CDevice::ZeroDivGuard(Ibre, absIb) );
	// dIb / dIbim
	pDynaModel->SetElement(A(V_IB), A(V_IBIM), -CDevice::ZeroDivGuard(Ibim, absIb) );

	absIb = sqrt(Iere * Iere + Ieim * Ieim);

	// dIe / dIe
	pDynaModel->SetElement(A(V_IE), A(V_IE), 1.0);
	// dIe / dIbre
	pDynaModel->SetElement(A(V_IE), A(V_IERE), -CDevice::ZeroDivGuard(Iere, absIb) );
	// dIe / dIbim
	pDynaModel->SetElement(A(V_IE), A(V_IEIM), -CDevice::ZeroDivGuard(Ieim, absIb) );


	// dPb / dPb
	pDynaModel->SetElement(A(V_PB), A(V_PB), 1.0);
	// dPb / dVp
	pDynaModel->SetElement(A(V_PB), pNodeIp->A(CDynaNodeBase::V_V), -Ibre * Cosp - Ibim * Sinp);
	// dPb / dDeltaP
	pDynaModel->SetElement(A(V_PB), pNodeIp->A(CDynaNodeBase::V_DELTA), Ibre * Vp * Sinp - Ibim * Vp * Cosp);
	// dPb / dIbre
	pDynaModel->SetElement(A(V_PB), A(V_IBRE), -Vp * Cosp);
	// dPb / dIbim
	pDynaModel->SetElement(A(V_PB), A(V_IBIM), -Vp * Sinp);


	// dQb / dQb
	pDynaModel->SetElement(A(V_QB), A(V_QB), 1.0);
	// dQb / dVp
	pDynaModel->SetElement(A(V_QB), pNodeIp->A(CDynaNodeBase::V_V), Ibim * Cosp - Ibre * Sinp);
	// dQb / dDeltaP
	pDynaModel->SetElement(A(V_QB), pNodeIp->A(CDynaNodeBase::V_DELTA), -Ibre * Vp * Cosp - Ibim * Vp * Sinp);
	// dQb / dIbre
	pDynaModel->SetElement(A(V_QB), A(V_IBRE), -Vp * Sinp);
	// dQb / dIbim
	pDynaModel->SetElement(A(V_QB), A(V_IBIM),  Vp * Cosp);


	// dPe / dPe
	pDynaModel->SetElement(A(V_PE), A(V_PE), 1.0);
	// dPe / dVq
	pDynaModel->SetElement(A(V_PE), pNodeIq->A(CDynaNodeBase::V_V), -Iere * Cosq - Ieim * Sinq);
	// dPe / dDeltaQ
	pDynaModel->SetElement(A(V_PE), pNodeIq->A(CDynaNodeBase::V_DELTA), Iere * Vq * Sinq - Ieim * Vq * Cosq);
	// dPe / dIere
	pDynaModel->SetElement(A(V_PE), A(V_IERE), -Vq * Cosq);
	// dPe / dIeim
	pDynaModel->SetElement(A(V_PE), A(V_IEIM), -Vq * Sinq);

	// dQe / dQe
	pDynaModel->SetElement(A(V_QE), A(V_QE), 1.0);
	// dQe / dVq
	pDynaModel->SetElement(A(V_QE), pNodeIq->A(CDynaNodeBase::V_V), Ieim * Cosq - Iere * Sinq);
	// dQe / dDeltaQ
	pDynaModel->SetElement(A(V_QE), pNodeIq->A(CDynaNodeBase::V_DELTA), -Iere * Vq * Cosq - Ieim * Vq * Sinq);
	// dQe / dIere
	pDynaModel->SetElement(A(V_QE), A(V_IERE), -Vq * Sinq);
	// dQe / dIeim
	pDynaModel->SetElement(A(V_QE), A(V_IEIM), Vq * Cosq);

	absIb = sqrt(Pb * Pb + Qb * Qb);

	// dSb / dSb
	pDynaModel->SetElement(A(V_SB), A(V_SB), 1.0);
	// dSb / dPb
	pDynaModel->SetElement(A(V_SB), A(V_PB), -CDevice::ZeroDivGuard(Pb, absIb) );
	// dSb / dQb
	pDynaModel->SetElement(A(V_SB), A(V_QB), -CDevice::ZeroDivGuard(Qb, absIb) );

	absIb = sqrt(Pe * Pe + Qe * Qe);

	if (absIb < DFW2_EPSILON)
		absIb = DFW2_EPSILON;

	// dSe / dSe
	pDynaModel->SetElement(A(V_SE), A(V_SE), 1.0);
	// dSe / dPe
	pDynaModel->SetElement(A(V_SE), A(V_PE), -CDevice::ZeroDivGuard(Pe, absIb) );
	// dSe / dQe
	pDynaModel->SetElement(A(V_SE), A(V_QE), -CDevice::ZeroDivGuard(Qe, absIb) );


	return true;
}


bool CDynaBranchMeasure::BuildRightHand(CDynaModel* pDynaModel)
{

	m_pBranch->m_pNodeIq->UpdateVreVim();
	m_pBranch->m_pNodeIp->UpdateVreVim();

	cplx& Ue = cplx(m_pBranch->m_pNodeIq->Vre, m_pBranch->m_pNodeIq->Vim);
	cplx& Ub = cplx(m_pBranch->m_pNodeIp->Vre, m_pBranch->m_pNodeIp->Vim);

	cplx cIb = -m_pBranch->Yips * Ub + m_pBranch->Yip  * Ue;
	cplx cIe = -m_pBranch->Yiq  * Ub + m_pBranch->Yiqs * Ue;
	cplx cSb = Ub * conj(cIb);
	cplx cSe = Ue * conj(cIe);


	pDynaModel->SetFunction(A(V_IBRE),	Ibre - cIb.real());
	pDynaModel->SetFunction(A(V_IBIM),	Ibim - cIb.imag());
	pDynaModel->SetFunction(A(V_IERE),	Iere - cIe.real());
	pDynaModel->SetFunction(A(V_IEIM),	Ieim - cIe.imag());
	pDynaModel->SetFunction(A(V_IB),	Ib - abs(cIb));
	pDynaModel->SetFunction(A(V_IE),	Ie - abs(cIe));
	pDynaModel->SetFunction(A(V_PB),	Pb - cSb.real());
	pDynaModel->SetFunction(A(V_QB),	Qb - cSb.imag());
	pDynaModel->SetFunction(A(V_PE),	Pe - cSe.real());
	pDynaModel->SetFunction(A(V_QE),	Qe - cSe.imag());
	pDynaModel->SetFunction(A(V_SB),	Sb - abs(cSb));
	pDynaModel->SetFunction(A(V_SE),	Se - abs(cSe));

	return true;
}

eDEVICEFUNCTIONSTATUS CDynaBranchMeasure::Init(CDynaModel* pDynaModel)
{
	return ProcessDiscontinuity(pDynaModel);
}

void CDynaBranchMeasure::CalculateFlows(const CDynaBranch* pBranch, cplx& cIb, cplx& cIe, cplx& cSb, cplx& cSe)
{
	// !!!!!!!!!!!!!   здесь рассчитываем на то, что для узлов начала и конца были сделаны UpdateVreVim !!!!!!!!!!!!!!

	cplx& Ue = cplx(pBranch->m_pNodeIq->Vre, pBranch->m_pNodeIq->Vim);
	cplx& Ub = cplx(pBranch->m_pNodeIp->Vre, pBranch->m_pNodeIp->Vim);
	cIb = -pBranch->Yips * Ub + pBranch->Yip  * Ue;
	cIe = -pBranch->Yiq  * Ub + pBranch->Yiqs * Ue;
	cSb = Ub * conj(cIb);
	cSe = Ue * conj(cIe);
}

eDEVICEFUNCTIONSTATUS CDynaBranchMeasure::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	m_pBranch->m_pNodeIq->UpdateVreVim();
	m_pBranch->m_pNodeIp->UpdateVreVim();
	cplx cIb, cIe, cSb, cSe;
	CDynaBranchMeasure::CalculateFlows(m_pBranch, cIb, cIe, cSb, cSe);
	Ibre = cIb.real();			Ibim = cIb.imag();
	Iere = cIe.real();			Ieim = cIe.imag();
	Ib = abs(cIb);				Ie = abs(cIe);
	Pb = cSb.real();			Qb = cSb.imag();
	Pe = cSe.real();			Qe = cSe.imag();
	Sb = abs(cSb);				Se = abs(cSe);
	return DFS_OK;
}



const CDeviceContainerProperties CDynaBranch::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.SetType(DEVTYPE_BRANCH);
	props.SetClassName(CDeviceContainerProperties::m_cszNameBranch, CDeviceContainerProperties::m_cszSysNameBranch);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, _T(""));
	props.m_lstAliases.push_back(CDeviceContainerProperties::m_cszAliasBranch);
	return props;
}

// описание переменных расчетных потоков по ветви
const CDeviceContainerProperties CDynaBranchMeasure::DeviceProperties()
{
	CDeviceContainerProperties props;
	// линковка делается только с ветвями, поэтому описание
	// правил связывания не нужно
	props.SetType(DEVTYPE_BRANCHMEASURE);
	props.SetClassName(CDeviceContainerProperties::m_cszNameBranchMeasure, CDeviceContainerProperties::m_cszSysNameBranchMeasure);
	props.nEquationsCount = CDynaBranchMeasure::VARS::V_LAST;
	props.m_VarMap.insert(make_pair(_T("Ibre"), CVarIndex(CDynaBranchMeasure::V_IBRE, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("Ibim"), CVarIndex(CDynaBranchMeasure::V_IBIM, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("Iere"), CVarIndex(CDynaBranchMeasure::V_IERE, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("Ieim"), CVarIndex(CDynaBranchMeasure::V_IEIM, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("Ib"),	CVarIndex(CDynaBranchMeasure::V_IB, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("Ie"),	CVarIndex(CDynaBranchMeasure::V_IE, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(make_pair(_T("Pb"),	CVarIndex(CDynaBranchMeasure::V_PB, VARUNIT_MW)));
	props.m_VarMap.insert(make_pair(_T("Qb"),	CVarIndex(CDynaBranchMeasure::V_QB, VARUNIT_MVAR)));
	props.m_VarMap.insert(make_pair(_T("Pe"),	CVarIndex(CDynaBranchMeasure::V_PE, VARUNIT_MW)));
	props.m_VarMap.insert(make_pair(_T("Qe"),	CVarIndex(CDynaBranchMeasure::V_QE, VARUNIT_MVAR)));
	props.m_VarMap.insert(make_pair(_T("Sb"),	CVarIndex(CDynaBranchMeasure::V_SB, VARUNIT_MVA)));
	props.m_VarMap.insert(make_pair(_T("Se"),	CVarIndex(CDynaBranchMeasure::V_SE, VARUNIT_MVA)));
	return props;
}

const _TCHAR* CDynaBranch::m_cszBranchStateNames[4] = { _T("On"), _T("Off"), _T("Htrip"), _T("Ttrip"), };