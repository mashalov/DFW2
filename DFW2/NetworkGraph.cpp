#include "stdafx.h"
#include "DynaModel.h"
#include "DynaGeneratorMotion.h"

using namespace DFW2;
bool CDynaModel::Link()
{
	bool bRes = true;

	Nodes.LinkToLRCs(LRCs);

	// делаем отдельные списки ссылок устройств контейнера для ведущих устройств, ведомых устройств 
	// и без учета направления
	for (auto&& it : m_DeviceContainers)
	{
		CDeviceContainerProperties &Props = it->m_ContainerProps;
		// отдельные ссылки без направления для ведущих и ведомых
		Props.m_Slaves.reserve(Props.m_LinksFrom.size() + Props.m_LinksTo.size());
		Props.m_Masters.reserve(Props.m_Slaves.capacity());

		for (auto&& it1 : Props.m_LinksTo)
			if (it1.first != DEVTYPE_MODEL)
			{
				if (it1.second.eDependency == DPD_MASTER)
				{
					Props.m_MasterLinksTo.insert(std::make_pair(it1.first, &it1.second));	// ведущие к
					Props.m_Masters.push_back(&it1.second);								// ведущие без направления
				}
				else
				{
					Props.m_Slaves.push_back(&it1.second);								// ведомые без направления
				}
			}

		for (auto && it2 : Props.m_LinksFrom)
			if (it2.first != DEVTYPE_MODEL)
			{
				if (it2.second.eDependency == DPD_MASTER)
				{
					Props.m_MasterLinksFrom.insert(std::make_pair(it2.first, &it2.second));	// ведущие от
					Props.m_Masters.push_back(&it2.second);								// ведущие без направления
				}
				else
				{
					Props.m_Slaves.push_back(&it2.second);								// ведомые без направления
				}
			}
	}


	// линкуем всех со всеми по матрице
	for (auto&& it : m_DeviceContainers)			// внешний контейнер
	{
		for (auto&& lt : m_DeviceContainers)		// внутренний контейнер
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
					Log(DFW2::CDFW2Messages::DFW2LOG_INFO, fmt::format("Связь {} <- {}", it->GetTypeName(), lt->GetTypeName()));
					// организуем связь внешгего контейнера с внутренним
					bRes = it->CreateLink(lt) && bRes;
					_ASSERTE(bRes);
				}
			}
		}
	}
	// по атрибутам контейнеров формируем отдельные списки контейнеров для
	// обновления после итерации Ньютона и после прогноза, чтобы не проверять эти атрибуты в основных циклах
	for (auto&& it : m_DeviceContainers)
	{
		if (it->m_ContainerProps.bNewtonUpdate)
			m_DeviceContainersNewtonUpdate.push_back(it);
		if (it->m_ContainerProps.bPredict)
			m_DeviceContainersPredict.push_back(it);
	}

	return bRes;
}

void CDynaModel::PrepareNetworkElements()
{
	// сбрасываем обработку топологии
	// после импорта данных, в которых могли быть отключенные узлы и ветви
	sc.m_bProcessTopology = false;

	if(Nodes.Count() == 0 || Branches.Count() == 0)
		throw dfw2error(CDFW2Messages::m_cszNoNodesOrBranchesForLF);

	bool bOk(true);

	// убеждаемся в том, что в исходных данных есть СХН с постоянной мощностью
	if (!(m_pLRCGen = static_cast<CDynaLRC*>(LRCs.GetDevice(-1))))
	{
		Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszMustBeConstPowerLRC);
		bOk = false;
	}
	// убеждаемся в том, что в исходных данных есть СХН для постоянной генерации в динамике
	if (!(m_pLRCLoad = static_cast<CDynaLRC*>(LRCs.GetDevice((ptrdiff_t)0))))
	{
		Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszMustBeConstPowerLRC);
		bOk = false;
	}

	// для узлов учитываем проводимости реакторов
	// и проверяем значение Uном
	for (auto&& it : Nodes)
	{
		CDynaNode* pNode = static_cast<CDynaNode*>(it);
		// задаем V0 для узла. Оно потребуется временно, для холстого расчета шунтовых
		// нагрузок в CreateSuperNodes
		if (pNode->Unom < DFW2_EPSILON)
		{
			pNode->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongUnom, pNode->GetVerbalName(), pNode->Unom));
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
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(it);
		// проверяем не самозамкнута ли ветвь
		if (pBranch->Iq == pBranch->Ip)
		{
			// если ветвь самозамкнута, выключаем ее навсегда
			pBranch->Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszBranchLooped, pBranch->Ip));
			pBranch->SetBranchState(CDynaBranch::BranchState::BRANCH_OFF, eDEVICESTATECAUSE::DSC_INTERNAL_PERMANENT);
			continue;

		}
		// нулевой коэффициент трансформации заменяем на единичный
		if (Equal(pBranch->Ktr, 0.0) || Equal(pBranch->Ktr, 1.0))
		{
			// если ветвь не трансформатор, схема замещения "П"
			pBranch->Ktr = 1.0;
			pBranch->GIp = pBranch->GIq = pBranch->G / 2.0;
			pBranch->BIp = pBranch->BIq = pBranch->B / 2.0;
		}
		else
		{
			// если трансформатор, схема замещения "Г"
			pBranch->GIp = pBranch->G;
			pBranch->BIp = pBranch->B;
			pBranch->GIq = pBranch->BIq = 0.0;
		}

		// учитываем реакторы в начале и в конце
		pBranch->GIp += pBranch->NrIp * pBranch->GrIp;
		pBranch->GIq += pBranch->NrIq * pBranch->GrIq;
		pBranch->BIp += pBranch->NrIp * pBranch->BrIp;
		pBranch->BIq += pBranch->NrIq * pBranch->BrIq;
	}

	if (!bOk)
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);
}

