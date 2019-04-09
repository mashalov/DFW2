#include "stdafx.h"
#include "DynaModel.h"
#include "DeviceContainer.h"


using namespace DFW2;

CDeviceContainer::CDeviceContainer(CDynaModel *pDynaModel) : m_pControlledData(NULL),
															 m_ppDevicesAux(NULL),
															 m_ppSingleLinks(NULL),
															 m_pDynaModel(pDynaModel)
{

}

void CDeviceContainer::CleanUp()
{
	if (m_pControlledData)
	{
		delete [] m_pControlledData;
		m_pControlledData = NULL;
	}
	else
	{
		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
			delete (*it);
	}

	if (m_ppSingleLinks)
	{
		delete m_ppSingleLinks;
		m_ppSingleLinks = NULL;
	}

	if (m_ppDevicesAux)
	{
		delete m_ppDevicesAux;
		m_ppDevicesAux = NULL;
	}

	for (LINKSVECITR it = m_Links.begin(); it != m_Links.end(); it++)
		delete (*it);

	m_DevVec.clear();
}

CDeviceContainer::~CDeviceContainer()
{
	CleanUp();
}

bool CDeviceContainer::RemoveDeviceByIndex(ptrdiff_t nIndex)
{
	bool bRes = false;
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_DevVec.size()))
	{
		DEVICEVECTORITR it = m_DevVec.begin() + nIndex;
		delete *it;
		m_DevVec.erase(it);
		m_DevSet.clear();
		bRes = true;
	}
	return bRes;
}

bool CDeviceContainer::RemoveDevice(ptrdiff_t nId)
{
	bool bRes = false;
	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end() ; it++ )
	{
		if ((*it)->GetId() == nId)
		{
			delete *it;
			m_DevVec.erase(it);
			m_DevSet.clear();
			bRes = true;
			break;
		}
	}
	return bRes;
}

bool CDeviceContainer::AddDevice(CDevice* pDevice)
{
	if (pDevice)
	{
		pDevice->m_nInContainerIndex = m_DevVec.size();
		m_DevVec.push_back(pDevice);
		pDevice->SetContainer(this);
		m_DevSet.clear();
		return true;
	}
	return false;
}


bool CDeviceContainer::RegisterVariable(const _TCHAR* cszVarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits)
{
	bool bInserted = m_ContainerProps.m_VarMap.insert(make_pair(cszVarName, CVarIndex(nVarIndex, eVarUnits))).second;
	return bInserted;
}

bool CDeviceContainer::RegisterConstVariable(const _TCHAR* cszVarName, ptrdiff_t nVarIndex, eDEVICEVARIABLETYPE eDevVarType)
{
	bool bInserted = m_ContainerProps.m_ConstVarMap.insert(make_pair(cszVarName, CConstVarIndex(nVarIndex, eDevVarType))).second;
	return bInserted;
}

bool CDeviceContainer::VariableOutputEnable(const _TCHAR* cszVarName, bool bOutputEnable)
{
	VARINDEXMAPITR it = m_ContainerProps.m_VarMap.find(cszVarName);
	if (it != m_ContainerProps.m_VarMap.end())
	{
		it->second.m_bOutput = bOutputEnable;
		return true;
	}
	else
		return false;
}

ptrdiff_t CDeviceContainer::GetVariableIndex(const _TCHAR* cszVarName) const
{
	VARINDEXMAPCONSTITR it = m_ContainerProps.m_VarMap.find(cszVarName);
	if (it != m_ContainerProps.m_VarMap.end())
		return it->second.m_nIndex;
	else
		return -1;
}

ptrdiff_t CDeviceContainer::GetConstVariableIndex(const _TCHAR* cszVarName) const
{
	CONSTVARINDEXMAPCONSTITR it = m_ContainerProps.m_ConstVarMap.find(cszVarName);
	if (it != m_ContainerProps.m_ConstVarMap.end())
		return it->second.m_nIndex;
	else
		return -1;
}

VARINDEXMAPCONSTITR CDeviceContainer::VariablesBegin()
{
	return m_ContainerProps.m_VarMap.begin();
}
VARINDEXMAPCONSTITR CDeviceContainer::VariablesEnd()
{
	return m_ContainerProps.m_VarMap.end();
}

