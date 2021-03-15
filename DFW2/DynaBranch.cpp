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
	m_strVerbalName = fmt::format("{} - {}{} {}", Ip, Iq, (Np ? fmt::format(" ({})", Np) : ""), GetName());
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
	// Для ветвей с малым или отрицательным сопротивлением
	// задаем сопротивление в о.е. относительно напряжения
	double Xfictive = m_pNodeIp->Unom;
	Xfictive *= Xfictive;
	Xfictive *= 0.000004;
	
	if (bFixNegativeZ)
	{
		// если нужно убрать отрицательные сопротивления - меняем отрицательные и нулевые на 
		// положительные
		if (R <= 0)
			Rf = 0.0;
		if (X <= 0)
			Xf = Xfictive;
	}
	else
	{
		// если нет - только заменяем нулевые на минимальные. Нулевые заменяются только для ветвей вне суперузлов (идеальные трансформаторы)
		if (R == 0.0 && X == 0.0)
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
					pBranch->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszBranchNodeNotFound, 
																		   NodeId, 
																		   pBranch->Ip, 
																		   pBranch->Iq, 
																		   pBranch->Np));
					bRes = false;
					_ASSERTE(bRes);
				}
			}

			if (pBranch->m_pNodeIp && pBranch->m_pNodeIq)
			{
				// если оба узла найдены
				// формируем имя ветви по найденным узлам
				pBranch->SetName(fmt::format("{} - {} {}",
					pBranch->m_pNodeIp->GetName(),
					pBranch->m_pNodeIq->GetName(),
					pBranch->Np ? fmt::format(" цепь {}", pBranch->Np) : ""));
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
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
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
eDEVICEFUNCTIONSTATUS CDynaBranch::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice)
{
	// при отключении ветви два режима
	BranchState eBranchState(m_BranchState);
	if (pCauseDevice && pCauseDevice->GetType() == DEVTYPE_NODE && eState == eDEVICESTATE::DS_OFF)
	{
		// если передан узел, из-за которого должно измениться состояние ветви
		// отключаем ветвь со стороны отключаемого узла или полностью, если оппозитный тоже отключен
		DisconnectBranchFromNode(static_cast<CDynaNodeBase*>(pCauseDevice));
		// если состояние ветви изменилось - добавляем оппозитный узел в список проверки,
		// который может понадобиться для каскадного отключения узлов
		if (eBranchState != m_BranchState)
		{
			GetOppositeNode(static_cast<CDynaNodeBase*>(pCauseDevice))->AddToTopologyCheck();
			eBranchState = m_BranchState;
		}
	}
	else
	{
		// если данных по узлу нет 
		if (eState == eDEVICESTATE::DS_ON)
			eBranchState = BRANCH_ON;
	}
	return SetBranchState(eBranchState, eStateCause);
}

// shortcut для получения состояния ветви в терминах CDevice
eDEVICESTATE CDynaBranch::GetState() const
{
	eDEVICESTATE State = eDEVICESTATE::DS_ON;
	if (m_BranchState == BRANCH_OFF)
		State = eDEVICESTATE::DS_OFF;
	return State;
}

// рассчитывает проводимости ветви
// в зависимости от заданного состояния
void CDynaBranch::CalcAdmittances(bool bFixNegativeZs)
{
	// продольную проводимость рассчитываем 
	// в режиме корректировки отрицательных сопротивления
	// в зависимость от параметра (Зейдель или нет)
	
	// если ветвь имеет сопротивление ниже минимального, проводимость
	// этой ветви будет трактоваться как нулевая. Такие ветви нужно обрабатывать
	// в суперузлах. Проводимости в начале в конце (Yip/Yiq) и проводимости в начале в конце 
	// с учетом шунтов (Yips/Yiqs) при этом будут пригодны для расчета балансовых соотношений
	// в суперузле

	cplx Ybranch = GetYBranch(bFixNegativeZs);
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

	_CheckNumber(Yip.real());
	_CheckNumber(Yip.imag());
	_CheckNumber(Yiq.real());
	_CheckNumber(Yiq.imag());
	_CheckNumber(Yips.real());
	_CheckNumber(Yips.imag());
	_CheckNumber(Yiqs.real());
	_CheckNumber(Yiqs.imag());
}