// строит синхронные зоны по топологии и определяет их состояния
void CDynaNodeContainer::BuildSynchroZones()
{
	if (!m_pSynchroZones)
		m_pSynchroZones = m_pDynaModel->GetDeviceContainer(eDFW2DEVICETYPE::DEVTYPE_SYNCZONE);
	if (!m_pSynchroZones)
		throw dfw2error("CDynaNodeContainer::ProcessTopology - SynchroZone container not found");

	// проверяем, есть ли отключенные узлы
	// даже не знаю что лучше для поиска первого отключенного узла...
	bool bGotOffNodes = std::find_if(m_DevVec.begin(), m_DevVec.end(), [](const auto* pNode)->bool { return !pNode->IsStateOn(); }) != m_DevVec.end();
	
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

		auto pSyncZones = m_pSynchroZones->CreateDevices<CSynchroZone>(nZonesCount); /// ???????

		if (bGotOffNodes)
		{
			// если есть отключенные узлы готовим зону для отключенных узлов
			CSynchroZone *pOffZone = pSyncZones + NodeIslands.size();
			// зону отключаем, чтобы она не попадала в систему уравнений
			pOffZone->SetState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_EXTERNAL);
			pOffZone->SetId(NodeIslands.size());
			pOffZone->SetName("SyncZone offline");
			// и ставим эту зону для всех отключенных узлов
			for (auto&& it : m_DevVec)
				if(!it->IsStateOn())
					static_cast<CDynaNodeBase*>(it)->m_pSyncZone = pOffZone;
		}

		CSynchroZone *pNewZone = pSyncZones;

		// для всех зон узлы связываем с зонами
		for (auto&& zone : NodeIslands)
		{
			// готовим место для списка генераторов
			pNewZone->m_LinkedGenerators.reserve(3 * Count());
			// придумываем идентификатор и имя
			pNewZone->SetId(pNewZone - pSyncZones + 1);
			pNewZone->SetName("SyncZone");
			for (auto&& node : zone.second)
			{
				node->m_pSyncZone = pNewZone;
				node->MarkZoneEnergized();
			}
			pNewZone++;
		}
		EnergizeZones(nDeenergizedCount, nEnergizedCount);
	}
}

// изменяет готовность узлов к включению или выключению по признаку зоны - под напряжением или без напряжения
void CDynaNodeContainer::EnergizeZones(ptrdiff_t &nDeenergizedCount, ptrdiff_t &nEnergizedCount)
{
	bool bRes = true;
	nDeenergizedCount = nEnergizedCount = 0;
	//return;
	// проходим по узлам
	for (auto&& it : m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		// если в узле есть синхронная зона
		if (!pNode->m_pSyncZone)
			throw dfw2error("CDynaNodeContainer::EnergizeZones SyncZone not assigned");
		// и она под напряжением
		if (pNode->m_pSyncZone->m_bEnergized)
		{
			// если узел отключен
			if (!pNode->IsStateOn())
			{
				// и он был отключен внутренней командной фреймворка (например был в зоне со снятым напряжением)
				if (pNode->GetStateCause() == eDEVICESTATECAUSE::DSC_INTERNAL)
				{
					// включаем узел
					pNode->SetState(eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_INTERNAL);
					pNode->Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszNodeRiseDueToZone, pNode->GetVerbalName()));
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
				pNode->SetState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
				pNode->Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszNodeTripDueToZone, pNode->GetVerbalName()));
				// считаем количество отключенных узлов
				nDeenergizedCount++;
			}
		}
	}
}