CONSTVARINDEXMAPCONSTITR CDeviceContainer::ConstVariablesBegin()
{
	return m_ContainerProps.m_ConstVarMap.begin();
}
CONSTVARINDEXMAPCONSTITR CDeviceContainer::ConstVariablesEnd()
{
	return m_ContainerProps.m_ConstVarMap.end();
}

CDevice* CDeviceContainer::GetDeviceByIndex(ptrdiff_t nIndex)
{
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_DevVec.size()))
		return m_DevVec[nIndex];
	else
		return NULL;
}

bool CDeviceContainer::SetUpSearch()
{
	bool bRes = true;
	if (!m_DevSet.size())
	{
		DEVSEARCHSET AlreadyReported;
		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
		{
			if (!m_DevSet.insert(*it).second)
			{
				if (AlreadyReported.find(*it) == AlreadyReported.end())
				{
					(*it)->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDuplicateDevice, (*it)->GetVerbalName()));
					AlreadyReported.insert(*it);
				}
				bRes = false;
			}
		}
	}
	return bRes;
}

CDevice* CDeviceContainer::GetDevice(CDeviceId* pDeviceId)
{
	CDevice *pRes = NULL;
	if (SetUpSearch())
	{
		DEVSEARCHSETITR it = m_DevSet.find(pDeviceId);
		if (it != m_DevSet.end())
			pRes = static_cast<CDevice*>(*it);
	}
	return pRes;
}


CDevice* CDeviceContainer::GetDevice(ptrdiff_t nId)
{
	CDeviceId Id(nId);
	return GetDevice(&Id);
}

size_t CDeviceContainer::Count()
{
	return m_DevVec.size();
}

size_t CDeviceContainer::GetResultVariablesCount()
{
	size_t nCount = 0;
	for (VARINDEXMAPCONSTITR vit = VariablesBegin(); vit != VariablesEnd(); vit++)
		if (vit->second.m_bOutput)
			nCount++;
	return nCount;
}


void CDeviceContainer::Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage, ptrdiff_t nDBIndex)
{
	_tcprintf(_T("\n%s Status %d DBIndex %d"), cszMessage, Status, nDBIndex);
}

bool CDeviceContainer::CreateLink(CDeviceContainer* pLinkContainer)
{
	bool bRes = true;
	PrepareSingleLinks();
	pLinkContainer->PrepareSingleLinks();
	if (pLinkContainer->Count())
	{
		LinkDirectionTo		LinkTo;
		LinkDirectionFrom   LinkFrom;
		CDeviceContainer *pContLead = DetectLinks(pLinkContainer, LinkTo, LinkFrom);

		if (pContLead && pLinkContainer)
		{
			if (LinkFrom.eLinkMode == DLM_MULTI)
			{
				eDFW2DEVICETYPE TreatAs = DEVTYPE_UNKNOWN; // search device type to create link according to link map
				for (LINKSFROMMAPITR it = m_ContainerProps.m_LinksFrom.begin(); it != m_ContainerProps.m_LinksFrom.end(); it++)
					if (it->second.nLinkIndex == LinkFrom.nLinkIndex)
					{
						TreatAs = it->first;
						break;
					}

				if (TreatAs != DEVTYPE_UNKNOWN)
				{
					CMultiLink *pLink = new CMultiLink(pLinkContainer, Count());
					ptrdiff_t nLinkIndex = GetLinkIndex(TreatAs);

					// store new link to links, it will be used by LinkToContainer from container to be linked
					m_Links.push_back(pLink);
					// give container to be linked control to link to this container
					// supply this and index of new link

					ptrdiff_t nOldLinkIndex = LinkFrom.nLinkIndex;
					LinkFrom.nLinkIndex = m_Links.size() - 1;	// fake LinkToContainer by pointing link index to new MultiLink
					bRes = (*pLinkContainer->begin())->LinkToContainer(this, pContLead, LinkTo, LinkFrom);

					// when link successful check link index was >= 0
					if (bRes && nLinkIndex >= 0)
					{
						// in this case relink to existing index
						bRes = m_Links[nLinkIndex]->Join(pLink);
						// and remove new link, use present link of type of container to be linked instead
						m_Links.pop_back();
					}
				}
				else
				{
					bRes = false;
				}
			}
			else
			{
				bRes = (*pLinkContainer->begin())->LinkToContainer(this, pContLead, LinkTo, LinkFrom);
			}
		}
		else
		{
			bRes = false;
		}
	}
	return bRes;
}


