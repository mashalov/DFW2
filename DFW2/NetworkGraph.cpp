﻿#include "stdafx.h"
#include "DynaModel.h"
#include "DynaGeneratorMotion.h"

using namespace DFW2;

bool CDynaModel::Link()
{
	bool bRes{ true };

	Nodes.LinkToLRCs(LRCs);
	Nodes.LinkToReactors(Reactors);
	Branches.IndexBranchIds();
	Branches.LinkToReactors(Reactors);

	// делаем отдельные списки ссылок устройств контейнера для ведущих устройств, ведомых устройств 
	// и без учета направления
	for (auto&& it : DeviceContainers_)
	{
		CDeviceContainerProperties &Props = it->ContainerProps();
		// отдельные ссылки без направления для ведущих и ведомых
		Props.Slaves.reserve(Props.LinksFrom_.size() + Props.LinksTo_.size());
		Props.Masters.reserve(Props.Slaves.capacity());

		for (auto&& it1 : Props.LinksTo_)
			if (it1.first != DEVTYPE_MODEL)
			{
				if (it1.second.eDependency == DPD_MASTER)
				{
					Props.MasterLinksTo.insert(std::make_pair(it1.first, &it1.second));	// ведущие к
					Props.Masters.push_back(&it1.second);								// ведущие без направления
				}
				else
				{
					Props.Slaves.push_back(&it1.second);								// ведомые без направления
				}
			}

		for (auto && it2 : Props.LinksFrom_)
			if (it2.first != DEVTYPE_MODEL)
			{
				if (it2.second.eDependency == DPD_MASTER)
				{
					Props.MasterLinksFrom.insert(std::make_pair(it2.first, &it2.second));	// ведущие от
					Props.Masters.push_back(&it2.second);								// ведущие без направления
				}
				else
				{
					Props.Slaves.push_back(&it2.second);								// ведомые без направления
				}
			}
	}


	// линкуем всех со всеми по матрице
	for (auto&& it : DeviceContainers_)				// внешний контейнер
	{
		for (auto&& lt : DeviceContainers_)			// внутренний контейнер
		{
			// не линкуем тип с этим же типом
			if (it != lt)
			{
				LinkDirectionFrom LinkFrom;
				LinkDirectionTo LinkTo;
				// проверяем возможность связи внешнего контейнера со внутренним
				CDeviceContainer *pContLead = it->DetectLinks(lt, LinkTo, LinkFrom);
				if (pContLead == lt)
				{
					// если выбран внутренний контейнер
					Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Связь {} <- {}", it->GetTypeName(), lt->GetTypeName()));
					// организуем связь внешгего контейнера с внутренним
					bRes = it->CreateLink(lt) && bRes;
					_ASSERTE(bRes);
				}
			}
		}
	}
	// создает списки контейнеров в соответствии с их
	// свойствами (особый предиктор, пост-расчет и т.д)
	ConsiderContainerProperties();
	//Branches.CreateMeasures(BranchMeasures);
	return bRes;
}

void CDynaModel::PrepareNetworkElements()
{
	// сбрасываем обработку топологии
	// после импорта данных, в которых могли быть отключенные узлы и ветви
	sc.m_bProcessTopology = false;

	if(Nodes.Count() == 0 || Branches.Count() == 0)
		throw dfw2error(CDFW2Messages::m_cszNoNodesOrBranchesForLF);

	bool bOk{ true };

	// убеждаемся в том, что в исходных данных есть СХН, соотвествующая параметрам

	struct LRCCheckListType
	{
		CDynaLRC** ppLRC;
		ptrdiff_t Id;
	};

	std::array<LRCCheckListType, 3> LRCCheckList =
	{
		{
			{&pLRCIconst_, -2},
			{&pLRCYconst_,  0},
			{&pLRCSconst_, -1},
		}
	};

	// собираем указатели на разные типы СХН и убеждаемся что они заданы
	for (const auto& LRCCheck : LRCCheckList)
	{
		if (*LRCCheck.ppLRC = static_cast<CDynaLRC*>(LRCs.GetDevice(LRCCheck.Id)), *LRCCheck.ppLRC == nullptr)
		{
			Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszMustBeEmbeddedLRC, LRCCheck.Id));
			bOk = false;
		}
	}

	switch (m_Parameters.m_eGeneratorLessLRC)
	{
		case GeneratorLessLRC::Iconst:  pLRCGen_ = pLRCIconst_; break;
		case GeneratorLessLRC::Sconst:  pLRCGen_ = pLRCSconst_; break;
	}

	pLRCLoad_ = pLRCYconst_;

	// для узлов учитываем проводимости реакторов
	// и проверяем значение Uном
	for (auto&& it : Nodes)
	{
		const auto& pNode{ static_cast<CDynaNode*>(it) };
		// задаем V0 для узла. Оно потребуется временно, для холстого расчета шунтовых
		// нагрузок в CreateSuperNodes
		if (Consts::Equal(pNode->Unom, 0.0))
		{
			pNode->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongUnom, pNode->GetVerbalName(), pNode->Unom));
			bOk = false;
		}
		else
			pNode->PickV0();

		pNode->G += pNode->Gr0 * pNode->Nr;
		pNode->B += pNode->Br0 * pNode->Nr;
		// после того как обновили значения G и B обнуляем количество реакторов,
		// чтобы после сериализации мы получили итоговое G и при сериализации получили 
		// правильное значение
		pNode->Nr = 0;
		pNode->Gshunt = pNode->Bshunt = 0.0;
	}

	if (!bOk)
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);

	for (auto&& it : Branches)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(it) };
		// проверяем не самозамкнута ли ветвь
		if (pBranch->key.Iq == pBranch->key.Ip)
		{
			// если ветвь самозамкнута, выключаем ее навсегда
			pBranch->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszBranchLooped, pBranch->key.Ip));
			pBranch->SetBranchState(CDynaBranch::BranchState::BRANCH_OFF, eDEVICESTATECAUSE::DSC_INTERNAL_PERMANENT);
			continue;

		}
		// нулевой коэффициент трансформации заменяем на единичный
		if (Consts::Equal(pBranch->Ktr, 0.0) || Consts::Equal(pBranch->Ktr, 1.0))
		{
			// если ветвь не трансформатор, схема замещения "П"
			pBranch->Ktr = 1.0;
			pBranch->GIp0 = pBranch->GIq0 = pBranch->G / 2.0;
			pBranch->BIp0 = pBranch->BIq0 = pBranch->B / 2.0;
		}
		else
		{
			// если трансформатор, схема замещения "Г"
			pBranch->GIp0 = pBranch->G;
			pBranch->BIp0 = pBranch->B;
			pBranch->GIq0 = pBranch->BIq0 = 0.0;
		}

		// учитываем реакторы в начале и в конце
		pBranch->GIp0 += pBranch->NrIp * pBranch->GrIp;
		pBranch->GIq0 += pBranch->NrIq * pBranch->GrIq;
		pBranch->BIp0 += pBranch->NrIp * pBranch->BrIp;
		pBranch->BIq0 += pBranch->NrIq * pBranch->BrIq;
	}

	if (!bOk)
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);
}