// ставит признак зоны узла - под напряжением или со снятым напряжением
void CDynaNodeBase::MarkZoneEnergized()
{
	if (!m_pSyncZone)
		throw dfw2error("CDynaNodeBase::MarkZoneEnergized SyncZone not assigned");

	if (IsStateOn())
	{
		// проходим по связям с генераторами
		CLinkPtrCount *pLink = GetLink(1);
		CDevice  **ppGen(nullptr);
		while (pLink->In(ppGen))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			// если генератор есть и он включен
			if (pGen->IsKindOfType(DEVTYPE_VOLTAGE_SOURCE) && pGen->IsStateOn())
			{
				// зону ставим под напряжение
				m_pSyncZone->m_bEnergized = true;
				// добавляем генератор в список зоны
				m_pSyncZone->m_LinkedGenerators.push_back(pGen);
				// если к тому же генератор ШБМ, то ставим признак зоны ШБМ
				if (pGen->GetType() == DEVTYPE_GEN_INFPOWER)
					m_pSyncZone->m_bInfPower = true;
				else
					// иначе считаем общий момент инерации зоны (если у генератора есть уравнение движения)
					if (pGen->IsKindOfType(DEVTYPE_GEN_MOTION))
						m_pSyncZone->Mj += static_cast<CDynaGeneratorMotion*>(pGen)->Mj;
			}
		}
	}
}


void CDynaNodeContainer::ProcessTopologyRequest()
{
	m_pDynaModel->ProcessTopologyRequest();
}

NODEISLANDMAPITRCONST CDynaNodeContainer::GetNodeIsland(CDynaNodeBase* const pNode, const NODEISLANDMAP& Islands)
{
	for (NODEISLANDMAPITRCONST it = Islands.begin() ; it != Islands.end() ; it++)
	{
		if (it->second.find(pNode) != it->second.end())
			return it;
	}
	return Islands.end();
}