bool CDeviceContainer::IsKindOfType(eDFW2DEVICETYPE eType)
{
	if (GetType() == eType) return true;

	TYPEINFOSET& TypeInfoSet = m_ContainerProps.m_TypeInfoSet;
	return TypeInfoSet.find(eType) != TypeInfoSet.end();
}

void CDeviceContainer::PrepareSingleLinks()
{
	if (!m_ppSingleLinks && Count() > 0)
	{
		ptrdiff_t nPossibleLinksCount = GetPossibleSingleLinksCount();
		if (nPossibleLinksCount > 0)
		{
			m_ppSingleLinks = new CDevice*[nPossibleLinksCount * Count()]();
			CDevice **ppLinkPtr = m_ppSingleLinks;
			for (DEVICEVECTORITR it = begin(); it != end(); it++)
			{
				(*it)->SetSingleLinkStart(ppLinkPtr);
				ppLinkPtr += nPossibleLinksCount;
				_ASSERTE(ppLinkPtr <= m_ppSingleLinks + nPossibleLinksCount * Count());
			}
		}
	}
}

CMultiLink* CDeviceContainer::GetCheckLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex)
{
	if (nLinkIndex >= 0 && nLinkIndex < static_cast<ptrdiff_t>(m_Links.size()))
	{
		LINKSVECITR it = m_Links.begin() + nLinkIndex;
		if (nDeviceIndex >= 0 && nDeviceIndex < static_cast<ptrdiff_t>((*it)->m_nSize))
			return *it;
	}
	return NULL;
}
bool CDeviceContainer::IncrementLinkCounter(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex)
{
	bool bRes = false;
	CMultiLink *pLink = GetCheckLink(nLinkIndex, nDeviceIndex);
	if (pLink)
	{
		pLink->m_pLinkInfo[nDeviceIndex].m_nCount++;
		bRes = true;
	}
	return bRes;
}

bool CDeviceContainer::AllocateLinks(ptrdiff_t nLinkIndex)
{
	bool bRes = false;
	CMultiLink *pLink = GetCheckLink(nLinkIndex, 0);
	if (pLink)
	{
		CDevice **ppLink = pLink->m_ppPointers;

		for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(pLink->m_nSize); i++)
		{
			CLinkPtrCount *pLinkPtr = &pLink->m_pLinkInfo[i];
			pLinkPtr->m_pPointer = ppLink;
			ppLink += pLinkPtr->m_nCount;
		}
	}
	return bRes;
}

bool CDeviceContainer::RestoreLinks(ptrdiff_t nLinkIndex)
{
	bool bRes = false;
	CMultiLink *pLink = GetCheckLink(nLinkIndex, 0);
	if (pLink)
	{
		for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(pLink->m_nSize); i++)
		{
			CLinkPtrCount *pLinkPtr = &pLink->m_pLinkInfo[i];
			pLinkPtr->m_pPointer -= pLinkPtr->m_nCount;
		}
	}
	return bRes;
}


bool CDeviceContainer::AddLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex, CDevice* pDevice)
{
	bool bRes = false;
	CMultiLink *pLink = GetCheckLink(nLinkIndex, nDeviceIndex);
	if (pLink)
	{
		CLinkPtrCount *pLinkPtr = &pLink->m_pLinkInfo[nDeviceIndex];
		*pLinkPtr->m_pPointer = pDevice;
		pLinkPtr->m_pPointer++;
	}
	return bRes;
}

bool CDeviceContainer::EstimateBlock(CDynaModel *pDynaModel)
{
	bool bRes = true;
	for (DEVICEVECTORITR it = begin(); it != end() && bRes; it++)
	{
		(*it)->EstimateEquations(pDynaModel);
	}
	return bRes;
}

bool CDeviceContainer::BuildBlock(CDynaModel* pDynaModel)
{
	bool bRes = true;
	for (DEVICEVECTORITR it = begin(); it != end() && bRes; it++)
	{
		bRes = (*it)->BuildEquations(pDynaModel) && bRes;
	}
	return bRes;
}

bool CDeviceContainer::BuildRightHand(CDynaModel* pDynaModel)
{
	bool bRes = true;
	for (DEVICEVECTORITR it = begin(); it != end() && bRes; it++)
	{
		bRes = (*it)->BuildRightHand(pDynaModel) && bRes;
	}
	return bRes;
}


