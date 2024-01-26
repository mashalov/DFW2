#include "stdafx.h"
#include "DynaBranch.h"
#include "BranchMeasures.h"
#include "DynaModel.h"


using namespace DFW2;

// Обновляет имя ветви
void CDynaBranch::UpdateVerbalName()
{
	VerbalName_ = fmt::format("{} - {}{} [{}]", key.Ip, key.Iq, (key.Np ? fmt::format(" ({})", key.Np) : ""), GetName());
	if (pContainer_)
		VerbalName_ = fmt::format("{} {}", pContainer_->GetTypeName(), VerbalName_);
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
	bool bZ1{ pNodeSuperIp_ == pNodeSuperIq_ };
	bool bZ2{ IsZeroImpedance() };
	// Ассертит случай, когда ветвь в суперузле но имеет большее чем пороговое сопротивление
	//_ASSERTE(bZ1 == bZ2);
	if(bZ2||bZ1)
		return cplx(0.0);
#else
	if(InSuperNode())
		return cplx(0.0);
#endif 

	double Rf{ R }, Xf{ X };
	// Для ветвей с малым или отрицательным сопротивлением
	// задаем сопротивление в о.е. относительно напряжения
	const double Xfictive{ pNodeIp_->Unom * 110.0 * 0.000004 };

#define ZMIN_RASTRWIN

#ifdef ZMIN_RASTRWIN
	// В RastrWin минимальное сопротивление вводится только для R = 0.0, X = 0.0
	// но при этом сопротивление может оказаться меньше минимального
	if (Rf == 0.0 && Xf == 0.0)
		Xf = Xfictive;
#else
	// Другой вариант - контролировать минимальное сопротивление составляющих
	// и не допускать сопротивление ниже чем заданное
	if (std::abs(Rf) < Xfictive)
	{
		// если оно положительное и не превышает минимальное, ставим минимальное
		if (Xf >= 0 && Xf < Xfictive)
			Xf = Xfictive;
		// если отрицательное не меньше минимального по модулю, ставим отрицательное минимальнео
		else if (Xf <= 0 && Xf > -Xfictive)
			Xf = -Xfictive;
	}
#endif

	cplx y { 1.0 / cplx(Rf, Xf) };
	
	// для Зейделя отрицательные реактивные проводимости заменяем на минимальне положительные
	if (bFixNegativeZ)
	{
		if (y.imag() > 0.0)
			y.imag(-2.0 / Xfictive);
	}
	
	return y;
}


