#include "stdafx.h"
#include "DynaModel.h"
#include "queue"


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


CDynaNodeBase* CDynaNodeContainer::GetFirstNode()
{
	CDynaNodeBase *pDynaNode = NULL;
	for (DEVICEVECTORITR it = begin(); it != end(); it++)
	{
		CDynaNodeBase *pDynaNodeTest = static_cast<CDynaNode*>(*it);
		pDynaNodeTest->m_nZoneDistance = -1;
		if (!pDynaNode)
			pDynaNode = pDynaNodeTest;
	}

	if (pDynaNode)
	{
		if (!pDynaNode->m_pSyncZone)
		{
			pDynaNode->m_pSyncZone = CreateNewSynchroZone();
		}
		else
		{
			pDynaNode->m_pSyncZone->Clear();
		}
	}
	return pDynaNode;
}

CDynaNodeBase* CDynaNodeContainer::GetNextNode()
{
	CDynaNodeBase *pDynaNode = NULL;
	for (DEVICEVECTORITR it = begin(); it != end(); it++)
	{
		CDynaNodeBase *pCheckNode = static_cast<CDynaNode*>(*it);
		if (pCheckNode->m_nZoneDistance < 0)
		{
			pDynaNode = pCheckNode;
			pDynaNode->m_nZoneDistance = 0;
			if (!pDynaNode->m_pSyncZone)
			{
				// no sz for node, create new
				pDynaNode->m_pSyncZone = CreateNewSynchroZone();
			}
			else
			{
				// check zone was passed already
				if (pDynaNode->m_pSyncZone->m_bPassed)
				{
					// zone this node belongs has been passed, so it is not in this zone
					pDynaNode->m_pSyncZone = CreateNewSynchroZone();
				}
				else
				{
					// clear sz parameters
					pDynaNode->m_pSyncZone->Clear();
				}
			}
			break;
		}
	}
	return pDynaNode;
}

CSynchroZone* CDynaNodeContainer::CreateNewSynchroZone()
{
	CSynchroZone *pNewSZ = new CSynchroZone();
	pNewSZ->Clear();
	pNewSZ->m_LinkedGenerators.reserve(3 * Count());
	pNewSZ->SetId(m_pSynchroZones->Count());
	pNewSZ->SetName(_T("Синхрозона"));
	m_pSynchroZones->AddDevice(pNewSZ);
	m_bRebuildMatrix = true;
	return pNewSZ;
}

bool CDynaNodeContainer::BuildSynchroZones()
{
	bool bRes = false;

	if (Count() && m_Links.size() > 0)
	{
		CDynaNodeBase *pDynaNode = GetFirstNode();

		if (pDynaNode)
		{
			pDynaNode->m_nZoneDistance = 0;

			std::queue<CDynaNodeBase*>  Queue;
			bRes = true;

			while (pDynaNode)
			{
				Queue.push(pDynaNode);
				while (!Queue.empty())
				{
					CDynaNodeBase *pCurrent = Queue.front();
					pCurrent->MarkZoneEnergized();
					Queue.pop();
					CDevice **ppBranch = NULL;
					CLinkPtrCount *pLink = pCurrent->GetLink(0);
					pCurrent->ResetVisited();
					while (pLink->In(ppBranch))
					{
						CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
						// can reach incident node in case branch is on only, not tripped from begin or end
						if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
						{
							CDynaNodeBase *pn = pBranch->m_pNodeIp == pCurrent ? pBranch->m_pNodeIq : pBranch->m_pNodeIp;
							if (pn->m_nZoneDistance == -1)
							{
								pn->m_nZoneDistance = pCurrent->m_nZoneDistance + 1;

								// if no zone for next node, fill with current zone,
								// but if it is another zone reached from current - fill, but require matrix to be rebuilt

								if (pn->m_pSyncZone)
									if (pn->m_pSyncZone != pCurrent->m_pSyncZone)
										m_bRebuildMatrix = true;

								pn->m_pSyncZone = pCurrent->m_pSyncZone;
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

bool CDynaNodeContainer::EnergizeZones(ptrdiff_t &nDeenergizedCount, ptrdiff_t &nEnergizedCount)
{
	bool bRes = true;
	nDeenergizedCount = nEnergizedCount = 0;
	for (DEVICEVECTORITR it = begin(); it != end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		if (pNode->m_pSyncZone)
		{
			if (pNode->m_pSyncZone->m_bEnergized)
			{
				if (!pNode->IsStateOn())
				{
					if (pNode->GetStateCause() == eDEVICESTATECAUSE::DSC_INTERNAL)
					{
						pNode->SetState(eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_INTERNAL);
						pNode->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszNodeRiseDueToZone, pNode->GetVerbalName()));
						nEnergizedCount++;
					}
				}
			}
			else
			{
				if (pNode->IsStateOn())
				{
					pNode->SetState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
					pNode->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszNodeTripDueToZone, pNode->GetVerbalName()));
					nDeenergizedCount++;
				}
			}
		}
		else
			bRes = false;
	}
	return bRes;
}

void CDynaNodeBase::MarkZoneEnergized()
{
	if (m_pSyncZone)
	{
		CLinkPtrCount *pLink = GetLink(1);
		if (pLink)
		{
			CDevice  **ppGen = NULL;
			ResetVisited();
			while (pLink->In(ppGen))
			{
				CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
				if (pGen->IsKindOfType(DEVTYPE_VOLTAGE_SOURCE) && pGen->IsStateOn())
				{
					m_pSyncZone->m_bEnergized = true;
					m_pSyncZone->m_LinkedGenerators.push_back(pGen);

					if (pGen->GetType() == DEVTYPE_GEN_INFPOWER)
						m_pSyncZone->m_bInfPower = true;
					else
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

bool CDynaNodeContainer::CreateSuperNodes()
{
	bool bRes = true;
	CDeviceContainer *pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	for (DEVICEVECTORITR it = pBranchContainer->begin(); it != pBranchContainer->end(); it++)
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*it);
		if (pBranch->R < 0.1 && pBranch->X < 0.1)
		{
			pBranch->m_pNodeIp->GetVerbalName();
			pBranch->m_pNodeIq->GetVerbalName();
		}
	}
	return bRes;
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
