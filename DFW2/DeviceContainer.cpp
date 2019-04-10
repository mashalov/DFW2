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

// очистка контейнера перед заменой содержимого или из под деструктора
void CDeviceContainer::CleanUp()
{
	if (m_pControlledData)
	{
		// если был передан линейный массив с созданными устройствами
		// удаляем каждое из них
		delete [] m_pControlledData;
		m_pControlledData = NULL;
	}
	else
	{
		// если добавляли отдельные устройства - удаляем устройства по отдельности
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

// добавляет устройство в подготовленный контейнер
bool CDeviceContainer::AddDevice(CDevice* pDevice)
{
	if (pDevice)
	{
		// отмечаем в устройстве его индекс в контейнере
		// этот индекс необходим для связи уравненеий устройств
		pDevice->m_nInContainerIndex = m_DevVec.size();
		// помещаем устройство в контейнер
		m_DevVec.push_back(pDevice);
		// сообщаем устройству что оно в контейнере
		pDevice->SetContainer(this);
		// очищаем сет для поиска, так как идет добавление устройств
		// и сет в конце этого процесса должен быть перестроен заново
		m_DevSet.clear();
		return true;
	}
	return false;
}

// добавление переменной состояния в контейнер
// Требуются имя перменной (уникальное), индекс и единицы измерения
// если переменная с таким именем уже есть возвращает false
bool CDeviceContainer::RegisterVariable(const _TCHAR* cszVarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits)
{
	bool bInserted = m_ContainerProps.m_VarMap.insert(make_pair(cszVarName, CVarIndex(nVarIndex, eVarUnits))).second;
	return bInserted;
}

// добавление перменной константы устройства (константа - параметр не изменяемый в процессе расчета и пользуемый при инициализации)
// Требуются имя, индекс и тип константы. Индексы у констант и переменных состояния разные
bool CDeviceContainer::RegisterConstVariable(const _TCHAR* cszVarName, ptrdiff_t nVarIndex, eDEVICEVARIABLETYPE eDevVarType)
{
	bool bInserted = m_ContainerProps.m_ConstVarMap.insert(make_pair(cszVarName, CConstVarIndex(nVarIndex, eDevVarType))).second;
	return bInserted;
}

// управление выводом переменной в результаты
bool CDeviceContainer::VariableOutputEnable(const _TCHAR* cszVarName, bool bOutputEnable)
{
	// ищем переменную по имени в карте переменных контейнера
	VARINDEXMAPITR it = m_ContainerProps.m_VarMap.find(cszVarName);
	if (it != m_ContainerProps.m_VarMap.end())
	{
		// если нашли - ставим заданный атрибут вывода 
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

// формирование сета для поиска устройств в контейнере
bool CDeviceContainer::SetUpSearch()
{
	bool bRes = true;
	// добавляем устройства в сет только если он был пуст
	// по этому признаку обходим обновление сета,
	// так как SetUpSearch вызывается перед любым поиском устройства
	if (!m_DevSet.size())
	{
		// сет для контроля дублей
		DEVSEARCHSET AlreadyReported;
		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
		{
			// ищем устройство из вектора в сете по идентификатору
			if (!m_DevSet.insert(*it).second)
			{
				// если такое устройство есть - это дубль.
				if (AlreadyReported.find(*it) == AlreadyReported.end())
				{
					// если про дубль устройства еще не сообщали - сообщаем и добавляем устройство в сет дублей
					(*it)->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDuplicateDevice, (*it)->GetVerbalName()));
					AlreadyReported.insert(*it);
				}
				bRes = false;	// если обнаружены дубли - это ошибка
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

// получить количество переменных, которое нужно выводить в результаты
size_t CDeviceContainer::GetResultVariablesCount()
{
	size_t nCount = 0;
	// определяем простым подсчетом переменных состояния с признаком вывода
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
	// выделяем память под ссылки в данном контейнере и внешнем контейнере, с которым надо установить связь
	PrepareSingleLinks();
	pLinkContainer->PrepareSingleLinks();
	// если во внешнем контейнере есть устройства
	if (pLinkContainer->Count())
	{
		LinkDirectionTo		LinkTo;
		LinkDirectionFrom   LinkFrom;
		// определяем внешние и внутренние связи (небольшой overhead, потому что мы уже вызывали эту функцию раньше, можно закешировать)
		// или вообще передать заранее определенные LinkTo и LinkFrom
		CDeviceContainer *pContLead = DetectLinks(pLinkContainer, LinkTo, LinkFrom);

		if (pContLead && pLinkContainer)
		{
			if (LinkFrom.eLinkMode == DLM_MULTI)
			{
				// если связь одного с несколькими

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
				// если связь одного с одним
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
	// если запрашиваемый тип и есть тип устройства - оно подходит
	if (GetType() == eType) return true;
	// если нет - ищем заданный тип в списке типов наследования
	TYPEINFOSET& TypeInfoSet = m_ContainerProps.m_TypeInfoSet;
	// и если находим - возвращаем true - "да, это устройство может быть трактовано как требуемое"
	return TypeInfoSet.find(eType) != TypeInfoSet.end();
}

// распределение памяти под связи устройства
void CDeviceContainer::PrepareSingleLinks()
{
	// если память еще не была распределена и количество устройств в контейнере не нулевое
	if (!m_ppSingleLinks && Count() > 0)
	{
		// получаем количество возможных связей
		ptrdiff_t nPossibleLinksCount = GetPossibleSingleLinksCount();
		// если это количество не нулевое
		if (nPossibleLinksCount > 0)
		{
			// выделяем общий буфер под все устройства
			m_ppSingleLinks = new CDevice*[nPossibleLinksCount * Count()]();
			CDevice **ppLinkPtr = m_ppSingleLinks;
			// обходим все устройства в векторе контейнера
			for (DEVICEVECTORITR it = begin(); it != end(); it++)
			{
				// каждому из устройств сообщаем адрес, откуда можно брать связи
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

	// ветви хотя и тоже устройства, но мы их не инициализируем
	if (GetType() == DEVTYPE_BRANCH)
		return m_eDeviceFunctionStatus;


	DEVICEVECTORITR it = begin();
	
	// обходим устройства в векторе 
	for (; it != end() ; it++)
	{
		// проверяем в каком состоянии находится устройство
		switch ((*it)->CheckInit(pDynaModel))
		{
		case DFS_FAILED:
			m_eDeviceFunctionStatus = DFS_FAILED;		// если инициализации устройства завалена - завален и контейнер
			break;
		case DFS_NOTREADY:
			m_eDeviceFunctionStatus = DFS_NOTREADY;		// если инициализация еще не выполнена, отмечаем, что и контейнер нужно инициализировать позже
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

// получить индекс ссылки по типу заданного контейнера
ptrdiff_t CDeviceContainer::GetLinkIndex(CDeviceContainer* pLinkContainer)
{
	ptrdiff_t nRet = -1;
	// обходим вектор ссылок
	for (LINKSVECITR it = m_Links.begin(); it != m_Links.end(); it++)
		if ((*it)->m_pContainer == pLinkContainer)
		{
			// если заданный контейнер входит в набор возможных ссылок 
			// возвращаем его индекс в векторе ссылок
			nRet = it - m_Links.begin();
			break;
		}
	return nRet;
}

// получить индекс ссылки по типу устройства
ptrdiff_t CDeviceContainer::GetLinkIndex(eDFW2DEVICETYPE eDeviceType)
{
	ptrdiff_t nRet = -1;
	// обходим вектор ссылок
	for (LINKSVECITR it = m_Links.begin(); it != m_Links.end(); it++)
		if ((*it)->m_pContainer->IsKindOfType(eDeviceType))
		{
			// если заданный тип входит в дерево наследования контейнера 
			// возвращаем его индекс в векторе ссылок
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

	// просматриваем возможные связи _из_ внешнего контейнер
	for (LINKSTOMAPITR extlinkstoit = pExtContainer->m_ContainerProps.m_LinksTo.begin();
			   		   extlinkstoit != pExtContainer->m_ContainerProps.m_LinksTo.end(); 
					   extlinkstoit++)
	{
		if (IsKindOfType(extlinkstoit->first))
		{
			// если данный контейнер подходит по типу для организации связи
			LinkTo = extlinkstoit->second;
			// возвращаем внешний контейнер и подтверждаем что готовы быть с ним связаны
			pRetContainer = pExtContainer;
			// дополнительно просматриваема связи _из_ контенера 
			for (LINKSFROMMAPITR linksfrom = m_ContainerProps.m_LinksFrom.begin();
								 linksfrom != m_ContainerProps.m_LinksFrom.end();
								 linksfrom++)
			{
				if (pExtContainer->IsKindOfType(linksfrom->first))
				{
					// если можно связаться по типу с внешним контейнером - заполняем связь, по которой это можно сделать
					LinkFrom = linksfrom->second;
					break;
				}
			}
			break;
		}
	}

	if (!pRetContainer)
	{
		// если из внешнего контейнера к данном связь не найдена
		// просматриваем возможные связи _из_ данного контейнера

		for (LINKSTOMAPITR linkstoit = m_ContainerProps.m_LinksTo.begin();
						   linkstoit != m_ContainerProps.m_LinksTo.end();
						   linkstoit++)
		{
			if (pExtContainer->IsKindOfType(linkstoit->first))
			{
				// если внешний контейнер может быть связан с данным
				LinkTo = linkstoit->second;
				// возвращаем данный контейнер и подтверждаем что он готов с связи с внешним
				pRetContainer = this;
				// дополнительно просматриваем связи _из_ внешнего контейнера
				for (LINKSFROMMAPITR linksfrom = pExtContainer->m_ContainerProps.m_LinksFrom.begin();
									 linksfrom != pExtContainer->m_ContainerProps.m_LinksFrom.end();
									 linksfrom++)
				{
					if (IsKindOfType(linksfrom->first))
					{
						// если данный контейнер может быть связан внешним - заполняем связь
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