// строит синхронные зоны по топологии и определяет их состояния
void CDynaNodeContainer::BuildSynchroZones()
{
	if (!pSynchroZones)
		pSynchroZones = pDynaModel_->GetDeviceContainer(eDFW2DEVICETYPE::DEVTYPE_SYNCZONE);
	if (!pSynchroZones)
		throw dfw2error("CDynaNodeContainer::ProcessTopology - SynchroZone container not found");

	// модели с углами могут рассчитываться
	// относительно COI. При переходе к новому 
	// составу синхронных зон нужно восстановить
	// углы в синхронные оси !
	if(pDynaModel_->UseCOI())
		for (auto&& SyncZone : *pSynchroZones)
			static_cast<CSynchroZone*>(SyncZone)->AnglesToSyncReference();

	// проверяем, есть ли отключенные узлы
	// даже не знаю что лучше для поиска первого отключенного узла...
	bool bGotOffNodes = std::find_if(DevVec.begin(), DevVec.end(), [](const auto* pNode)->bool { return !pNode->IsStateOn(); }) != DevVec.end();
	
	/*
	bool bGotOffNodes(false);
	for (auto&& it : m_DevVec)
		if (!it->IsStateOn())
		{
			bGotOffNodes = true;
			break;
		}
	*/

	ptrdiff_t nEnergizedCount(1), nDeenergizedCount(1);
	while ((nEnergizedCount || nDeenergizedCount))
	{
		NODEISLANDMAP NodeIslands;
		GetTopologySynchroZones(NodeIslands);
		// если есть отключенные узлы, то количество синхрозон должно быть на 1 больше
		// дополнительная зона для отключенных узлов
		size_t nZonesCount = NodeIslands.size() + (bGotOffNodes ? 1 : 0);

		auto pSyncZones{ pSynchroZones->CreateDevices<CSynchroZone>(nZonesCount) }; /// ???????

		if (bGotOffNodes)
		{
			// если есть отключенные узлы готовим зону для отключенных узлов
			CSynchroZone *pOffZone = pSyncZones + NodeIslands.size();
			// зону отключаем, чтобы она не попадала в систему уравнений
			pOffZone->SetState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_EXTERNAL);
			pOffZone->SetId(NodeIslands.size());
			pOffZone->SetName("SyncZone offline");
			// и ставим эту зону для всех отключенных узлов
			for (auto&& it : DevVec)
				if(!it->IsStateOn())
					static_cast<CDynaNodeBase*>(it)->pSyncZone = pOffZone;
		}

		CSynchroZone* pNewZone{ pSyncZones };

		// для всех зон узлы связываем с зонами
		for (auto&& zone : NodeIslands)
		{
			// готовим место для списка генераторов
			pNewZone->LinkedGenerators.reserve(3 * Count());
			// придумываем идентификатор и имя
			pNewZone->SetId(pNewZone - pSyncZones + 1);
			pNewZone->SetName("SyncZone");
			for (auto&& node : zone.second)
			{
				node->pSyncZone = pNewZone;
				node->MarkZoneEnergized();
			}

			// если в зоне нет генераторов - обозначаем ее как зону с ШБМ
			// (когда будут АД, нужно будет смотреть на них)
			if (pNewZone->LinkedGenerators.empty())
				pNewZone->InfPower = true;

			pNewZone++;
		}
		EnergizeZones(nDeenergizedCount, nEnergizedCount);
	}
}

