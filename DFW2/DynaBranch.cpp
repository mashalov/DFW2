#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"


using namespace DFW2;

// Обновляет имя ветви
void CDynaBranch::UpdateVerbalName()
{
	m_strVerbalName = fmt::format("{} - {}{} [{}]", key.Ip, key.Iq, (key.Np ? fmt::format(" ({})", key.Np) : ""), GetName());
	if (m_pContainer)
		m_strVerbalName = fmt::format("{} {}", m_pContainer->GetTypeName(), m_strVerbalName);
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
		if (X < 0 || (X <= 0 && R == 0.0))
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

		// идем по контейнеру ветвей
		for (auto&& it : *m_pContainer)
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(it);
			pBranch->UpdateVerbalName();

			pBranch->m_pNodeIp = pBranch->m_pNodeIq = nullptr;

			// оба узла ветви обрабатываем в последовательных
			// итерациях цикла
			for (int i = 0; i < 2; i++)
			{
				CDynaNodeBase *pNode;
				ptrdiff_t NodeId = i ? pBranch->key.Ip : pBranch->key.Iq;

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
					pBranch->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszBranchNodeNotFound,
																		   NodeId, 
																		   pBranch->key.Ip, 
																		   pBranch->key.Iq,
																		   pBranch->key.Np));
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
					pBranch->key.Np ? fmt::format(" цепь {}", pBranch->key.Np) : ""));
			}
		}

		if (bRes)
		{
			// формируем буфер под ссылки в контейнере узлов
			pContainer->AllocateLinks(ptrdiff_t(0));
			// проходим по контейнеру ветвей
			for (auto&& it : *m_pContainer)
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(it);
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
			eBranchState = BranchState::BRANCH_ON;
	}
	return SetBranchState(eBranchState, eStateCause);
}

// shortcut для получения состояния ветви в терминах CDevice
eDEVICESTATE CDynaBranch::GetState() const
{
	eDEVICESTATE State = eDEVICESTATE::DS_ON;
	if (m_BranchState == BranchState::BRANCH_OFF)
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
	cplx Ktrx(Ktr, Kti);

	// постоянная часть проводимостей на землю от собственных шунтов ветви и старых реакторов не изменяется 
	// рассчитана заранее
	GIp = GIp0;		BIp = BIp0;		GIq = GIq0;		BIq = BIq0;

	// текущую проводимость на землю рассчитываем как сумму постоянной и включенных реакторов
	// в начале и в конце
	for(const auto& reactor : reactorsHead)
		if (reactor->IsStateOn())
		{
			GIp += reactor->g;
			BIp += reactor->b;
		}

	for (const auto& reactor : reactorsTail)
		if (reactor->IsStateOn())
		{
			GIq += reactor->g;
			BIq += reactor->b;
		}

	switch (m_BranchState)
	{
	case CDynaBranch::BranchState::BRANCH_OFF:
		// ветвь полностью отключена
		// все проводимости равны нулю
		Yip = Yiq = Yips = Yiqs = cplx(0.0, 0.0);
		break;
	case CDynaBranch::BranchState::BRANCH_ON:
		// ветвь включена
		// проводимости рассчитываем по "П"-схеме
		Yip = Ybranch / Ktrx;
		Yiq = Ybranch / conj(Ktrx);
		Yips = cplx(GIp, BIp) + Ybranch;
		Yiqs = cplx(GIq, BIq) + Ybranch / norm(Ktrx);
		break;
	case CDynaBranch::BranchState::BRANCH_TRIPIP:
	{
		// ветвь отключена в начале		
		Yip = Yiq = Yips = 0.0;		// взаимные и собственная со стороны узла начала - 0
		Yiqs = cplx(GIq, BIq);

		cplx Yip1(GIp, BIp);
		cplx Ysum = Yip1 + Ybranch;

		_ASSERTE(!Equal(abs(Ysum), 0.0));

		// собстванная со стороны узла конца
		if (!Equal(abs(Ysum), 0.0))
			Yiqs += (Yip1 * Ybranch / Ysum) / norm(Ktrx);
	}
	break;
	case CDynaBranch::BranchState::BRANCH_TRIPIQ:
	{
		// отключена в конце
		Yiq = Yip = Yiqs = 0.0;		// взаимные и собственная со стороны узла конца - 0
		Yips = cplx(GIp, BIp);

		cplx Yip1(GIq, BIq);

		// собственная со стороны узла начала
		cplx Ysum = Yip1 / norm(Ktrx) + Ybranch;

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

	if (m_BranchState == CDynaBranch::BranchState::BRANCH_ON && Equal(Ktr,1.0) && Equal(Kti,0.0))
	{
		double Zmin = pModel->GetZeroBranchImpedance();
		if (std::abs(R) / m_pNodeIp->Unom / m_pNodeIq->Unom < Zmin &&
			std::abs(X) / m_pNodeIp->Unom / m_pNodeIq->Unom < Zmin)
			return true;
	}

	return false;
}


void CDynaBranch::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty("ip", key.Ip);
	Serializer->AddProperty("iq", key.Iq);
	Serializer->AddProperty("np", key.Np);
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
	Serializer->AddEnumProperty("sta", new CSerializerAdapterEnum(m_BranchState, m_cszBranchStateNames));
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

void CDynaBranch::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_BRANCH);
	props.SetClassName(CDeviceContainerProperties::m_cszNameBranch, CDeviceContainerProperties::m_cszSysNameBranch);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, "");
	props.m_lstAliases.push_back(CDeviceContainerProperties::m_cszAliasBranch);

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaBranch>>();
}