void CDynaNodeContainer::DumpNodeIslands(NODEISLANDMAP& Islands)
{
	for (auto&& supernode : Islands)
	{
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszIslandOfSuperNode , supernode.first->GetVerbalName()));
		for (auto&& slavenode : supernode.second)
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, fmt::format("--> {}", slavenode->GetVerbalName()));
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

	auto FindSlack = [](const auto& itr)->bool {return itr.first->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE; };

	std::set<CDynaNodeBase*> Slacks;
	for (auto&& it : JoinableNodes)
	{
		if(FindSlack(it))
			Slacks.insert(it.first);
	}
	// вырабатываем сет заданных узлов
	while (!JoinableNodes.empty())
	{
		// ищем первый узел, предопочитаем базисный
		auto Slack = JoinableNodes.begin();
		if (!Slacks.empty())
		{
			Slack = JoinableNodes.find(*Slacks.begin());
			Slacks.erase(Slack->first);
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
	CDeviceContainer *pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	if(!pBranchContainer)
		throw dfw2error("CDynaNodeContainer::CreateSuperNodes - Branch container not found");

	ClearSuperLinks();
	// обнуляем ссылки подчиненных узлов на суперузлы
	for (auto&& nit : m_DevVec)
		static_cast<CDynaNodeBase*>(nit)->m_pSuperNodeParent = nullptr;

	NODEISLANDMAP JoinableNodes, SuperNodes;
	// строим список связности по включенным ветвям с нулевым сопротивлением
	ptrdiff_t nZeroBranchCount(0);
	for (auto&& it : *pBranchContainer)
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(it);
		// по умолчанию суперузлы ветви равны исходным узлам
		pBranch->m_pNodeSuperIp = pBranch->m_pNodeIp;
		pBranch->m_pNodeSuperIq = pBranch->m_pNodeIq;
		if (pBranch->IsZeroImpedance())
		{
			JoinableNodes[pBranch->m_pNodeIp].insert(pBranch->m_pNodeIq);
			JoinableNodes[pBranch->m_pNodeIq].insert(pBranch->m_pNodeIp);
			nZeroBranchCount += 2;	// учитываем, что ветвь может учитываться в двух узлах два раза
		}
	}
	// получаем список островов
	GetNodeIslands(JoinableNodes, SuperNodes);

	// строим связи от подчиенных узлов к суперузлам
	// готовим вариант связи с ветвями
	// в дополнительном списке связей

	DumpNodeIslands(SuperNodes);

		
	m_SuperLinks.emplace_back(this, Count()); // используем конструктор CMultiLink
	CMultiLink& pNodeSuperLink(m_SuperLinks.back());
	// заполняем список в два прохода: на первом считаем количество, на втором - заполняем ссылки
	for (int pass = 0; pass < 2; pass++)
	{
		for (auto&& nit : SuperNodes)
			for (auto&& sit : nit.second)
				if (nit.first != sit)
				{
					sit->m_pSuperNodeParent = nit.first;
					if (pass)
						AddLink(pNodeSuperLink, nit.first->m_nInContainerIndex, sit);
					else
						IncrementLinkCounter(pNodeSuperLink, nit.first->m_nInContainerIndex);
				}

		if (pass)
			RestoreLinks(pNodeSuperLink);	// на втором проходе финализируем ссылки
		else
			AllocateLinks(pNodeSuperLink);	// на первом проходe размечаем ссылки по узлам
	}

	// перестраиваем связи суперузлов с ветвями:
	m_SuperLinks.emplace_back(m_Links[0].m_pContainer, Count());
	CMultiLink& pBranchSuperLink(m_SuperLinks.back());

	// заполняем список в два прохода: на первом считаем количество, на втором - заполняем ссылки
	for (int pass = 0; pass < 2; pass++)
	{
		for (DEVICEVECTORITR it = pBranchContainer->begin(); it != pBranchContainer->end(); it++)
		{
			// учитываем только включенные ветви (по идее можно фильтровать и ветви с нулевым сопротивлением)
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);
				
			// Здесь включаем все ветви: и включенные и отключенные, иначе надо всякий раз перестраивать матрицу
			if (pBranch->m_BranchState != CDynaBranch::BranchState::BRANCH_ON)
				continue;

			CDynaNodeBase *pNodeIp(pBranch->m_pNodeIp);
			CDynaNodeBase *pNodeIq(pBranch->m_pNodeIq);

			if (pNodeIp->m_pSuperNodeParent == pNodeIq->m_pSuperNodeParent)
			{
				// суперузлы в начале и в конце одинаковые
				if (!pNodeIp->m_pSuperNodeParent)
				{
					// суперузлы отсуствуют - ветвь между двумя обычными узлами или суперузлами
					if (pass)
					{
						// на втором проходе добавляем ссылки
						AddLink(pBranchSuperLink, pNodeIp->m_nInContainerIndex, pBranch);
						AddLink(pBranchSuperLink, pNodeIq->m_nInContainerIndex, pBranch);
					}
					else
					{
						// на первом проходе считаем количество ссылок
						IncrementLinkCounter(pBranchSuperLink, pNodeIp->m_nInContainerIndex);
						IncrementLinkCounter(pBranchSuperLink, pNodeIq->m_nInContainerIndex);
					}
					//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("Branch %s connects supernodes %s and %s", pBranch->GetVerbalName(), pNodeIp->GetVerbalName(), pNodeIq->GetVerbalName()));
				}
				else
					// иначе суперузел есть - и ветвь внутри него
					pBranch->m_pNodeSuperIp = pBranch->m_pNodeSuperIq = pNodeIp->m_pSuperNodeParent;
					//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("Branch %s in super node %s", pBranch->GetVerbalName(), pNodeIp->m_pSuperNodeParent->GetVerbalName()));
			}
			else
			{
				// суперузлы в начале и в конце разные
				CDynaNodeBase *pNodeIpSuper(pNodeIp);
				CDynaNodeBase *pNodeIqSuper(pNodeIq);

				// если у узлов есть суперузел - связываем ветвь не с узлом, а с его
				// суперузлом
				if (pNodeIpSuper->m_pSuperNodeParent)
					pNodeIpSuper = pNodeIpSuper->m_pSuperNodeParent;

				if (pNodeIqSuper->m_pSuperNodeParent)
					pNodeIqSuper = pNodeIqSuper->m_pSuperNodeParent;

				pBranch->m_pNodeSuperIp = pNodeIpSuper;
				pBranch->m_pNodeSuperIq = pNodeIqSuper;

				// если суперузел у узлов ветви не общий
				if (pNodeIpSuper != pNodeIqSuper)
				{
					if (pass)
					{
						AddLink(pBranchSuperLink, pNodeIpSuper->m_nInContainerIndex, pBranch);
						AddLink(pBranchSuperLink, pNodeIqSuper->m_nInContainerIndex, pBranch);
						// в ветви нет одиночных связей, но есть два адреса узлов начала и конца.
						// их меняем на адреса суперузлов
					}
					else
					{
						IncrementLinkCounter(pBranchSuperLink, pNodeIpSuper->m_nInContainerIndex);
						IncrementLinkCounter(pBranchSuperLink, pNodeIqSuper->m_nInContainerIndex);
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

		if(pass)
			RestoreLinks(pBranchSuperLink);		// на втором проходе финализируем ссылки
		else
			AllocateLinks(pBranchSuperLink);	// на первом проходм размечаем ссылки по узлам
	}

	// перестраиваем все остальные связи кроме ветвей
	// идем по мультиссылкам узла
	for (auto&& multilink : m_Links)
	{
		// если ссылка на контейнер ветвей - пропускаем (обработали выше отдельно)
		if (multilink.m_pContainer->GetType() == DEVTYPE_BRANCH)
			continue;
		// определяем индекс ссылки один-к-одному в контейнере, с которым связаны узлы
		// для поиска индекса контейнер запрашиваем по типу связи "Узел"
		ptrdiff_t nLinkIndex = multilink.m_pContainer->GetSingleLinkIndex(DEVTYPE_NODE);

		// создаем дополнительную мультисвязь и добавляем в список связей суперузлов
		m_SuperLinks.emplace_back(multilink.m_pContainer, Count());
		CMultiLink& pSuperLink(m_SuperLinks.back());
		// для хранения оригинальных связей устройств с узлами используем карту устройство-устройство
		m_OriginalLinks.push_back(std::make_unique<DEVICETODEVICEMAP>(DEVICETODEVICEMAP()));

		// обрабатываем связь в два прохода : подсчет + выделение памяти и добавление ссылок
		for (int pass = 0; pass < 2; pass++)
		{
			// идем по узлам
			for (auto&& node : m_DevVec)
			{
				CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(node);
				CDynaNodeBase *pSuperNode = pNode;
				// если у узла есть суперузел, то берем его указатель
				// иначе узел сам себе суперузел
				if (pNode->m_pSuperNodeParent)
					pSuperNode = pNode->m_pSuperNodeParent;
				// достаем из узла мультиссылку на текущий тип связи
				CLinkPtrCount* pLink = multilink.GetLink(pNode->m_nInContainerIndex);
				CDevice **ppDevice(nullptr);
				// идем по мультиссылке
				while (pLink->In(ppDevice))
				{
					if (pass)
					{
						// добавляем к суперузлу ссылку на внешее устройство
						AddLink(pSuperLink, pSuperNode->m_nInContainerIndex, *ppDevice);
						// указатель на прежний узел в устройстве, которое сязано с узлом
						CDevice *pOldDev = (*ppDevice)->GetSingleLink(nLinkIndex);
						// заменяем ссылку на старый узел ссылкой на суперузел
						(*ppDevice)->SetSingleLink(nLinkIndex, pSuperNode);
						// сохраняем оригинальную связь устройства с узлом в карте
						m_OriginalLinks.back()->insert(std::make_pair(*ppDevice, pOldDev));
						//*
						//string strName(pOldDev ? pOldDev->GetVerbalName() : "");
						//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex("Change link of object %s from node %s to supernode %s",
						//		(*ppDevice)->GetVerbalName(),
						//		strName.c_str(),SuperNodeBlock.first->GetVerbalName()));
					}
					else
						IncrementLinkCounter(pSuperLink, pSuperNode->m_nInContainerIndex); // на первом проходе просто считаем количество связей
				}
			}

			if (pass)
				RestoreLinks(pSuperLink);	// на втором проходе финализируем ссылки
			else
				AllocateLinks(pSuperLink);	// на первом проходм размечаем ссылки по узлам
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
	m_pVirtualBranches = std::make_unique<VirtualBranch[]>(m_SuperLinks[1].m_nCount);
	// Создаем список ветвей с нулевым сопротивлением внутри суперузла
	// Ветви с нулевым сопротивлением хранятся в общем векторе контейнера
	// узлы имеют три указателя на этот вектор - начало и конец для всех ветвей
	// и указатель на список параллельных ветвей (см. TidyZeroBranches)
	m_pZeroBranches = std::make_unique<VirtualZeroBranch[]>(nZeroBranchCount);
	m_pZeroBranchesEnd = m_pZeroBranches.get() + nZeroBranchCount;

	VirtualZeroBranch* pCurrentZeroBranch = m_pZeroBranches.get();
	for (auto&& node : m_DevVec)
	{
		CDynaNodeBase* pNode = static_cast<CDynaNodeBase*>(node);
		// формируем диапазоны ветвей с нулевым сопротивлением для узлов (просто инициализация)
		pNode->m_VirtualZeroBranchBegin = pNode->m_VirtualZeroBranchEnd = pCurrentZeroBranch;
		// если у нас есть ветви с нулевым сопротивлением строим их списки
		// если узел входит в суперузел - его нулевые ветви нам не нужны, 
		// они будут учтены с узла представителя суперузла
		// если ветвей с нулевым сопротивлением нет вообще - пропускаем обработку
		// (цикл не обходим чтобы инициализировать m_VirtualZeroBranchBegin = m_VirtualZeroBranchEnd
		if (!nZeroBranchCount || pNode->m_pSuperNodeParent)
			continue;

		CDynaNodeBase* pSlaveNode = pNode;
		CLinkPtrCount* pSlaveNodeLink = pNode->GetSuperLink(0);
		CDevice** ppSlaveNode(nullptr);

		// сначала обрабатываем узел-представитель суперузла
		// затем выбираем входящие в суперузел узлы
		while (pSlaveNode)
		{
			CLinkPtrCount* pBranchLink = pSlaveNode->GetLink(0);
			CDevice** ppDevice(nullptr);
			while (pBranchLink->In(ppDevice))
			{
				CDynaBranch* pBranch = static_cast<CDynaBranch*>(*ppDevice);
				pCurrentZeroBranch = pNode->AddZeroBranch(pBranch);
			}
			// достаем из суперссылки на узлы следующий узел суперузла
			pSlaveNode = pSlaveNodeLink->In(ppSlaveNode) ? static_cast<CDynaNodeBase*>(*ppSlaveNode) : nullptr;
		}

		// приводим ссылки на нулевые ветви к нужному формату для последующей обработки в циклах
		pNode->TidyZeroBranches();
	}

	/*
	// восстановление внешних одиночных ссылок
	for (auto&& node : m_DevVec)
	{
		// для всех устройств, на которые изначально ссылался
		// узел записываем в одиночную ссылку на узел адрес узла
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(node);
		// проходим по исходным ссылкам,
		// они заменяют ссылки на суперузлы
		for (auto&& link : m_Links)
		{
			CDevice **ppDevice(nullptr);
			pNode->ResetVisited();
			CLinkPtrCount *pLink = link->GetLink(pNode->m_nInContainerIndex);
			ptrdiff_t nLinkIndex = link->m_pContainer->GetSingleLinkIndex(DEVTYPE_NODE);
			while (pLink->In(ppDevice))
				(*ppDevice)->SetSingleLink(nLinkIndex, pNode);	
		}
	}
	*/
}


void CDynaNodeContainer::CalculateSuperNodesAdmittances(bool bFixNegativeZs)
{
	// Рассчитываем проводимости узлов и ветвей только после того, как провели
	// топологический анлализ. Это позволяет определить ветви внутри суперузлов
	// которые оказались параллельны нулевым ветвям, но имеют сопротивление выше
	// порогового. Такие ветви можно определить по тому, что они отнесены к одному и 
	// тому же суперзлу. При этом не нужно искать параллельные нулевые ветви
	CalcAdmittances(bFixNegativeZs);
	VirtualBranch* pCurrentBranch = m_pVirtualBranches.get();
	for (auto&& node : m_DevVec)
	{
		CDynaNodeBase* pNode = static_cast<CDynaNodeBase*>(node);
		// если узел входит в суперузел - для него виртуальные ветви отсутствуют
		pNode->m_VirtualBranchBegin = pNode->m_VirtualBranchEnd = pCurrentBranch;
		if (pNode->m_pSuperNodeParent)
			continue;
		CLinkPtrCount* pBranchLink = pNode->GetSuperLink(1);
		pNode->ResetVisited();
		CDevice** ppDevice(nullptr);
		while (pBranchLink->In(ppDevice))
		{
			CDynaBranch* pBranch = static_cast<CDynaBranch*>(*ppDevice);
			// обходим включенные ветви также как и для подсчета размерностей выше
			CDynaNodeBase* pOppNode = pBranch->GetOppositeSuperNode(pNode);
			// получаем проводимость к оппозитному узлу
			cplx* pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
			// проверяем, уже прошли данный оппозитный узел для просматриваемого узла или нет
			ptrdiff_t DupIndex = pNode->CheckAddVisited(pOppNode);
			if (DupIndex < 0)
			{
				// если нет - добавляем ветвь в список данного узла
				pCurrentBranch->Y = *pYkm;
				pCurrentBranch->pNode = pOppNode;
				pCurrentBranch++;
			}
			else
				(pNode->m_VirtualBranchBegin + DupIndex)->Y += *pYkm; // если оппозитный узел уже прошли, ветвь не добавляем, а суммируем ее проводимость параллельно с уже пройденной ветвью
		}
		pNode->m_VirtualBranchEnd = pCurrentBranch;
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
	m_SuperLinks.clear();
	m_OriginalLinks.clear();
}

CDynaNodeBase* CDynaNodeContainer::FindGeneratorNodeInSuperNode(CDevice *pGen)
{
	CDynaNodeBase *pRet(nullptr);
	// ищем генератор в карте сохраненных связей и возвращаем оригинальный узел
	if (pGen && m_OriginalLinks.size() > 0)
	{
		const auto& GenMap = m_OriginalLinks[0];
		const auto& itGen = GenMap->find(pGen);
		if (itGen != GenMap->end())
			pRet = static_cast<CDynaNodeBase*>(itGen->second);
	}
	return pRet;
}

CLinkPtrCount* CDynaNodeBase::GetSuperLink(ptrdiff_t nLinkIndex)
{
	_ASSERTE(m_pContainer);
	return static_cast<CDynaNodeContainer*>(m_pContainer)->GetCheckSuperLink(nLinkIndex, m_nInContainerIndex).GetLink(m_nInContainerIndex);
}

CMultiLink& CDynaNodeContainer::GetCheckSuperLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex)
{
	return GetCheckLink(nLinkIndex, nDeviceIndex, m_SuperLinks);
}

_IterationControl& CDynaNodeContainer::IterationControl()
{
	return m_IterationControl;
}

std::string CDynaNodeContainer::GetIterationControlString()
{
	std::string retString = fmt::format("{:15f} {:>11} {:15f} {:>11} {:5.2f} {:>11} {:5.2f} {:>11} {:>5}",
		m_IterationControl.m_MaxImbP.GetDiff(), m_IterationControl.m_MaxImbP.GetId(),
		m_IterationControl.m_MaxImbQ.GetDiff(), m_IterationControl.m_MaxImbQ.GetId(),
		m_IterationControl.m_MaxV.GetDiff(), m_IterationControl.m_MaxV.GetId(),
		m_IterationControl.m_MinV.GetDiff(), m_IterationControl.m_MinV.GetId(),
		m_IterationControl.m_nQviolated,
		m_IterationControl.m_ImbRatio);
	return retString;
}

void CDynaNodeContainer::DumpIterationControl()
{
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, GetIterationControlString());
}

// функция готовит топологию к дианамике в первый раз
// учитывает, что суперузлы уже могли быть построены в процессе расчета УР
// функция должна повторять ProcessTopology за исключением условного вызова
// CreateSuperNodes()
void CDynaNodeContainer::ProcessTopologyInitial()
{
	// если суперузлы отсутствуют, строим суперузлы
	// если есть считаем что они построены в УР
	if(m_SuperLinks.empty())
		CreateSuperNodes();
	BuildSynchroZones();
	m_pDynaModel->RebuildMatrix(true);
}	

// функция обработки топологии, вызывается в процессе расчета динамики
void CDynaNodeContainer::ProcessTopology()
{
	CreateSuperNodes();
	BuildSynchroZones();
	m_pDynaModel->RebuildMatrix(true);
}

void CDynaNodeBase::AddToTopologyCheck()
{
	if (m_pContainer)
		static_cast<CDynaNodeContainer*>(m_pContainer)->AddToTopologyCheck(this);
}

void CDynaNodeContainer::AddToTopologyCheck(CDynaNodeBase *pNode)
{
	m_TopologyCheck.insert(pNode);
}

void CDynaNodeContainer::ResetTopologyCheck()
{
	m_TopologyCheck.clear();
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
	CDeviceContainer* pNodeContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_NODE);
	CDeviceContainer *pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	NODEISLANDMAP JoinableNodes;

	// сначала вводим в список узлов все включенные, чтобы в списке оказались все узлы, даже без связей с базой
	for (auto&& it : *pNodeContainer)
	{
		CDynaNodeBase* pNode = static_cast<CDynaNodeBase*>(it);
		if (pNode->GetState() == eDEVICESTATE::DS_ON)
			JoinableNodes.insert(std::make_pair(pNode, NodeSet()));
	}

	for (auto&& it : *pBranchContainer)
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(it);
		if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON)
		{
			JoinableNodes[pBranch->m_pNodeIp].insert(pBranch->m_pNodeIq);
			JoinableNodes[pBranch->m_pNodeIq].insert(pBranch->m_pNodeIp);
		}
	}
	// формируем перечень островов
	GetNodeIslands(JoinableNodes, NodeIslands);
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszIslandCount, NodeIslands.size()));
}