// изменяет готовность узлов к включению или выключению по признаку зоны - под напряжением или без напряжения
void CDynaNodeContainer::EnergizeZones(ptrdiff_t &nDeenergizedCount, ptrdiff_t &nEnergizedCount)
{
	bool bRes{ true };
	nDeenergizedCount = nEnergizedCount = 0;
	//return;
	// проходим по узлам
	for (auto&& it : DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		// если в узле есть синхронная зона
		if (!pNode->pSyncZone)
			throw dfw2error("CDynaNodeContainer::EnergizeZones SyncZone not assigned");
		// и она под напряжением
		if (pNode->pSyncZone->Energized)
		{
			// если узел отключен и имеет включенные связи
			// !!!!!!! Тут скорее всего придется включать острова по связям, так же как мы отключаем острова !!!!!!
			if (!pNode->IsStateOn() && !pNode->IsDangling())
			{
				// и он был отключен внутренней командной фреймворка (например был в зоне со снятым напряжением)
				if (pNode->GetStateCause() == eDEVICESTATECAUSE::DSC_INTERNAL)
				{
					// включаем узел
					pNode->ChangeState(eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_INTERNAL);
					pNode->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszNodeRiseDueToZone, pNode->GetVerbalName()));
					// считаем количество включенных узлов
					nEnergizedCount++;
				}
			}
		}
		else
		{
			// если зона со снятым напряжением
			// и узел включен
			if (pNode->IsStateOn())
			{
				// отключаем узел с признаком, что его состояние изменилось внутренней командой фреймворка
				pNode->ChangeState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
				pNode->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszNodeTripDueToZone, pNode->GetVerbalName()));
				// считаем количество отключенных узлов
				nDeenergizedCount++;
			}
		}
	}
}

// ставит признак зоны узла - под напряжением или со снятым напряжением
void CDynaNodeBase::MarkZoneEnergized()
{
	if (!pSyncZone)
		throw dfw2error("CDynaNodeBase::MarkZoneEnergized SyncZone not assigned");

	if (IsStateOn())
	{
		// проходим по связям с генераторами
		const CLinkPtrCount* const pLink{ GetLink(1) };
		LinkWalker<CDynaPowerInjector> pGen;
		while (pLink->In(pGen))
		{
			// если генератор есть и он включен
			if (pGen->IsKindOfType(DEVTYPE_VOLTAGE_SOURCE) && pGen->IsStateOn())
			{
				// зону ставим под напряжение
				pSyncZone->Energized = true;
				// добавляем генератор в список зоны
				pSyncZone->LinkedGenerators.push_back(pGen);
				// если к тому же генератор ШБМ, то ставим признак зоны ШБМ
				if (pGen->GetType() == DEVTYPE_GEN_INFPOWER)
					pSyncZone->InfPower = true;
				else
					// иначе считаем общий момент инерации зоны (если у генератора есть уравнение движения)
					if (pGen->IsKindOfType(DEVTYPE_GEN_MOTION))
						pSyncZone->Mj += static_cast<CDynaGeneratorMotion*>(static_cast<CDynaPowerInjector*>(pGen))->Mj;
			}
		}
	}
}

void CDynaNodeContainer::DumpNodeIslands(NODEISLANDMAP& Islands)
{
	for (auto&& supernode : Islands)
	{
		pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszIslandOfSuperNode, supernode.first->GetVerbalName()));
		for (auto&& slavenode : supernode.second)
			pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("--> {}", slavenode->GetVerbalName()));
	}
}

void CDynaNodeContainer::GetNodeIslands(NODEISLANDMAP& JoinableNodes, NODEISLANDMAP& Islands)
{
	/*
	procedure DFS - iterative(G, v) :
	2      let S be a stack
	3      S.push(v)
	4      while S is not empty
	5          v = S.pop()
	6          if v is not labeled as discovered :
	7              label v as discovered
	8              for all edges from v to w in G.adjacentEdges(v) do
	9                  S.push(w)
	*/

	Islands.clear();	// очищаем результат
	std::stack<CDynaNodeBase*> Stack;

	// вырабатываем сет заданных узлов
	while (!JoinableNodes.empty())
	{
		// ищем первый узел, предопочитаем базисный
		// если нет - то PV
		auto Slack = JoinableNodes.begin();
		// ищем узел для построения суперузла в порядке базисный, PV, PQ
		for (auto it = JoinableNodes.begin(); it != JoinableNodes.end(); it++)
		{
			// пропускаем нагрузочные (первый уже есть)
			if (it->first->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PQ)
				continue;
			else if (it->first->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_BASE)
			{
				// если нашли базисный - сразу берем его
				Slack = it;
				break;
			}
			else // остались только PV-узлы, выбираем текущий по наиболее широкому диапазону
			{
				if (Slack->first->IsLFTypePV())
				{
					if ((Slack->first->LFQmax - Slack->first->LFQmin) < (it->first->LFQmax - it->first->LFQmin))
						Slack = it;
				}
				else
					Slack = it;
			}
		}

		// вставляем первый узел как основу для острова
		const auto& CurrentSuperNode = Islands.insert(std::make_pair(Slack->first, std::set<CDynaNodeBase*>{}));
		// берем первый узел из сета и готовим к DFS
		Stack.push(Slack->first);

		// делаем стандартный DFS
		while (!Stack.empty())
		{
			// к текущему острову добавляем найденный узел
			CurrentSuperNode.first->second.insert(Stack.top());
			// проверяем, есть ли найденный узел во входном сете
			const auto& jit = JoinableNodes.find(Stack.top());
			Stack.pop();
			// если есть - добавляем в стек все связанные с найденным узлы
			if (jit != JoinableNodes.end())
			{
				for (auto && bit : jit->second)
					Stack.push(bit);
				// а сам найденный узел удаляем из входного сета
				JoinableNodes.erase(jit);
			}
		}
	}
}

// создает и формирует проводимости сети в суперузлах
void CDynaNodeContainer::CreateSuperNodes()
{
	// отдельные вызовы для построения структуры
	CreateSuperNodesStructure();
	// и расчета проводимостей
	CalculateSuperNodesAdmittances();
	// позволяют использовать функции раздельно для подготовки
	// данных к расчету УР и коррекции отрицательных сопротивлений для Зейделя
	// с последующим возвратом к нормальным сопротивлениям для Ньютона
	// без полной пересборки суперузлов
}