void CDynaBranchContainer::IndexBranchIds()
{
	BranchKeyMap.clear();
	for (auto&& branch : m_DevVec)
	{
		CDynaBranch* pBranch = static_cast<CDynaBranch*>(branch);
		BranchKeyMap.insert(pBranch);
	}
}

CDynaBranch* CDynaBranchContainer::GetByKey(const CDynaBranch::Key& key)
{
	CDynaBranch search;
	search.key = key;
	const auto& branch = BranchKeyMap.find(&search);
	if (branch != BranchKeyMap.end())
		return *branch;
	return nullptr;
}

void CDynaBranchContainer::LinkToReactors(CDeviceContainer& containerReactors)
{
	for (const auto& reactor : containerReactors)
	{
		const CDynaReactor* pReactor = static_cast<const CDynaReactor*>(reactor);

		// отбираем линейные реакторы, которые подключены после выключателя
		if ((pReactor->Type == 1 || pReactor->Type == 2) && pReactor->Placement == 1)
		{
			// ищем заданную в реакторе ветвь (поиск работает в обоих направлениях)
			CDynaBranch* pBranch = GetByKey({ pReactor->HeadNode, pReactor->TailNode, pReactor->ParrBranch });
			if (pBranch)
			{
				// если ветвь найдена, проверяем, мы ее нашли в прямом или в обратном направлении
				if (pBranch->key.Ip == pReactor->HeadNode)
				{
					// нашли в прямом, реакторы в начале подключаем к началу, реакторы в конце - к концу
					if (pReactor->Type == 1)
						pBranch->reactorsHead.push_back(pReactor);
					else
						pBranch->reactorsTail.push_back(pReactor);
				}
				else
				{
					// нашли в обратном направлении, реакторы в начале подключаем к концу, реакторы в конце - к началу
					if (pReactor->Type == 1)
						pBranch->reactorsTail.push_back(pReactor);
					else
						pBranch->reactorsHead.push_back(pReactor);
				}
			}
			else
			{
				Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszBranchNotFoundForReactor,
					pReactor->HeadNode,
					pReactor->TailNode,
					pReactor->ParrBranch,
					reactor->GetVerbalName()));
			}
		}
	}
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


const char* CDynaBranch::m_cszBranchStateNames[4] = { "On", "Off", "Htrip", "Ttrip", };