void CDynaNodeContainer::PrepareLFTopology()
{
	NodeSet Queue;
	// проверяем все узлы на соответствие состояния их самих и инцидентных ветвей
	for (auto&& node : m_DevVec)
		SwitchOffDanglingNode(static_cast<CDynaNodeBase*>(node), Queue);
	// отключаем узлы попавшие в очередь отключения
	SwitchOffDanglingNodes(Queue);
	
	NODEISLANDMAP NodeIslands;
	GetTopologySynchroZones(NodeIslands);

	for (auto&& island : NodeIslands)
	{
		// для каждого острова считаем количество базисных узлов
		ptrdiff_t nSlacks = std::count_if(island.second.begin(), island.second.end(), [](CDynaNodeBase* pNode) -> bool { 
				return pNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE;
		});
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszIslandSlackBusesCount, island.second.size(), nSlacks));
		if (!nSlacks)
		{
			// если базисных узлов в острове нет - отключаем все узлы острова
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszIslandNoSlackBusesShutDown));
			NodeSet& Queue = island.second;
			for (auto&& node : Queue)
			{
				m_pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszNodeShutDownAsNotLinkedToSlack, node->GetVerbalName()));
				node->ChangeState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
			}
			// и отключаем ветви
			SwitchOffDanglingNodes(Queue);
		}
	}
}