void CDynaNodeContainer::CreateSuperNodesStructure()
{
	CDeviceContainer* pBranchContainer{ pDynaModel_->GetDeviceContainer(DEVTYPE_BRANCH) };
	if(!pBranchContainer)
		throw dfw2error("CDynaNodeContainer::CreateSuperNodes - Branch container not found");

	ClearSuperLinks();
	// обнуляем ссылки подчиненных узлов на суперузлы
	for (auto&& nit : DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(nit) };
		pNode->pSuperNodeParent = nullptr;
		pNode->ZeroLF.SuperNodeLFIndex = 0;
	}

	// очищаем список суперузлов для расчета потокораспределения с нулевыми сопротивлениями
	ZeroLFSet.clear();

	NODEISLANDMAP JoinableNodes, SuperNodes;
	// строим список связности по включенным ветвям с нулевым сопротивлением
	ptrdiff_t ZeroBranchCount{ 0 };
	for (auto&& it : *pBranchContainer)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(it) };
		// по умолчанию суперузлы ветви равны исходным узлам
		pBranch->pNodeSuperIp_ = pBranch->pNodeIp_;
		pBranch->pNodeSuperIq_ = pBranch->pNodeIq_;
		if (pBranch->IsZeroImpedance())
		{
			JoinableNodes[pBranch->pNodeIp_].insert(pBranch->pNodeIq_);
			JoinableNodes[pBranch->pNodeIq_].insert(pBranch->pNodeIp_);
			ZeroBranchCount += 2;	// учитываем, что ветвь может учитываться в двух узлах два раза
		}
	}
	// получаем список островов
	GetNodeIslands(JoinableNodes, SuperNodes);

	// строим связи от подчиенных узлов к суперузлам
	// готовим вариант связи с ветвями
	// в дополнительном списке связей

	DumpNodeIslands(SuperNodes);

		
	SuperLinks.emplace_back(this, Count()); // используем конструктор CMultiLink
	CMultiLink& pNodeSuperLink(SuperLinks.back());
	// заполняем список в два прохода: на первом считаем количество, на втором - заполняем ссылки
	for (int pass = 0; pass < 2; pass++)
	{
		for (auto&& nit : SuperNodes)
			for (auto&& sit : nit.second)
				if (nit.first != sit)
				{
					sit->pSuperNodeParent = nit.first;
					if (pass)
						AddLink(pNodeSuperLink, nit.first->InContainerIndex(), sit);
					else
						IncrementLinkCounter(pNodeSuperLink, nit.first->InContainerIndex());
				}
		if (pass == 0)
			AllocateLinks(pNodeSuperLink);	// на первом проходe размечаем ссылки по узлам
	}

	const ptrdiff_t nSuperNodesCount{ std::count_if(DevVec.begin(), DevVec.end(), [](const CDevice* pDev)
		{
			return static_cast<const CDynaNodeBase*>(pDev)->pSuperNodeParent == nullptr;
		})
	};

	Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszTopologyNodesCreated,
		nSuperNodesCount,
		Count(),
		Count() > 0 ? 100.0 * nSuperNodesCount / Count() : 0.0));

	// перестраиваем связи суперузлов с ветвями:
	SuperLinks.emplace_back(Links_[0].pContainer_, Count());
	CMultiLink& pBranchSuperLink(SuperLinks.back());

	// заполняем список в два прохода: на первом считаем количество, на втором - заполняем ссылки
	for (int pass = 0; pass < 2; pass++)
	{
		for (auto&& it : *pBranchContainer)
		{
			// учитываем только включенные ветви (по идее можно фильтровать и ветви с нулевым сопротивлением)
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(it);
				
			// Здесь включаем все ветви: и включенные и отключенные, иначе надо всякий раз перестраивать матрицу
			if (pBranch->BranchState_ != CDynaBranch::BranchState::BRANCH_ON)
				continue;

			const auto& pNodeIp(pBranch->pNodeIp_);
			const auto& pNodeIq(pBranch->pNodeIq_);

			if (pNodeIp->pSuperNodeParent == pNodeIq->pSuperNodeParent)
			{
				// суперузлы в начале и в конце одинаковые
				if (!pNodeIp->pSuperNodeParent)
				{
					// суперузлы отсуствуют - ветвь между двумя обычными узлами или суперузлами
					if (pass)
					{
						// на втором проходе добавляем ссылки
						AddLink(pBranchSuperLink, pNodeIp->InContainerIndex(), pBranch);
						AddLink(pBranchSuperLink, pNodeIq->InContainerIndex(), pBranch);
					}
					else
					{
						// на первом проходе считаем количество ссылок
						IncrementLinkCounter(pBranchSuperLink, pNodeIp->InContainerIndex());
						IncrementLinkCounter(pBranchSuperLink, pNodeIq->InContainerIndex());
					}
					//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("Branch %s connects supernodes %s and %s", pBranch->GetVerbalName(), pNodeIp->GetVerbalName(), pNodeIq->GetVerbalName()));
				}
				else
					// иначе суперузел есть - и ветвь внутри него
					pBranch->pNodeSuperIp_ = pBranch->pNodeSuperIq_ = pNodeIp->pSuperNodeParent;
					//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("Branch %s in super node %s", pBranch->GetVerbalName(), pNodeIp->m_pSuperNodeParent->GetVerbalName()));
			}
			else
			{
				// суперузлы в начале и в конце разные
				// если у узлов есть суперузел - связываем ветвь не с узлом, а с его
				// суперузлом

				const auto& pNodeIpSuper{ pBranch->pNodeSuperIp_ = pNodeIp->GetSuperNode() };
				const auto& pNodeIqSuper{ pBranch->pNodeSuperIq_ = pNodeIq->GetSuperNode() };

				// если суперузел у узлов ветви не общий
				if (pNodeIpSuper != pNodeIqSuper)
				{
					if (pass)
					{
						AddLink(pBranchSuperLink, pNodeIpSuper->InContainerIndex(), pBranch);
						AddLink(pBranchSuperLink, pNodeIqSuper->InContainerIndex(), pBranch);
						// в ветви нет одиночных связей, но есть два адреса узлов начала и конца.
						// их меняем на адреса суперузлов
					}
					else
					{
						IncrementLinkCounter(pBranchSuperLink, pNodeIpSuper->InContainerIndex());
						IncrementLinkCounter(pBranchSuperLink, pNodeIqSuper->InContainerIndex());
					}
				}
				/*
				const char* cszSuper = "super";
				const char* cszSlave = "super";
				m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("Branch %s connects %snode %s and %s node %s",
					pBranch->GetVerbalName(),
					pNodeIpSuper->m_pSuperNodeParent ? cszSlave : cszSuper,
					pNodeIpSuper->GetVerbalName(),
					pNodeIqSuper->m_pSuperNodeParent ? cszSlave : cszSuper,
					pNodeIqSuper->GetVerbalName()));
				*/
			}
		}

		if(pass == 0)
			AllocateLinks(pBranchSuperLink);	// на первом проходе размечаем ссылки по узлам
	}

	// перестраиваем все остальные связи кроме ветвей
	// идем по мультиссылкам узла
	for (auto&& multilink : Links_)
	{
		// если ссылка на контейнер ветвей - пропускаем (обработали выше отдельно)
		if (multilink.pContainer_->GetType() == DEVTYPE_BRANCH)
			continue;
		// определяем индекс ссылки один-к-одному в контейнере, с которым связаны узлы
		// для поиска индекса контейнер запрашиваем по типу связи "Узел"
		ptrdiff_t nLinkIndex = multilink.pContainer_->GetSingleLinkIndex(DEVTYPE_NODE);

		// создаем дополнительную мультисвязь и добавляем в список связей суперузлов
		SuperLinks.emplace_back(multilink.pContainer_, Count());
		CMultiLink& pSuperLink(SuperLinks.back());
		// для хранения оригинальных связей устройств с узлами используем карту устройство-устройство
		OriginalLinks.push_back(std::make_unique<DEVICETODEVICEMAP>(DEVICETODEVICEMAP()));


		// обрабатываем связь в два прохода : подсчет + выделение памяти и добавление ссылок
		for (int pass = 0; pass < 2; pass++)
		{
			// идем по узлам
			for (auto&& node : DevVec)
			{
				CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(node);
				CDynaNodeBase *pSuperNode = pNode;
				// если у узла есть суперузел, то берем его указатель
				// иначе узел сам себе суперузел
				if (pNode->pSuperNodeParent)
					pSuperNode = pNode->pSuperNodeParent;
				// достаем из узла мультиссылку на текущий тип связи
				const  CLinkPtrCount* const  pLink{ multilink.GetLink(pNode->InContainerIndex()) };
				LinkWalker<CDevice> pDevice;
				// идем по мультиссылке
				while (pLink->In(pDevice))
				{
					if (pass)
					{
						// добавляем к суперузлу ссылку на внешее устройство
						AddLink(pSuperLink, pSuperNode->InContainerIndex(), pDevice);
						// указатель на прежний узел в устройстве, которое сязано с узлом
						CDevice *pOldDev = pDevice->GetSingleLink(nLinkIndex);
						// заменяем ссылку на старый узел ссылкой на суперузел
						pDevice->SetSingleLink(nLinkIndex, pSuperNode);
						// сохраняем оригинальную связь устройства с узлом в карте
						OriginalLinks.back()->insert(std::make_pair(pDevice, pOldDev));
						//*
						//string strName(pOldDev ? pOldDev->GetVerbalName() : "");
						//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("Change link of object %s from node %s to supernode %s",
						//		(*ppDevice)->GetVerbalName(),
						//		strName.c_str(),SuperNodeBlock.first->GetVerbalName()));
					}
					else
						IncrementLinkCounter(pSuperLink, pSuperNode->InContainerIndex()); // на первом проходе просто считаем количество связей
				}
			}

			if (pass == 0)
				AllocateLinks(pSuperLink);	// на первом проходе размечаем ссылки по узлам
		}
	}

	/*
	for (auto&& node : m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(node);
		if (pNode->m_pSuperNodeParent || pNode->GetState() != eDEVICESTATE::DS_ON)
			continue;
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("%snode %s", pNode->m_pSuperNodeParent ? "": "super", pNode->GetVerbalName()));

		for (auto&& superlink : m_SuperLinks)
		{
			CDevice **ppDevice(nullptr);
			pNode->ResetVisited();
			CLinkPtrCount *pLink = superlink->GetLink(pNode->m_nInContainerIndex);
			while (pLink->In(ppDevice))
			{
				m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("\tchild %s", (*ppDevice)->GetVerbalName()));
			}
		}
	}
	*/


	//  Создаем виртуальные ветви
	// Количество виртуальных ветвей не превышает количества ссылок суперузлов на ветви
	pVirtualBranches = std::make_unique<VirtualBranch[]>(SuperLinks[1].Count_);
	// Создаем список ветвей с нулевым сопротивлением внутри суперузла
	// Ветви с нулевым сопротивлением хранятся в общем векторе контейнера
	// узлы имеют три указателя на этот вектор - начало и конец для всех ветвей
	// и указатель на список параллельных ветвей (см. TidyZeroBranches)
	pZeroBranches = std::make_unique<VirtualZeroBranch[]>(ZeroBranchCount);
	pZeroBranchesEnd_ = pZeroBranches.get() + ZeroBranchCount;

	if (ZeroBranchCount)
	{
		VirtualZeroBranch* pCurrentZeroBranch{ pZeroBranches.get() };
		for (auto&& node : DevVec)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(node) };
			// формируем диапазоны ветвей с нулевым сопротивлением для узлов (просто инициализация)
			pNode->pVirtualZeroBranchBegin_ = pNode->pVirtualZeroBranchEnd_ = pCurrentZeroBranch;
			// если у нас есть ветви с нулевым сопротивлением строим их списки
			// если узел входит в суперузел - его нулевые ветви нам не нужны, 
			// они будут учтены с узла представителя суперузла
			// если ветвей с нулевым сопротивлением нет вообще - пропускаем обработку
			// (цикл не обходим чтобы инициализировать m_VirtualZeroBranchBegin = m_VirtualZeroBranchEnd
			if (pNode->pSuperNodeParent)
				continue;

			CDynaNodeBase* pSlave{ pNode };
			const CLinkPtrCount* const pSlaveNodeLink{ pNode->GetSuperLink(0) };
			LinkWalker<CDynaNodeBase> pSlaveNode;

			// сначала обрабатываем узел-представитель суперузла
			// затем выбираем входящие в суперузел узлы
			while (pSlave)
			{
				const CLinkPtrCount* const pBranchLink{ pSlave->GetLink(0) };
				LinkWalker<CDynaBranch> pBranch;
				while (pBranchLink->In(pBranch))
					pCurrentZeroBranch = pNode->AddZeroBranch(pBranch);

				// достаем из суперссылки на узлы следующий узел суперузла
				pSlave = pSlaveNodeLink->In(pSlaveNode) ? static_cast<CDynaNodeBase*>(pSlaveNode) : nullptr;
			}

			// приводим ссылки на нулевые ветви к нужному формату для последующей обработки в циклах
			pNode->TidyZeroBranches();
		}
	}
}