bool CDynaBranch::LinkToContainer(CDeviceContainer *pContainer, CDeviceContainer *pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom)
{
	bool bRes{ false };
	if (pContainer_)
	{
		bRes = true;

		// идем по контейнеру ветвей
		for (auto&& it : *pContainer_)
		{
			CDynaBranch* pBranch{ static_cast<CDynaBranch*>(it) };
			pBranch->UpdateVerbalName();

			pBranch->pNodeIp_ = pBranch->pNodeIq_ = nullptr;

			// оба узла ветви обрабатываем в последовательных
			// итерациях цикла
			for (int i = 0; i < 2; i++)
			{
				CDynaNodeBase *pNode;
				const ptrdiff_t NodeId{ i ? pBranch->key.Ip : pBranch->key.Iq };

				// ищем узел по номеру
				if (pContainer->GetDevice(NodeId, pNode))
				{
					// если узел найден, резервируем в нем
					// связь для текущей ветви	
					pNode->IncrementLinkCounter(0);

					// жестко связываем ветвь и узлы
					if (i)
						pBranch->pNodeIp_ = pNode;
					else
						pBranch->pNodeIq_ = pNode;
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

			if (pBranch->pNodeIp_ && pBranch->pNodeIq_)
			{
				// если оба узла найдены
				// формируем имя ветви по найденным узлам
				pBranch->SetName(fmt::format("{} - {} {}",
					pBranch->pNodeIp_->GetName(),
					pBranch->pNodeIq_->GetName(),
					pBranch->key.Np ? fmt::format(" цепь {}", pBranch->key.Np) : ""));
			}
		}

		if (bRes)
		{
			// формируем буфер под ссылки в контейнере узлов
			pContainer->AllocateLinks(ptrdiff_t(0));
			// проходим по контейнеру ветвей
			for (auto&& it : *pContainer_)
			{
				const auto& pBranch{ static_cast<CDynaBranch*>(it) };
				// и добавляем к узлам в контейнере связи с ветвями
				pContainer->AddLink(ptrdiff_t(0), pBranch->pNodeIp_->InContainerIndex(), pBranch);
				pContainer->AddLink(ptrdiff_t(0), pBranch->pNodeIq_->InContainerIndex(), pBranch);
			}
		}
	}
	return bRes;
}


// изменяет состояние ветви на заданное
eDEVICEFUNCTIONSTATUS CDynaBranch::SetBranchState(CDynaBranch::BranchState eBranchState, eDEVICESTATECAUSE eStateCause)
{
	eDEVICEFUNCTIONSTATUS Status{ eDEVICEFUNCTIONSTATUS::DFS_OK };
	if (eBranchState != BranchState_)
	{
		// если заданное состояние ветви не 
		// совпадает с текущим
		BranchState_ = eBranchState;
		// синхронизируем фиктивную переменную состояния
		// по маппингу состояния ветви в обычное состояние устройства
		StateVar_ = (GetState() == eDEVICESTATE::DS_OFF) ? 0.0 : 1.0;
		// информируем модель о необходимости 
		// обработки разрыва
		pNodeIp_->ProcessTopologyRequest();
	}
	return Status;
}

// shortcut для установки состояния ветви в терминах CDevice
eDEVICEFUNCTIONSTATUS CDynaBranch::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice)
{
	// при отключении ветви два режима
	BranchState eBranchState{ BranchState_ };

	if (pCauseDevice && pCauseDevice->GetType() == DEVTYPE_NODE && eState == eDEVICESTATE::DS_OFF)
	{
		// если передан узел, из-за которого должно измениться состояние ветви
		// отключаем ветвь со стороны отключаемого узла или полностью, если оппозитный тоже отключен
		DisconnectBranchFromNode(static_cast<CDynaNodeBase*>(pCauseDevice));
		// если состояние ветви изменилось - добавляем оппозитный узел в список проверки,
		// который может понадобиться для каскадного отключения узлов
		if (eBranchState != BranchState_)
		{
			GetOppositeNode(static_cast<CDynaNodeBase*>(pCauseDevice))->AddToTopologyCheck();
			eBranchState = BranchState_;
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
	eDEVICESTATE State{ eDEVICESTATE::DS_ON };
	if (BranchState_ == BranchState::BRANCH_OFF)
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

	const cplx Ybranch{ GetYBranch(bFixNegativeZs) }, Ktrx(Ktr, Kti);

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

	switch (BranchState_)
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

		const cplx Yip1(GIp, BIp), Ysum{ Yip1 + Ybranch };

		_ASSERTE(!Consts::Equal(std::abs(Ysum), 0.0));

		// собстванная со стороны узла конца
		if (!Consts::Equal(std::abs(Ysum), 0.0))
			Yiqs += (Yip1 * Ybranch / Ysum) / norm(Ktrx);
	}
	break;
	case CDynaBranch::BranchState::BRANCH_TRIPIQ:
	{
		// отключена в конце
		Yiq = Yip = Yiqs = 0.0;		// взаимные и собственная со стороны узла конца - 0
		Yips = cplx(GIp, BIp);

		const cplx Yip1(GIq, BIq);

		// собственная со стороны узла начала
		const cplx Ysum{ Yip1 / norm(Ktrx) + Ybranch };

		_ASSERTE(!Consts::Equal(abs(Ysum), 0.0));

		if (!Consts::Equal(abs(Ysum), 0.0))
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
	const CDynaModel* pModel{ GetModel() };

	if (BranchState_ == CDynaBranch::BranchState::BRANCH_ON && Consts::Equal(Ktr,1.0) && Consts::Equal(Kti,0.0))
	{
		const  double Zmin{ pModel->GetZeroBranchImpedance() };
		if (std::abs(R) / pNodeIp_->Unom / pNodeIq_->Unom < Zmin &&
			std::abs(X) / pNodeIp_->Unom / pNodeIq_->Unom < Zmin)
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
	Serializer->AddProperty(CDynaNodeBase::cszr_, R, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaNodeBase::cszx_, X, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty("ktr", Ktr, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty("kti", Kti, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(CDynaNodeBase::cszg_, G, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty(CDynaNodeBase::cszb_, B, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty("gr_ip", GrIp, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty("gr_iq", GrIq, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty("br_ip", BrIp, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty("br_iq", BrIq, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty("nr_ip", NrIp, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty("nr_iq", NrIq, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddEnumProperty(CDevice::cszSta_, new CSerializerAdapterEnum<CDynaBranch::BranchState>(BranchState_, cszBranchStateNames_));
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
	_ASSERTE(pOriginNode == pNodeIq_ || pOriginNode == pNodeIp_);
	return pNodeIp_ == pOriginNode ? pNodeIq_ : pNodeIp_;
}

CDynaNodeBase* CDynaBranch::GetOppositeSuperNode(CDynaNodeBase* pOriginNode)
{
	_ASSERTE(pOriginNode == pNodeSuperIq_ || pOriginNode == pNodeSuperIp_);
	return pNodeSuperIp_ == pOriginNode ? pNodeSuperIq_ : pNodeSuperIp_;
}

void CDynaBranch::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_BRANCH);
	props.SetClassName(CDeviceContainerProperties::cszNameBranch_, CDeviceContainerProperties::cszSysNameBranch_);
	props.AddLinkTo(DEVTYPE_NODE, DLM_SINGLE, DPD_MASTER, "");
	props.Aliases_.push_back(CDynaBranch::cszAliasBranch_);
	props.bStoreStates = false;
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaBranch>>();
}

void CDynaBranchContainer::IndexBranchIds()
{
	BranchKeyMap.clear();
	for (auto&& branch : DevVec)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(branch) };
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
		const auto& pReactor{ static_cast<const CDynaReactor*>(reactor) };

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

	if (pNode == pNodeIp_)
	{
		return BranchState_ == CDynaBranch::BranchState::BRANCH_ON || BranchState_ == CDynaBranch::BranchState::BRANCH_TRIPIQ;
	}
	else if (pNode == pNodeIq_)
	{
		return BranchState_ == CDynaBranch::BranchState::BRANCH_ON || BranchState_ == CDynaBranch::BranchState::BRANCH_TRIPIP;
	}
	else
		_ASSERTE(!"CDynaNodeContainer::BranchAndNodeConnected - branch and node mismatch");

	return false;
}

bool CDynaBranch::DisconnectBranchFromNode(CDynaNodeBase* pNode)
{
	bool bDisconnected{ false };
	_ASSERTE(pNode->GetState() == eDEVICESTATE::DS_OFF);

	const char* pSwitchOffMode{ CDFW2Messages::m_cszSwitchedOffBranchComplete };

	if (pNode == pNodeIp_)
	{
		switch (BranchState_)
		{
			// если ветвь включена - отключаем ее со стороны отключенного узла
		case CDynaBranch::BranchState::BRANCH_ON:

			pSwitchOffMode = CDFW2Messages::m_cszSwitchedOffBranchHead;
			BranchState_ = CDynaBranch::BranchState::BRANCH_TRIPIP;
			bDisconnected = true;
			break;
			// если ветвь отключена с обратной стороны - отключаем полностью
		case CDynaBranch::BranchState::BRANCH_TRIPIQ:
			BranchState_ = CDynaBranch::BranchState::BRANCH_OFF;
			bDisconnected = true;
			break;
		}
	}
	else if (pNode == pNodeIq_)
	{
		switch (BranchState_)
		{
			// если ветвь включена - отключаем ее со стороны отключенного узла
		case CDynaBranch::BranchState::BRANCH_ON:
			pSwitchOffMode = CDFW2Messages::m_cszSwitchedOffBranchTail;
			BranchState_ = CDynaBranch::BranchState::BRANCH_TRIPIQ;
			bDisconnected = true;
			break;
			// если ветвь отключена с обратной стороны - отключаем полностью
		case CDynaBranch::BranchState::BRANCH_TRIPIP:
			BranchState_ = CDynaBranch::BranchState::BRANCH_OFF;
			bDisconnected = true;
			break;
		}
	}
	else
		_ASSERTE(!"CDynaNodeContainer::DisconnectBranchFromNode - branch and node mismatch");

	if (bDisconnected)
	{
		Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszSwitchedOffBranch,
			pContainer_->GetTypeName(),
			GetVerbalName(),
			pSwitchOffMode,
			pNode->GetVerbalName()));
	}

	return bDisconnected;
}

void CDynaBranchContainer::CreateMeasures(CDeviceContainer& containerBranchMeasures)
{
	if (containerBranchMeasures.GetType() != DEVTYPE_BRANCHMEASURE)
		throw dfw2error("CDynaBranchContainer::CreateMeasures given container is not CDynaBranchBranchMeasures container");
	containerBranchMeasures.CreateDevices(Count());
	auto itBranch{ begin() };
	auto itMeasure{ containerBranchMeasures.begin() };
	for (; itBranch != end(); itBranch++, itMeasure++)
		static_cast<CDynaBranchMeasure*>(*itMeasure)->SetBranch(static_cast<CDynaBranch*>(*itBranch));
}