bool CDeviceContainer::BuildDerivatives(CDynaModel* pDynaModel)
{
	bool bRes = true;
	for (DEVICEVECTORITR it = begin(); it != end() && bRes; it++)
	{
		bRes = (*it)->BuildDerivatives(pDynaModel) && bRes;
	}
	return bRes;
}


bool CDeviceContainer::NewtonUpdateBlock(CDynaModel* pDynaModel)
{
	bool bRes = true;
	for (DEVICEVECTORITR it = begin(); it != end() && bRes; it++)
	{
		bRes = (*it)->NewtonUpdateEquation(pDynaModel) && bRes;
	}
	return bRes;
}

bool CDeviceContainer::LeaveDiscontinuityMode(CDynaModel* pDynaModel)
{
	bool bRes = true;
	for (DEVICEVECTORITR it = begin(); it != end() && bRes; it++)
	{
		bRes = bRes && (*it)->LeaveDiscontinuityMode(pDynaModel);
	}
	return bRes;
}

eDEVICEFUNCTIONSTATUS CDeviceContainer::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	m_eDeviceFunctionStatus = DFS_OK;

	if (GetType() == DEVTYPE_BRANCH)
		return m_eDeviceFunctionStatus;

	DEVICEVECTORITR it = begin();

	for (; it != end(); it++)
	{
		switch ((*it)->CheckProcessDiscontinuity(pDynaModel))
		{
		case DFS_FAILED:
			m_eDeviceFunctionStatus = DFS_FAILED;
			break;
		case DFS_NOTREADY:
			m_eDeviceFunctionStatus = DFS_NOTREADY;
			break;
		}
	}

	return m_eDeviceFunctionStatus;
}

void CDeviceContainer::UnprocessDiscontinuity()
{
	for (DEVICEVECTORITR it = begin(); it != end(); it++)
		(*it)->UnprocessDiscontinuity();
}

double CDeviceContainer::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double Kh = 1.0;
	for (DEVICEVECTORITR it = begin(); it != end() ; it++)
	{
		double Khi = (*it)->CheckZeroCrossing(pDynaModel);
		if (Khi < Kh)
			Kh = Khi;
	}
	return Kh;
}


eDEVICEFUNCTIONSTATUS CDeviceContainer::Init(CDynaModel* pDynaModel)
{
	m_eDeviceFunctionStatus = DFS_OK;

	if (GetType() == DEVTYPE_BRANCH)
		return m_eDeviceFunctionStatus;


	DEVICEVECTORITR it = begin();
	
	for (; it != end() ; it++)
	{
		switch ((*it)->CheckInit(pDynaModel))
		{
		case DFS_FAILED:
			m_eDeviceFunctionStatus = DFS_FAILED;
			break;
		case DFS_NOTREADY:
			m_eDeviceFunctionStatus = DFS_NOTREADY;
			break;
		}
	}

	// some of devices can be marked by state DS_ABSENT, which means such devices
	// should be removed from model. To do so, elements of vectors of pointers to that devices
	// should be removed

	it = begin();
	while (it != end())
	{
		if (!(*it)->IsPresent())
			it = m_DevVec.erase(it);
		else
			it++;
	}

	// when some elements were removed from vectors,
	// clear set to refresh search
	if (m_DevVec.size() != m_DevSet.size() )
		m_DevSet.clear();

	return m_eDeviceFunctionStatus;
}


ptrdiff_t CDeviceContainer::GetLinkIndex(CDeviceContainer* pLinkContainer)
{
	ptrdiff_t nRet = -1;
	for (LINKSVECITR it = m_Links.begin(); it != m_Links.end(); it++)
		if ((*it)->m_pContainer == pLinkContainer)
		{
			nRet = it - m_Links.begin();
			break;
		}
	return nRet;
}

ptrdiff_t CDeviceContainer::GetLinkIndex(eDFW2DEVICETYPE eDeviceType)
{
	ptrdiff_t nRet = -1;
	for (LINKSVECITR it = m_Links.begin(); it != m_Links.end(); it++)
		if ((*it)->m_pContainer->IsKindOfType(eDeviceType))
		{
			nRet = it - m_Links.begin();
			break;
		}
	return nRet;
}


CMultiLink::CMultiLink(CDeviceContainer* pContainer, size_t nCount) : m_pContainer(pContainer)
{
	m_ppPointers = new CDevice*[m_nCount = pContainer->Count() * 2]; // ?
	m_pLinkInfo = new CLinkPtrCount[nCount];
	m_nSize = nCount;
}