void CDynaNodeContainer::SwitchOffDanglingNode(CDynaNodeBase *pNode, NodeSet& Queue)
{
	CLinkPtrCount *pLink = pNode->GetLink(0);
	CDevice **ppDevice(nullptr);
	if (pNode->GetState() == eDEVICESTATE::DS_ON)
	{
		// проверяем есть ли хотя бы одна включенная ветвь к этому узлу
		while (pLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			if(pBranch->BranchAndNodeConnected(pNode))
				break; // ветвь есть
		}
		if (!ppDevice)
		{
			// все ветви отключены, отключаем узел
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszSwitchedOffNode, pNode->GetVerbalName()));
			// сбрасываем список проверки инцидентных узлов
			ResetTopologyCheck();
			// используем функцию ChangeState(), которая отключает все, что отнесено к отключаемому узлу, в том числе и ветви
			pNode->ChangeState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
			// и этот узел ставим в очередь на повторную проверку отключения ветвей
			Queue.insert(pNode);
			// в очередь также ставим все узлы, которые были инцидентны данному и состояния ветвей до которых изменились
			Queue.insert(m_TopologyCheck.begin(), m_TopologyCheck.end());
			// после ChangeState ветви, инцидентные узлу изменят состояние, поэтому надо проверить узлы на концах изменивших состояние ветвей
		}
	}
	else
	{
		// отключаем все ветви к этому узлу 
		while (pLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			if (pBranch->DisconnectBranchFromNode(pNode))
			{
				// состояние ветви изменилось, узел с другой стороны ставим в очередь на проверку
				CDynaNodeBase* pOppNode = pBranch->GetOppositeNode(pNode);
				if (pOppNode->GetState() == eDEVICESTATE::DS_ON)
					Queue.insert(pOppNode);
			}
		}
	}
}