// функция возвращает true если сопротивление ветви меньше порогового для ветви с "нулевым сопротивлением".
// функция не контролирует, находится ли ветвь в параллели к ветви с нулевым сопротивлением
bool CDynaBranch::IsZeroImpedance()
{
	const CDynaModel *pModel = GetModel();

	if (m_BranchState == CDynaBranch::BRANCH_ON && Equal(Ktr,1.0) && Equal(Kti,0.0))
	{
		double Zmin = pModel->GetZeroBranchImpedance();
		if (std::abs(cplx(R,X)) / m_pNodeIp->Unom / m_pNodeIq->Unom < Zmin )
			return true;
	}

	return false;
}


void CDynaBranch::UpdateSerializer(SerializerPtr& Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty("ip", Ip);
	Serializer->AddProperty("iq", Iq);
	Serializer->AddProperty("np", Np);
	Serializer->AddProperty("r", R, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty("x", X, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty("ktr", Ktr, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty("kti", Kti, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty("g", G, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty("b", B, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty("gr_ip", GrIp, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty("gr_iq", GrIq, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty("br_ip", BrIp, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty("br_iq", BrIq, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty("nr_ip", NrIp, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty("nr_iq", NrIq, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddEnumProperty("sta", new CSerializerAdapterEnumT<CDynaBranch::BranchState>(m_BranchState, m_cszBranchStateNames, _countof(m_cszBranchStateNames)));
	Serializer->AddState("Gip", GIp, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("Giq", GIq, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("Bip", BIp, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddState("Biq", BIq, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddState("Yip", Yip, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("Yiq", Yiq, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("Yips", Yips, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("Yiqs", Yiqs, eVARUNITS::VARUNIT_SIEMENS);
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
	pDynaModel->SetElement(Ibre, Ibre, 1.0);
	// dIbre / dVq
	pDynaModel->SetElement(Ibre, pNodeIq->V, b * Sinq - g * Cosq);
	// dIbre / dDeltaQ
	pDynaModel->SetElement(Ibre, pNodeIq->Delta, Vq * b * Cosq + Vq * g * Sinq);
	// dIbre / dVp
	pDynaModel->SetElement(Ibre, pNodeIp->V, gips * Cosp - bips * Sinp);
	// dIbre / dDeltaP
	pDynaModel->SetElement(Ibre, pNodeIp->Delta, -Vp * bips * Cosp - Vp * gips * Sinp );


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
	pDynaModel->SetElement(Ib, Ibre, -CDevice::ZeroDivGuard(Ibre, absIb) );
	// dIb / dIbim
	pDynaModel->SetElement(Ib, Ibim, -CDevice::ZeroDivGuard(Ibim, absIb) );

	absIb = sqrt(Iere * Iere + Ieim * Ieim);

	// dIe / dIe
	pDynaModel->SetElement(Ie, Ie, 1.0);
	// dIe / dIbre
	pDynaModel->SetElement(Ie, Iere, -CDevice::ZeroDivGuard(Iere, absIb) );
	// dIe / dIbim
	pDynaModel->SetElement(Ie, Ieim, -CDevice::ZeroDivGuard(Ieim, absIb) );


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
	pDynaModel->SetElement(Qb, Ibim,  Vp * Cosp);


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
	pDynaModel->SetElement(Sb, Pb, -CDevice::ZeroDivGuard(Pb, absIb) );
	// dSb / dQb
	pDynaModel->SetElement(Sb, Qb, -CDevice::ZeroDivGuard(Qb, absIb) );

	absIb = sqrt(Pe * Pe + Qe * Qe);

	if (absIb < DFW2_EPSILON)
		absIb = DFW2_EPSILON;

	// dSe / dSe
	pDynaModel->SetElement(Se, Se, 1.0);
	// dSe / dPe
	pDynaModel->SetElement(Se, Pe, -CDevice::ZeroDivGuard(Pe, absIb) );
	// dSe / dQe
	pDynaModel->SetElement(Se, Qe, -CDevice::ZeroDivGuard(Qe, absIb) );


	return true;
}


bool CDynaBranchMeasure::BuildRightHand(CDynaModel* pDynaModel)
{

	m_pBranch->m_pNodeIq->UpdateVreVim();
	m_pBranch->m_pNodeIp->UpdateVreVim();

	const cplx& Ue = cplx(m_pBranch->m_pNodeIq->Vre, m_pBranch->m_pNodeIq->Vim);
	const cplx& Ub = cplx(m_pBranch->m_pNodeIp->Vre, m_pBranch->m_pNodeIp->Vim);

	cplx cIb = -m_pBranch->Yips * Ub + m_pBranch->Yip  * Ue;
	cplx cIe = -m_pBranch->Yiq  * Ub + m_pBranch->Yiqs * Ue;
	cplx cSb = Ub * conj(cIb);
	cplx cSe = Ue * conj(cIe);


	pDynaModel->SetFunction(Ibre,	Ibre - cIb.real());
	pDynaModel->SetFunction(Ibim,	Ibim - cIb.imag());
	pDynaModel->SetFunction(Iere,	Iere - cIe.real());
	pDynaModel->SetFunction(Ieim,	Ieim - cIe.imag());
	pDynaModel->SetFunction(Ib,		Ib - abs(cIb));
	pDynaModel->SetFunction(Ie,		Ie - abs(cIe));
	pDynaModel->SetFunction(Pb,		Pb - cSb.real());
	pDynaModel->SetFunction(Qb,		Qb - cSb.imag());
	pDynaModel->SetFunction(Pe,		Pe - cSe.real());
	pDynaModel->SetFunction(Qe,		Qe - cSe.imag());
	pDynaModel->SetFunction(Sb,		Sb - abs(cSb));
	pDynaModel->SetFunction(Se,		Se - abs(cSe));

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
		Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszSwitchedOffBranch, 
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
	cIb = -pBranch->Yips * Ub + pBranch->Yip  * Ue;
	cIe = -pBranch->Yiq  * Ub + pBranch->Yiqs * Ue;
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
	Ibre = cIb.real();			Ibim = cIb.imag();
	Iere = cIe.real();			Ieim = cIe.imag();
	Ib = abs(cIb);				Ie = abs(cIe);
	Pb = cSb.real();			Qb = cSb.imag();
	Pe = cSe.real();			Qe = cSe.imag();
	Sb = abs(cSb);				Se = abs(cSe);
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}



const CDeviceContainerProperties CDynaBranch::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.SetType(DEVTYPE_BRANCH);
	props.SetClassName(CDeviceContainerProperties::m_cszNameBranch, CDeviceContainerProperties::m_cszSysNameBranch);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, "");
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
	props.m_VarMap.insert(std::make_pair("Ibre", CVarIndex(CDynaBranchMeasure::V_IBRE, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Ibim", CVarIndex(CDynaBranchMeasure::V_IBIM, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Iere", CVarIndex(CDynaBranchMeasure::V_IERE, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Ieim", CVarIndex(CDynaBranchMeasure::V_IEIM, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Ib",	CVarIndex(CDynaBranchMeasure::V_IB, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Ie",	CVarIndex(CDynaBranchMeasure::V_IE, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Pb",	CVarIndex(CDynaBranchMeasure::V_PB, VARUNIT_MW)));
	props.m_VarMap.insert(std::make_pair("Qb",	CVarIndex(CDynaBranchMeasure::V_QB, VARUNIT_MVAR)));
	props.m_VarMap.insert(std::make_pair("Pe",	CVarIndex(CDynaBranchMeasure::V_PE, VARUNIT_MW)));
	props.m_VarMap.insert(std::make_pair("Qe",	CVarIndex(CDynaBranchMeasure::V_QE, VARUNIT_MVAR)));
	props.m_VarMap.insert(std::make_pair("Sb",	CVarIndex(CDynaBranchMeasure::V_SB, VARUNIT_MVA)));
	props.m_VarMap.insert(std::make_pair("Se",	CVarIndex(CDynaBranchMeasure::V_SE, VARUNIT_MVA)));
	return props;
}

const char* CDynaBranch::m_cszBranchStateNames[4] = { "On", "Off", "Htrip", "Ttrip", };