bool CMultiLink::Join(CMultiLink *pLink)
{
	bool bRes = true;
	_ASSERTE(m_nSize == pLink->m_nSize);

	CDevice** ppNewPointers = new CDevice*[m_nCount = m_nCount + pLink->m_nCount];
	CDevice** ppNewPtr = ppNewPointers;

	CLinkPtrCount *pLeft = m_pLinkInfo;
	CLinkPtrCount *pRight = pLink->m_pLinkInfo;
	CLinkPtrCount *pLeftEnd = pLeft + m_nSize;

	while (pLeft < pLeftEnd)
	{
		CDevice **ppB = pLeft->m_pPointer;
		CDevice **ppE = pLeft->m_pPointer + pLeft->m_nCount;
		CDevice **ppNewPtrInitial = ppNewPtr;

		while (ppB < ppE)
		{
			*ppNewPtr = *ppB;
			ppB++;
			ppNewPtr++;
		}

		ppB = pRight->m_pPointer;
		ppE = pRight->m_pPointer + pRight->m_nCount;

		while (ppB < ppE)
		{
			*ppNewPtr = *ppB;
			ppB++;
			ppNewPtr++;
		}

		pLeft->m_pPointer = ppNewPtrInitial;
		pLeft->m_nCount += pRight->m_nCount;

		pLeft++;
		pRight++;
	}
	delete m_ppPointers;
	delete pLink;
	m_ppPointers = ppNewPointers;
	return bRes;
}


bool CDeviceContainer::PushVarSearchStack(CDevice*pDevice)
{
	return m_pDynaModel->PushVarSearchStack(pDevice);
}

bool CDeviceContainer::PopVarSearchStack(CDevice* &pDevice)
{
	return m_pDynaModel->PopVarSearchStack(pDevice);
}

void CDeviceContainer::ResetStack()
{
	m_pDynaModel->ResetStack();
}

ptrdiff_t CDeviceContainer::GetPossibleSingleLinksCount()
{
	return m_ContainerProps.nPossibleLinksCount;
}


ptrdiff_t CDeviceContainer::EquationsCount()
{
	return m_ContainerProps.nEquationsCount;
}


CDeviceContainer* CDeviceContainer::DetectLinks(CDeviceContainer* pExtContainer, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom)
{
	CDeviceContainer *pRetContainer = NULL;

	for (LINKSTOMAPITR extlinkstoit = pExtContainer->m_ContainerProps.m_LinksTo.begin();
			   		   extlinkstoit != pExtContainer->m_ContainerProps.m_LinksTo.end(); 
					   extlinkstoit++)
	{
		if (IsKindOfType(extlinkstoit->first))
		{
			LinkTo = extlinkstoit->second;
			pRetContainer = pExtContainer;
			for (LINKSFROMMAPITR linksfrom = m_ContainerProps.m_LinksFrom.begin();
								 linksfrom != m_ContainerProps.m_LinksFrom.end();
								 linksfrom++)
			{
				if (pExtContainer->IsKindOfType(linksfrom->first))
				{
					LinkFrom = linksfrom->second;
					break;
				}
			}
			break;
		}
	}

	if (!pRetContainer)
	{
		for (LINKSTOMAPITR linkstoit = m_ContainerProps.m_LinksTo.begin();
						   linkstoit != m_ContainerProps.m_LinksTo.end();
						   linkstoit++)
		{
			if (pExtContainer->IsKindOfType(linkstoit->first))
			{
				LinkTo = linkstoit->second;
				pRetContainer = this;
				for (LINKSFROMMAPITR linksfrom = pExtContainer->m_ContainerProps.m_LinksFrom.begin();
									 linksfrom != pExtContainer->m_ContainerProps.m_LinksFrom.end();
									 linksfrom++)
				{
					if (IsKindOfType(linksfrom->first))
					{
						LinkFrom = linksfrom->second;
						break;
					}
				}
				break;
			}
		}
	}

	return pRetContainer;
}


bool  CDeviceContainer::HasAlias(const _TCHAR *cszAlias)
{
	STRINGLIST& Aliases = m_ContainerProps.m_lstAliases;
	return std::find(Aliases.begin(), Aliases.end(), cszAlias) != Aliases.end();
}