void CDynaNodeContainer::DumpNetwork()
{
	std::ofstream dump(stringutils::utf8_decode(fmt::format("c:\\tmp\\network-{}.net", GetModel()->GetStepNumber())));
	if (dump.is_open())
	{
		for (auto& node : m_DevVec)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(node);
			dump << fmt::format("Node Id={} DBIndex={} V {} / {}", pNode->GetId(), pNode->GetDBIndex(), pNode->V.Value, pNode->Delta.Value / M_PI * 180.0);
			if(pNode->m_pSuperNodeParent)
				dump << fmt::format(" belongs to supernode Id={}", pNode->m_pSuperNodeParent->GetId());
			dump << std::endl;

			CLinkPtrCount* pBranchLink = pNode->GetLink(0);
			CDevice** ppDevice(nullptr);
			while (pBranchLink->In(ppDevice))
			{
				CDynaBranch* pBranch = static_cast<CDynaBranch*>(*ppDevice);
				dump << fmt::format("\tOriginal Branch %{}-%{}-({}) r={} x={} state={}",
					pBranch->Ip, pBranch->Iq, pBranch->Np,
					pBranch->R, pBranch->X, pBranch->m_BranchState) << std::endl;
			}

			CLinkPtrCount* pSuperNodeLink = pNode->GetSuperLink(0);
			ppDevice = nullptr;
			while (pSuperNodeLink->In(ppDevice))
			{
				CDynaNodeBase* pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
				dump << fmt::format("\t\tSlave Node Id={} DBIndex={}", pSlaveNode->GetId(), pSlaveNode->GetDBIndex()) << std::endl;

				CLinkPtrCount* pBranchLink = pSlaveNode->GetLink(0);
				CDevice** ppDeviceBranch(nullptr);
				while (pBranchLink->In(ppDeviceBranch))
				{
					CDynaBranch* pBranch = static_cast<CDynaBranch*>(*ppDeviceBranch);
					dump << fmt::format("\t\t\tOriginal Branch %{}-%{}-({}) r={} x={} state={}", 
						pBranch->Ip, pBranch->Iq, pBranch->Np, 
						pBranch->R, pBranch->X, pBranch->m_BranchState) << std::endl;
				}
			}

			for (VirtualBranch* pV = pNode->m_VirtualBranchBegin; pV < pNode->m_VirtualBranchEnd; pV++)
				dump << fmt::format("\tVirtual Branch to node Id={}", pV->pNode->GetId()) << std::endl;
		}
	}
	std::ofstream lulf(stringutils::utf8_decode(fmt::format("c:\\tmp\\nodes LULF-{}.csv", GetModel()->GetStepNumber())));
	if (lulf.is_open())
	{
		for (auto& node : m_DevVec)
		{
			CDynaNodeBase* pNode = static_cast<CDynaNodeBase*>(node);
			lulf << fmt::format("{};{};{}", pNode->GetId(), pNode->V.Value, pNode->Delta.Value / M_PI * 180.0) << std::endl;
		}
	}
}