void CDynaNodeContainer::CalculateSuperNodesAdmittances(bool bFixNegativeZs)
{
	// Рассчитываем проводимости узлов и ветвей только после того, как провели
	// топологический анализ. Это позволяет определить ветви внутри суперузлов
	// которые оказались параллельны нулевым ветвям, но имеют сопротивление выше
	// порогового. Такие ветви можно определить по тому, что они отнесены к одному и 
	// тому же суперзлу. При этом не нужно искать параллельные нулевые ветви
	CalcAdmittances(bFixNegativeZs);
	VirtualBranch* pCurrentBranch{ pVirtualBranches.get() };
	for (auto&& node : DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(node) };
		// если узел входит в суперузел - для него виртуальные ветви отсутствуют
		pNode->pVirtualBranchBegin_ = pNode->pVirtualBranchEnd_ = pCurrentBranch;
		if (pNode->pSuperNodeParent)
			continue;
		const CLinkPtrCount* const pBranchLink{ pNode->GetSuperLink(1) };
		pNode->ResetVisited();
		LinkWalker<CDynaBranch> pBranch;
		while (pBranchLink->In(pBranch))
		{
			// обходим включенные ветви также как и для подсчета размерностей выше
			const auto& pOppNode = pBranch->GetOppositeSuperNode(pNode);

			// получаем проводимость к оппозитному узлу
			_ASSERTE((pBranch->pNodeSuperIp_ == pNode && pBranch->pNodeSuperIq_ != pNode) || 
					 (pBranch->pNodeSuperIq_ == pNode && pBranch->pNodeSuperIp_ != pNode));

			// определяем направление ветви не по адресу узла, а по адресу суперузла,
			// так как нас интересует направление непереадресованных ветвей между суперузлами
			const cplx& Ykm{ pBranch->pNodeSuperIp_ == pNode ? pBranch->Yip : pBranch->Yiq };
			// проверяем, уже прошли данный оппозитный узел для просматриваемого узла или нет
			ptrdiff_t DupIndex = pNode->CheckAddVisited(pOppNode);
			if (DupIndex < 0)
			{
				// если нет - добавляем ветвь в список данного узла
				pCurrentBranch->Y = Ykm;
				pCurrentBranch->pNode = pOppNode;
				pCurrentBranch++;
			}
			else
				(pNode->pVirtualBranchBegin_ + DupIndex)->Y += Ykm; // если оппозитный узел уже прошли, ветвь не добавляем, а суммируем ее проводимость параллельно с уже пройденной ветвью
		}
		pNode->pVirtualBranchEnd_ = pCurrentBranch;
		// проверяем есть ли включенные ветви для данного узла
		// но лучше пользоваться синхронными зонами, которые почему-то были 
		// отключены. Зоны восстановлены 16.06.2021
		//if (pNode->m_VirtualBranchBegin == pNode->m_VirtualBranchEnd)
		//	pNode->ChangeState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
	}
	// считаем проводимости и шунтовые части нагрузки
	// для суперузлов выделенной функцией
	CalculateShuntParts();
}

