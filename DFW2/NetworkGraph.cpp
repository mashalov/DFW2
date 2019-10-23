﻿#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;
bool CDynaModel::Link()
{
	bool bRes = true;

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
					Props.m_MasterLinksTo.insert(make_pair(it1.first, &it1.second));	// ведущие к
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
					Props.m_MasterLinksFrom.insert(make_pair(it2.first, &it2.second));	// ведущие от
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
					Log(DFW2::CDFW2Messages::DFW2LOG_INFO, _T("Связь %s <- %s"), it->GetTypeName(), lt->GetTypeName());
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

bool CDynaModel::PrepareGraph()
{
	bool bRes = false;
	DEVICEVECTORITR it;

	// сбрасываем обработку топологии
	// после импорта данных, в которых могли быть отключенные узлы и ветви
	sc.m_bProcessTopology = false;

	
	if (!(m_pLRCGen = static_cast<CDynaLRC*>(LRCs.GetDevice(-1))))
	{
		Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszMustBeConstPowerLRC));
			bRes = false;
	}

	if (Branches.Count() && Nodes.Count())
	{
		bRes = true;
		for (it = Branches.begin(); it != Branches.end(); it++)
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);

			// check branch is looped
			if (pBranch->Iq != pBranch->Ip)
			{
				// fix transformer ratios to 1.0, allocate admittances to Gamma or Pi transformer circuits
				if (Equal(pBranch->Ktr,0.0) || Equal(pBranch->Ktr,0.0))
				{
					pBranch->Ktr = 1.0;
					pBranch->GIp = pBranch->GIq = pBranch->G / 2.0;
					pBranch->BIp = pBranch->BIq = pBranch->B / 2.0;
				}
				else
				{
					pBranch->GIp = pBranch->G;
					pBranch->BIp = pBranch->B;
					pBranch->GIq = pBranch->BIq = 0.0;
				}

				// account for reactors
				pBranch->GIp += pBranch->NrIp * pBranch->GrIp;
				pBranch->GIq += pBranch->NrIq * pBranch->GrIq;
				pBranch->BIp += pBranch->NrIp * pBranch->BrIp;
				pBranch->BIq += pBranch->NrIq * pBranch->BrIq;
			}
			else
			{
				pBranch->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszBranchLooped, pBranch->Ip));
				pBranch->m_BranchState = CDynaBranch::BRANCH_OFF;
			}
		}

		if (bRes)
		{
			// also add reactor's admittance
			for (it = Nodes.begin(); it != Nodes.end(); it++)
			{
				CDynaNode *pNode = static_cast<CDynaNode*>(*it);
				pNode->G += pNode->Gr0 * pNode->Nr;
				pNode->B += pNode->Br0 * pNode->Nr;
				pNode->Gshunt = pNode->Bshunt = 0.0;
			}
		}
	}
	return bRes;
}


bool CDynaModel::PrepareYs()
{
	bool bRes = true;
	DEVICEVECTORITR it;

	for (it = Branches.begin(); it != Branches.end(); it++)
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);
		pBranch->CalcAdmittances();
	}

	for (it = Nodes.begin(); it != Nodes.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		pNode->CalcAdmittances();
	}

	return bRes;
}

// возвращает первый узел для поиска зон
CDynaNodeBase* CDynaNodeContainer::GetFirstNode()
{
	CDynaNodeBase *pDynaNode(nullptr);
	for (DEVICEVECTORITR it = begin(); it != end(); it++)
	{
		// обходим узлы, сбрасываем дистанции зон
		CDynaNodeBase *pDynaNodeTest = static_cast<CDynaNode*>(*it);
		pDynaNodeTest->m_nZoneDistance = -1;
		// определяем первый узел
		if (!pDynaNode)
			pDynaNode = pDynaNodeTest;
	}

	if (pDynaNode)
	{
		if (!pDynaNode->m_pSyncZone)
		{
			// если у узла нет зоны, создаем ее
			pDynaNode->m_pSyncZone = CreateNewSynchroZone();
		}
		else
		{
			// если есть - очищаем данные зоны
			pDynaNode->m_pSyncZone->Clear();
		}
	}
	return pDynaNode;
}

