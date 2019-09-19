#include "stdafx.h"
#include "DynaModel.h"
#include "queue"
#include "stack"


using namespace DFW2;

bool CDynaModel::Link()
{
	bool bRes = true;

	// Prepare separate links to master device to help further CDevice::MastersReady
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		CDeviceContainerProperties &Props = (*it)->m_ContainerProps;

		LINKSTOMAP	 &LinksTo = Props.m_LinksTo;
		for (LINKSTOMAPITR it1 = LinksTo.begin(); it1 != LinksTo.end(); it1++)
			if (it1->second.eDependency == DPD_MASTER)
				if(it1->first != DEVTYPE_MODEL)
					Props.m_MasterLinksTo.insert(make_pair(it1->first, it1->second));

		LINKSFROMMAP &LinksFrom = Props.m_LinksFrom;
		for (LINKSFROMMAPITR it2 = LinksFrom.begin(); it2 != LinksFrom.end(); it2++)
			if (it2->second.eDependency == DPD_MASTER)
				if(it2->first != DEVTYPE_MODEL)
					Props.m_MasterLinksFrom.insert(make_pair(it2->first, it2->second));
	}


	// линкуем всех со всеми по матрице
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)			// внешний контейнер
	{
		for (DEVICECONTAINERITR lt = m_DeviceContainers.begin(); lt != m_DeviceContainers.end(); lt++)		// внутренний контейнер
		{
			// не линкуем тип с этим же типом
			if (it != lt)
			{
				LinkDirectionFrom LinkFrom;
				LinkDirectionTo LinkTo;
				// проверяем возможность связи внешнего контейнера со внутренним
				CDeviceContainer *pContLead = (*it)->DetectLinks(*lt, LinkTo, LinkFrom);
				if (pContLead == *lt)
				{
					// если выбран внутренний контейнер
					Log(DFW2::CDFW2Messages::DFW2LOG_INFO, _T("Связь %s <- %s"), (*it)->GetTypeName(), (*lt)->GetTypeName());
					// организуем связь внешгего контейнера с внутренним
					bRes = (*it)->CreateLink(*lt) && bRes;
					_ASSERTE(bRes);
				}
			}
		}
	}


	/*
	bRes = Nodes.CreateLink(&Branches) && bRes;
	bRes = Nodes.CreateLink(&GeneratorsMustang) && bRes;
	bRes = Nodes.CreateLink(&Generators3C) && bRes;
	bRes = Nodes.CreateLink(&Generators1C) && bRes;
	bRes = Nodes.CreateLink(&GeneratorsInfBus) && bRes;
	bRes = Nodes.CreateLink(&GeneratorsMotion) && bRes;
	bRes = Generators1C.CreateLink(&ExcitersMustang) && bRes;
	bRes = Generators3C.CreateLink(&ExcitersMustang) && bRes;
	bRes = GeneratorsMustang.CreateLink(&ExcitersMustang) && bRes;
	bRes = ExcitersMustang.CreateLink(&DECsMustang) && bRes;
	bRes = ExcitersMustang.CreateLink(&ExcConMustang) && bRes;
	//bRes = ExcitersMustang.CreateLink(&CustomDevice) && bRes;
	*/


	// по атрибутам контейнеров формируем отдельные списки контейнеров для
	// обновления после итерации Ньютона и после прогноза, чтобы не проверять эти атрибуты в основных циклах
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		if ((*it)->m_ContainerProps.bNewtonUpdate)
			m_DeviceContainersNewtonUpdate.push_back(*it);
		if ((*it)->m_ContainerProps.bPredict)
			m_DeviceContainersPredict.push_back(*it);
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

	if (Count() && m_Links.size() > 0)
	{
		CDynaNodeBase *pDynaNode = GetFirstNode();

		if (pDynaNode)
		{
			pDynaNode->m_nZoneDistance = 0;

			// используем BFS для обхода сети
			std::queue<CDynaNodeBase*>  Queue;
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

bool CDynaNodeContainer::GetNodeIslands(NODEISLANDMAP& JoinableNodes, NODEISLANDMAP& Islands)
{
	bool bRes(true);
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
		CDynaNodeBase *pSeed = JoinableNodes.begin()->first;
		Stack.push(pSeed);
		// вставляем первый узел как основу для острова
		auto& CurrentSuperNode = Islands.insert(make_pair(pSeed, set<CDynaNodeBase*>{}));

		// делаем стандартный DFS
		while (!Stack.empty())
		{
			CDynaNodeBase *pCurrentSeed = Stack.top();
			// к текущему острову добавляем найденный узел
			CurrentSuperNode.first->second.insert(pCurrentSeed);
			Stack.pop();

			// проверяем, есть ли найденный узел во входном сете
			auto& jit = JoinableNodes.find(pCurrentSeed);
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

	for (auto&& nit : Islands)
	{
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Supernode %d"), nit.first->GetId()));
		for (auto&& mit : nit.second)
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("\tSlave %d"), mit->GetId()));
	}

	return bRes;
}

bool CDynaNodeContainer::CreateSuperNodes()
{
	bool bRes = true;
	CDeviceContainer *pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);

	for (auto&& nit : m_DevVec)
		static_cast<CDynaNodeBase*>(nit)->m_pSuperNodeParent = nullptr;

	NODEISLANDMAP JoinableNodes, SuperNodes;

	for (DEVICEVECTORITR it = pBranchContainer->begin(); it != pBranchContainer->end(); it++)
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);
		if (pBranch->R < 0.1 && pBranch->X < 0.1)
		{
			JoinableNodes[pBranch->m_pNodeIp].insert(pBranch->m_pNodeIq);
			JoinableNodes[pBranch->m_pNodeIq].insert(pBranch->m_pNodeIp);
		}
	}

	bRes = GetNodeIslands(JoinableNodes, SuperNodes);
	if (bRes)
	{
		for (DEVICEVECTORITR it = pBranchContainer->begin(); it != pBranchContainer->end(); it++)
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);
			auto it1 = GetNodeIsland(pBranch->m_pNodeIp, SuperNodes);
			auto it2 = GetNodeIsland(pBranch->m_pNodeIq, SuperNodes);

			if (it1 == it2)
			{
				if (it1 != SuperNodes.end())
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Branch %s in super node %s"), pBranch->GetVerbalName(), it1->first->GetVerbalName()));
			}
			else
			{
				if(it1 == SuperNodes.end())
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Branch %s to supernode %s"), pBranch->GetVerbalName(), it2->first->GetVerbalName()));
				else if(it2 == SuperNodes.end())
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Branch %s from supernode %s"), pBranch->GetVerbalName(), it1->first->GetVerbalName()));
				else
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Branch %s connects supernodes %s and %s"), pBranch->GetVerbalName(), it1->first->GetVerbalName(), it2->first->GetVerbalName()));
			}
		}

		// идем по мультиссылкам узла
		for (auto&& multilink : m_Links)
		{
			// определяем индекс ссылки один-к-одному в контейнере, с которым связаны узлы
			// для поиска индекса контейнер запрашиваем по типу связи "Узел"
			ptrdiff_t nLinkIndex = multilink->m_pContainer->GetSingleLinkIndex(DEVTYPE_NODE);
			// идем по суперузлам
			for (auto&& SuperNodeBlock : SuperNodes)
			{
				// идем по входящим в суперузлы узлам
				for (auto&& SlaveNode : SuperNodeBlock.second)
				{
					// достаем из узла мультиссылку на текущий тип связи
					CLinkPtrCount *pLink = &multilink->m_pLinkInfo[SlaveNode->m_nInContainerIndex];
					SlaveNode->ResetVisited();
					CDevice **ppDevice(nullptr);
					// идем по мультиссылке
					while (pLink->In(ppDevice))
					{
						// указатель на прежний узел в устройстве, которое сязано с узлом
						CDevice *pOldDev = (*ppDevice)->GetSingleLink(nLinkIndex);
						// заменяем ссылку на старый узел ссылкой на суперузел
						(*ppDevice)->SetSingleLink(nLinkIndex, SuperNodeBlock.first);
						wstring strName(pOldDev ? pOldDev->GetVerbalName() : _T(""));
						m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, Cex(_T("Change link of object %s from node %s to supernode %s"), 
								(*ppDevice)->GetVerbalName(), 
								strName.c_str(),SuperNodeBlock.first->GetVerbalName()));
					}
				}
			}
		}
	}

	return bRes;
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

	m_bRebuildMatrix = false;

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

void CDynaNodeBase::ProcessTopologyRequest()
{
	if (m_pContainer)
		static_cast<CDynaNodeContainer*>(m_pContainer)->ProcessTopologyRequest();
}