void CDynaNodeContainer::ClearSuperLinks()
{
	SuperLinks.clear();
	OriginalLinks.clear();
}

CDynaNodeBase* CDynaNodeContainer::FindGeneratorNodeInSuperNode(CDevice *pGen)
{
	CDynaNodeBase *pRet(nullptr);
	// ищем генератор в карте сохраненных связей и возвращаем оригинальный узел
	if (pGen && OriginalLinks.size() > 0)
	{
		const auto& GenMap = OriginalLinks[0];
		const auto& itGen = GenMap->find(pGen);
		if (itGen != GenMap->end())
			pRet = static_cast<CDynaNodeBase*>(itGen->second);
	}
	return pRet;
}

const CLinkPtrCount* const CDynaNodeBase::GetSuperLink(ptrdiff_t LinkIndex)
{
	_ASSERTE(pContainer_);
	return static_cast<CDynaNodeContainer*>(pContainer_)->GetCheckSuperLink(LinkIndex, InContainerIndex_).GetLink(InContainerIndex_);
}

CMultiLink& CDynaNodeContainer::GetCheckSuperLink(ptrdiff_t LinkIndex, ptrdiff_t DeviceIndex)
{
	return GetCheckLink(LinkIndex, DeviceIndex, SuperLinks);
}



std::string CDynaNodeContainer::GetIterationControlString()
{
	
	std::string retString = fmt::format("{:4} {:15f} {:>11} {:15f} {:>11} {:5.2f} {:>11} {:5.2f} {:>11} {:>5}",
		IterationControl_.Number,
		IterationControl_.MaxImbP.GetDiff(), IterationControl_.MaxImbP.GetId(),
		IterationControl_.MaxImbQ.GetDiff(), IterationControl_.MaxImbQ.GetId(),
		IterationControl_.MaxV.GetDiff(), IterationControl_.MaxV.GetId(),
		IterationControl_.MinV.GetDiff(), IterationControl_.MinV.GetId(),
		IterationControl_.QviolatedCount,
		IterationControl_.ImbRatio);
	return retString;
}