// возвращает следующий узел для построения зон
CDynaNodeBase* CDynaNodeContainer::GetNextNode()
{
	CDynaNodeBase *pDynaNode(nullptr);
	for (DEVICEVECTORITR it = begin(); it != end(); it++)
	{
		// ищем узлы без дистанции до зоны
		CDynaNodeBase *pCheckNode = static_cast<CDynaNode*>(*it);
		if (pCheckNode->m_nZoneDistance < 0)
		{
			pDynaNode = pCheckNode;
			pDynaNode->m_nZoneDistance = 0;
			if (!pDynaNode->m_pSyncZone)
			{
				// если у узла нет зоны - создаем
				pDynaNode->m_pSyncZone = CreateNewSynchroZone();
			}
			else
			{
				// если есть, проверяем, уже обработали эту зону или нет
				if (pDynaNode->m_pSyncZone->m_bPassed)
				{
					// узел находится в уже обработанной зоне 
					// он был в ней, но при обработке зоны не получил дистанции, значит - теперь должен быть в другой
					// поэтому создаем для узла новую зону
					pDynaNode->m_pSyncZone = CreateNewSynchroZone();
				}
				else
				{
					// если нет - сбрасываем параметры зоны, зона при этом будет считаться обработанной
					pDynaNode->m_pSyncZone->Clear();
				}
			}
			break;
		}
	}
	return pDynaNode;
}

// создает новую зону
CSynchroZone* CDynaNodeContainer::CreateNewSynchroZone()
{
	CSynchroZone *pNewSZ = new CSynchroZone();
	pNewSZ->Clear();
	// готовим место для списка генераторов
	pNewSZ->m_LinkedGenerators.reserve(3 * Count());
	// придумываем идентификатор и имея
	pNewSZ->SetId(m_pSynchroZones->Count());
	pNewSZ->SetName(_T("Синхрозона"));
	// добавляем в контейнер синхронных зон
	m_pSynchroZones->AddDevice(pNewSZ);
	// обязательно взводим флаг перестроения матрицы
	m_bRebuildMatrix = true;
	return pNewSZ;
}

// строит синхронные зоны по топологии и определяет их состояния
bool CDynaNodeContainer::BuildSynchroZones()
{
	bool bRes = false;

	NodeQueue Queue;

	if (Count() && m_Links.size() > 0)
	{
		CDynaNodeBase *pDynaNode = GetFirstNode();

		if (pDynaNode)
		{
			pDynaNode->m_nZoneDistance = 0;

			// используем BFS для обхода сети
			bRes = true;

			while (pDynaNode)
			{
				Queue.push(pDynaNode);
				while (!Queue.empty())
				{
					CDynaNodeBase *pCurrent = Queue.front();
					// определяем состояние зоны по текущему узлу
					pCurrent->MarkZoneEnergized();
					Queue.pop();
					// обходим инцидентные ветви текущего узла
					CDevice **ppBranch(nullptr);
					CLinkPtrCount *pLink = pCurrent->GetLink(0);
					pCurrent->ResetVisited();
					while (pLink->In(ppBranch))
					{
						CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
						// can reach incident node in case branch is on only, not tripped from begin or end
						if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
						{
							// определяем инцидентный узел
							CDynaNodeBase *pn = pBranch->m_pNodeIp == pCurrent ? pBranch->m_pNodeIq : pBranch->m_pNodeIp;
							if (pn->m_nZoneDistance == -1)
							{
								// если дистанции до зоны у инцидентного узла нет - ставим ее по текущему узлу
								pn->m_nZoneDistance = pCurrent->m_nZoneDistance + 1;
								// если зоны текущего и инцидентного не совпадают - надо перестраивать матрицу
								if (pn->m_pSyncZone)
									if (pn->m_pSyncZone != pCurrent->m_pSyncZone)
										m_bRebuildMatrix = true;

								// ставим зону инцидентному узлу
								pn->m_pSyncZone = pCurrent->m_pSyncZone;
								// сбрасываем инцидентный узел в очередь поиска
								Queue.push(pn);
							}
						}

					}
				}
				pDynaNode = GetNextNode();
			}
		}
		else
			Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszAllNodesOff));
	}
	return bRes;
}

// изменяет готовность узлов к включению или выключению по признаку зоны - под напряжением или без напряжения
bool CDynaNodeContainer::EnergizeZones(ptrdiff_t &nDeenergizedCount, ptrdiff_t &nEnergizedCount)
{
	bool bRes = true;
	nDeenergizedCount = nEnergizedCount = 0;
	// проходим по узлам
	for (DEVICEVECTORITR it = begin(); it != end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		// если в узле есть синхронная зона
		if (pNode->m_pSyncZone)
		{
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
						pNode->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszNodeRiseDueToZone, pNode->GetVerbalName()));
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
					pNode->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszNodeTripDueToZone, pNode->GetVerbalName()));
					// считаем количество отключенных узлов
					nDeenergizedCount++;
				}
			}
		}
		else
			bRes = false;
	}
	return bRes;
}