void CDynaNodeContainer::DumpIterationControl(DFW2MessageStatus OutputStatus)
{
	pDynaModel_->Log(OutputStatus, GetIterationControlString());
}

// функция готовит топологию к дианамике в первый раз
// учитывает, что суперузлы уже могли быть построены в процессе расчета УР
// функция должна повторять ProcessTopology за исключением условного вызова
// CreateSuperNodes()
void CDynaNodeContainer::ProcessTopologyInitial()
{
	// если суперузлы отсутствуют, строим суперузлы
	// если есть считаем что они построены в УР
	if (SuperLinks.empty())
		CreateSuperNodes();
	else
		CalculateSuperNodesAdmittances();
	BuildSynchroZones();
	pDynaModel_->RebuildMatrix(true);
}	

// функция обработки топологии, вызывается в процессе расчета динамики
void CDynaNodeContainer::ProcessTopology()
{
	CreateSuperNodesStructure();
	// включаем или выключаем узлы по зонам
	BuildSynchroZones();
	// и только потом считаем проводимости узлов
	CalculateSuperNodesAdmittances();
	pDynaModel_->RebuildMatrix(true);
}

void CDynaNodeBase::AddToTopologyCheck()
{
	if (pContainer_)
		static_cast<CDynaNodeContainer*>(pContainer_)->AddToTopologyCheck(this);
}

void CDynaNodeContainer::AddToTopologyCheck(CDynaNodeBase *pNode)
{
	TopologyCheck.insert(pNode);
}

void CDynaNodeContainer::ResetTopologyCheck()
{
	TopologyCheck.clear();
}

void CDynaNodeContainer::SwitchOffDanglingNodes(NodeSet& Queue)
{
	// обрабатываем узлы до тех пор, пока есть отключения узлов при проверке
	while (!Queue.empty())
	{
		const auto& it = Queue.begin();
		CDynaNodeBase *pNextNode = *it;
		Queue.erase(it);
		SwitchOffDanglingNode(pNextNode, Queue);
	}
}

void CDynaNodeContainer::GetTopologySynchroZones(NODEISLANDMAP& NodeIslands)
{
	// формируем список связности узлов по включенным ветвям
	CDeviceContainer* pNodeContainer { pDynaModel_->GetDeviceContainer(DEVTYPE_NODE) };
	CDeviceContainer* pBranchContainer{ pDynaModel_->GetDeviceContainer(DEVTYPE_BRANCH) };
	NODEISLANDMAP JoinableNodes;

	// сначала вводим в список узлов все включенные, чтобы в списке оказались все узлы, даже без связей с базой
	for (auto&& it : *pNodeContainer)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		if (pNode->GetState() == eDEVICESTATE::DS_ON)
			JoinableNodes.insert(std::make_pair(pNode, NodeSet()));
	}

	for (auto&& it : *pBranchContainer)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(it) };
		if (pBranch->BranchState_ == CDynaBranch::BranchState::BRANCH_ON)
		{
			JoinableNodes[pBranch->pNodeIp_].insert(pBranch->pNodeIq_);
			JoinableNodes[pBranch->pNodeIq_].insert(pBranch->pNodeIp_);
		}
	}
	// формируем перечень островов
	GetNodeIslands(JoinableNodes, NodeIslands);
	pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszIslandCount, NodeIslands.size()));
}

void CDynaNodeContainer::PrepareLFTopology()
{
	NodeSet Queue;
	// проверяем все узлы на соответствие состояния их самих и инцидентных ветвей
	for (auto&& node : DevVec)
		SwitchOffDanglingNode(static_cast<CDynaNodeBase*>(node), Queue);
	// отключаем узлы попавшие в очередь отключения
	SwitchOffDanglingNodes(Queue);
	
	NODEISLANDMAP NodeIslands;
	GetTopologySynchroZones(NodeIslands);

	if (!pSynchroZones)
		pSynchroZones = pDynaModel_->GetDeviceContainer(eDFW2DEVICETYPE::DEVTYPE_SYNCZONE);
	if (!pSynchroZones)
		throw dfw2error("CDynaNodeContainer::ProcessTopology - SynchroZone container not found");

	auto pSyncZones{ pSynchroZones->CreateDevices<CSynchroZone>(NodeIslands.size()) }; /// ???????

	for (auto&& island : NodeIslands)
	{
		// для каждого острова считаем количество базисных узлов и средний угол
		size_t nSlacks{ 0 };
		// средний угол рассчитываем в Mj (для УР он не нужен)
		pSyncZones->Mj = 0.0;
		for (auto&& node : island.second)
		{
			node->pSyncZone = pSyncZones;
			if (node->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_BASE)
			{
				nSlacks++;
				pSyncZones->Mj += node->Delta;
			}
		}

		pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszIslandSlackBusesCount, 
			island.second.size(), 
			nSlacks, 
			pSyncZones->Mj * 180.0 / M_PI));

		if (!nSlacks)
		{
			// если базисных узлов в острове нет - отключаем все узлы острова
			pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszIslandNoSlackBusesShutDown));
			NodeSet& Queue = island.second;
			for (auto&& node : Queue)
			{
				pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszNodeShutDownAsNotLinkedToSlack, node->GetVerbalName()));
				node->ChangeState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
			}
			// и отключаем ветви
			SwitchOffDanglingNodes(Queue);
		}
		else
			pSyncZones->Mj /= nSlacks;

		pSyncZones++;
	}
}

void CDynaNodeContainer::SwitchOffDanglingNode(CDynaNodeBase *pNode, NodeSet& Queue)
{
	if (pNode->GetState() == eDEVICESTATE::DS_ON)
	{
		// проверяем есть ли хотя бы одна включенная ветвь к этому узлу
		if (pNode->IsDangling())
		{
			// все ветви отключены, отключаем узел
			pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszSwitchedOffNode, pNode->GetVerbalName()));
			// сбрасываем список проверки инцидентных узлов
			ResetTopologyCheck();
			// используем функцию ChangeState(), которая отключает все, что отнесено к отключаемому узлу, в том числе и ветви
			pNode->ChangeState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
			// и этот узел ставим в очередь на повторную проверку отключения ветвей
			Queue.insert(pNode);
			// в очередь также ставим все узлы, которые были инцидентны данному и состояния ветвей до которых изменились
			Queue.insert(TopologyCheck.begin(), TopologyCheck.end());
			// после ChangeState ветви, инцидентные узлу изменят состояние, поэтому надо проверить узлы на концах изменивших состояние ветвей
		}
	}
	else
	{
		// отключаем все ветви к этому узлу 
		const CLinkPtrCount* const pLink{ pNode->GetLink(0) };
		LinkWalker<CDynaBranch> pBranch;
		while (pLink->In(pBranch))
		{
			if (pBranch->DisconnectBranchFromNode(pNode))
			{
				// состояние ветви изменилось, узел с другой стороны ставим в очередь на проверку
				const auto& pOppNode{ pBranch->GetOppositeNode(pNode) };
				if (pOppNode->GetState() == eDEVICESTATE::DS_ON)
					Queue.insert(pOppNode);
			}
		}
	}
}

void CDynaNodeContainer::DumpNetwork()
{
	std::ofstream dump(GetModel()->Platform().ResultFile(fmt::format("network-{}.net", GetModel()->GetStepNumber())));
	if (dump.is_open())
	{
		for (auto& node : DevVec)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(node) };
			dump << fmt::format("Node Id={} DBIndex={} V {} / {}", pNode->GetId(), pNode->GetDBIndex(), pNode->V.Value, pNode->Delta.Value / M_PI * 180.0);
			if(pNode->pSuperNodeParent)
				dump << fmt::format(" belongs to supernode Id={}", pNode->pSuperNodeParent->GetId());
			dump << std::endl;

			const CLinkPtrCount* const pBranchLink{ pNode->GetLink(0) };
			LinkWalker<CDynaBranch> pBranch;
			while (pBranchLink->In(pBranch))
			{
				dump << fmt::format("\tOriginal Branch %{}-%{}-({}) r={} x={} state={}",
					pBranch->key.Ip, pBranch->key.Iq, pBranch->key.Np,
					pBranch->R, pBranch->X, pBranch->BranchState_) << std::endl;
			}

			const CLinkPtrCount* const pSuperNodeLink{ pNode->GetSuperLink(0) };

			LinkWalker<CDynaNodeBase> pSlaveNode;
			while (pSuperNodeLink->In(pSlaveNode))
			{
				dump << fmt::format("\t\tSlave Node Id={} DBIndex={}", pSlaveNode->GetId(), pSlaveNode->GetDBIndex()) << std::endl;

				const CLinkPtrCount* const  pBranchLink{ pSlaveNode->GetLink(0) };
				LinkWalker<CDynaBranch> pDeviceBranch;

				while (pBranchLink->In(pDeviceBranch))
				{
					dump << fmt::format("\t\t\tOriginal Branch %{}-%{}-({}) r={} x={} state={}", 
						pDeviceBranch->key.Ip, pDeviceBranch->key.Iq, pDeviceBranch->key.Np,
						pDeviceBranch->R, pDeviceBranch->X, pDeviceBranch->BranchState_) << std::endl;
				}
			}

			for (VirtualBranch* pV = pNode->pVirtualBranchBegin_; pV < pNode->pVirtualBranchEnd_; pV++)
				dump << fmt::format("\tVirtual Branch to node Id={}", pV->pNode->GetId()) << std::endl;
		}
	}
	std::ofstream lulf(GetModel()->Platform().ResultFile(fmt::format("nodes LULF-{}.csv", GetModel()->GetStepNumber())));
	if (lulf.is_open())
	{
		for (auto& node : DevVec)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(node) };
			lulf << fmt::format("{};{};{}", pNode->GetId(), pNode->V.Value, pNode->Delta.Value / M_PI * 180.0) << std::endl;
		}
	}
}