// ставит признак зоны узла - под напряжением или со снятым напряжением
void CDynaNodeBase::MarkZoneEnergized()
{
	if (m_pSyncZone)
	{
		// проходим по связям с генераторами
		CLinkPtrCount *pLink = GetLink(1);
		if (pLink)
		{
			CDevice  **ppGen(nullptr);
			ResetVisited();
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
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(CDFW2Messages::m_cszIslandOfSuperNode , supernode.first->GetVerbalName()));
		for (auto&& slavenode : supernode.second)
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("--> %s"), slavenode->GetVerbalName()));
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
	stack<CDynaNodeBase*> Stack;

	// вырабатываем сет заданных узлов
	while (!JoinableNodes.empty())
	{
		// берем первый узел из сета и готовим к DFS
		Stack.push(JoinableNodes.begin()->first);
		// вставляем первый узел как основу для острова
		auto& CurrentSuperNode = Islands.insert(make_pair(JoinableNodes.begin()->first, set<CDynaNodeBase*>{}));

		// делаем стандартный DFS
		while (!Stack.empty())
		{
			// к текущему острову добавляем найденный узел
			CurrentSuperNode.first->second.insert(Stack.top());
			// проверяем, есть ли найденный узел во входном сете
			auto& jit = JoinableNodes.find(Stack.top());
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

bool CDynaNodeContainer::CreateSuperNodes()
{
	bool bRes = true;
	CDeviceContainer *pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);

	ClearSuperLinks();
	// обнуляем ссылки подчиненных узлов на суперузлы
	for (auto&& nit : m_DevVec)
		static_cast<CDynaNodeBase*>(nit)->m_pSuperNodeParent = nullptr;

	NODEISLANDMAP JoinableNodes, SuperNodes;
	// строим список связности по включенным ветвям с нулевым сопротивлением
	for (DEVICEVECTORITR it = pBranchContainer->begin(); it != pBranchContainer->end(); it++)
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);
		pBranch->m_pNodeSuperIp = pBranch->m_pNodeIp;
		pBranch->m_pNodeSuperIq = pBranch->m_pNodeIq;
		if (pBranch->IsZeroImpedance())
		{
			JoinableNodes[pBranch->m_pNodeIp].insert(pBranch->m_pNodeIq);
			JoinableNodes[pBranch->m_pNodeIq].insert(pBranch->m_pNodeIp);
		}
	}
	// получаем список островов
	GetNodeIslands(JoinableNodes, SuperNodes);

	if (bRes)
	{
		// строим связи от подчиенных узлов к суперузлам
		// готовим вариант связи с ветвями
		// в дополнительном списке связей

		DumpNodeIslands(SuperNodes);

		
		m_SuperLinks.emplace_back(this, Count());
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
				if (pBranch->m_BranchState != CDynaBranch::BRANCH_ON)
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
						//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Branch %s connects supernodes %s and %s"), pBranch->GetVerbalName(), pNodeIp->GetVerbalName(), pNodeIq->GetVerbalName()));
					}
					//else
						// иначе суперузел есть - и ветвь внутри него
						//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Branch %s in super node %s"), pBranch->GetVerbalName(), pNodeIp->m_pSuperNodeParent->GetVerbalName()));
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

					// если суперузел у узлов ветви не общий
					if (pNodeIpSuper != pNodeIqSuper)
					{
						if (pass)
						{
							AddLink(pBranchSuperLink, pNodeIpSuper->m_nInContainerIndex, pBranch);
							AddLink(pBranchSuperLink, pNodeIqSuper->m_nInContainerIndex, pBranch);
							// в ветви нет одиночных связей, но есть два адреса узлов начала и конца.
							// их меняем на адреса суперузлов
							pBranch->m_pNodeSuperIp = pNodeIpSuper;
							pBranch->m_pNodeSuperIq = pNodeIqSuper;
						}
						else
						{
							IncrementLinkCounter(pBranchSuperLink, pNodeIpSuper->m_nInContainerIndex);
							IncrementLinkCounter(pBranchSuperLink, pNodeIqSuper->m_nInContainerIndex);
						}
					}
					/*
					const _TCHAR *cszSuper = _T("super");
					const _TCHAR *cszSlave = _T("super");
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Branch %s connects %snode %s and %s node %s"),
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
			m_OriginalLinks.push_back(make_unique<DEVICETODEVICEMAP>(DEVICETODEVICEMAP()));

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
					node->ResetVisited();
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
							m_OriginalLinks.back()->insert(make_pair(*ppDevice, pOldDev));
							//*
							//wstring strName(pOldDev ? pOldDev->GetVerbalName() : _T(""));
							//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Change link of object %s from node %s to supernode %s"),
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
	}

	/*
	for (auto&& node : m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(node);
		if (pNode->m_pSuperNodeParent || pNode->GetState() != eDEVICESTATE::DS_ON)
			continue;
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("%snode %s"), pNode->m_pSuperNodeParent ? _T(""): _T("super"), pNode->GetVerbalName()));

		for (auto&& superlink : m_SuperLinks)
		{
			CDevice **ppDevice(nullptr);
			pNode->ResetVisited();
			CLinkPtrCount *pLink = superlink->GetLink(pNode->m_nInContainerIndex);
			while (pLink->In(ppDevice))
			{
				m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("\tchild %s"), (*ppDevice)->GetVerbalName()));
			}
		}
	}
	*/

	for (auto&& node : m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(node);
		pNode->YiiSuper = pNode->Yii;
		CLinkPtrCount *pLink = m_SuperLinks[0].GetLink(node->m_nInContainerIndex);
		if (pLink->m_nCount)
		{
			CDevice **ppDevice(nullptr);
			pNode->ResetVisited();
			while (pLink->In(ppDevice))
			{
				CDynaNodeBase *pSlaveNode(static_cast<CDynaNodeBase*>(*ppDevice));
				pNode->YiiSuper += pSlaveNode->Yii;
			}
		}
	}

	//  Создаем виртуальные ветви
	// Количество виртуальных ветвей не превышает количества ссылок суперузлов на ветви
	m_pVirtualBranches = new VirtualBranch[m_SuperLinks[1].m_nCount];
	VirtualBranch *pCurrentBranch = m_pVirtualBranches;
	for (auto&& node : m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(node);
		CLinkPtrCount *pBranchLink = pNode->GetSuperLink(1);
		pNode->ResetVisited();
		CDevice **ppDevice = nullptr;
		pNode->m_VirtualBranchBegin = pCurrentBranch;
		while (pBranchLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			// обходим включенные ветви также как и для подсчета размерностей выше
			CDynaNodeBase *pOppNode = pBranch->GetOppositeSuperNode(pNode);
			// получаем проводимость к оппозитному узлу
			cplx *pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
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

	return bRes;
}

void CDynaNodeContainer::ClearSuperLinks()
{
	if (m_pVirtualBranches)
	{
		delete m_pVirtualBranches;
		m_pVirtualBranches = nullptr;
	}
	m_SuperLinks.clear();
	m_OriginalLinks.clear();
}

CDynaNodeBase* CDynaNodeContainer::FindGeneratorNodeInSuperNode(CDevice *pGen)
{
	CDynaNodeBase *pRet(nullptr);
	// ищем генератор в карте сохраненных связей и возвращаем оригинальный узел
	if (pGen && m_OriginalLinks.size() > 0)
	{
		auto& GenMap = m_OriginalLinks[0];
		auto& itGen = GenMap->find(pGen);
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

void CDynaNodeContainer::DumpIterationControl()
{
	// p q minv maxv
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, _T("%20g %6d %20g %6d %10g %6d %10g %6d %4d %10g"),
	m_IterationControl.m_MaxImbP.GetDiff(), m_IterationControl.m_MaxImbP.GetId(),
	m_IterationControl.m_MaxImbQ.GetDiff(), m_IterationControl.m_MaxImbQ.GetId(),
	m_IterationControl.m_MaxV.GetDiff(), m_IterationControl.m_MaxV.GetId(),
	m_IterationControl.m_MinV.GetDiff(), m_IterationControl.m_MinV.GetId(),
	m_IterationControl.m_nQviolated,
	m_IterationControl.m_ImbRatio);
}


bool CDynaNodeContainer::ProcessTopology()
{
	bool bRes = false;

	if (!m_pSynchroZones)
		m_pSynchroZones = m_pDynaModel->GetDeviceContainer(eDFW2DEVICETYPE::DEVTYPE_SYNCZONE);
	if (!m_pSynchroZones)
		return bRes;

	CreateSuperNodes();

	// unpass existing zones
	for (DEVICEVECTORITR it = m_pSynchroZones->begin(); it != m_pSynchroZones->end(); it++)
		static_cast<CSynchroZone*>(*it)->m_bPassed = false;

	ptrdiff_t nEnergizedCount = 1;
	ptrdiff_t nDeenergizedCount = 1;
	bRes = true;

	m_bRebuildMatrix = true;

	while ((nEnergizedCount || nDeenergizedCount) && bRes)
	{
		bRes = bRes && BuildSynchroZones();
		bRes = bRes && EnergizeZones(nDeenergizedCount, nEnergizedCount);
	}

	// find and delete existing unused zones

	DEVICEVECTORITR it = m_pSynchroZones->begin();
	size_t i = 0;
	while (i < m_pSynchroZones->Count())
	{
		CSynchroZone *pZone = static_cast<CSynchroZone*>(*(m_pSynchroZones->begin() + i));
		if (!pZone->m_bPassed)
		{
			m_pSynchroZones->RemoveDeviceByIndex(i);
			m_bRebuildMatrix = true;
		}
		i++;
	}

	m_pDynaModel->RebuildMatrix(m_bRebuildMatrix);

	return bRes;
}

void CDynaNodeContainer::SwitchOffDanglingNodes(NodeSet& Queue)
{
	// обрабатываем узлы до тех пор, пока есть отключения узлов при проверке
	while (!Queue.empty())
	{
		auto& it = Queue.begin();
		CDynaNodeBase *pNextNode = *it;
		Queue.erase(it);
		SwitchOffDanglingNode(pNextNode, Queue);
	}
}

void CDynaNodeContainer::PrepareLFTopology()
{
	NodeSet Queue;
	// проверяем все узлы на соответствие состояния их самих и инцидентных ветвей
	for (auto&& node : m_DevVec)
		SwitchOffDanglingNode(static_cast<CDynaNodeBase*>(node), Queue);
	// отключаем узлы попавшие в очередь отключения
	SwitchOffDanglingNodes(Queue);
	
	// формируем список связности узлов по включенным ветвям
	CDeviceContainer *pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	NODEISLANDMAP JoinableNodes, NodeIslands;
	for (DEVICEVECTORITR it = pBranchContainer->begin(); it != pBranchContainer->end(); it++)
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);
		if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON)
		{
			JoinableNodes[pBranch->m_pNodeIp].insert(pBranch->m_pNodeIq);
			JoinableNodes[pBranch->m_pNodeIq].insert(pBranch->m_pNodeIp);
		}
	}
	// формируем перечень островов
	GetNodeIslands(JoinableNodes, NodeIslands);
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszIslandCount, NodeIslands.size()));
	for (auto&& island : NodeIslands)
	{
		// для каждого острова считаем количество базисных узлов
		ptrdiff_t nSlacks = std::count_if(island.second.begin(), island.second.end(), [](CDynaNodeBase* pNode) -> bool { 
				return pNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE;
		});
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszIslandSlackBusesCount, island.second.size(), nSlacks));
		if (!nSlacks)
		{
			// если базисных узлов в острове нет - отключаем все узлы острова
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszIslandNoSlackBusesShutDown));
			NodeSet& Queue = island.second;
			for (auto&& node : Queue)
				static_cast<CDynaNodeBase*>(node)->SetState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
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
			if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON)
				break; // ветвь есть
		}
		if (!ppDevice)
		{
			// все ветви отключены, отключаем узел
			pNode->SetState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
			// и этот узел ставим в очередь на повторную проверку отключения ветвей
			Queue.insert(pNode);
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszSwitchedOffNode, pNode->GetVerbalName()));
		}
	}
	else
	{
		// отключаем все ветви к этому узлу 
		while (pLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			if (pBranch->m_BranchState != CDynaBranch::BranchState::BRANCH_OFF)
			{
				// к отключенному узлу подходит неотключенная ветвь - отключаем
				pBranch->m_BranchState = CDynaBranch::BRANCH_OFF;
				// и узел с другой стороны ставим в очередь на повторную проверку, если он не отключен
				CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
				if(pOppNode->GetState() == eDEVICESTATE::DS_ON)
					Queue.insert(pOppNode);
				m_pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszSwitchedOffBranch, pBranch->GetVerbalName(), pNode->GetVerbalName()));
			}
		}
	}
}