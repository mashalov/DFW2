#include "stdafx.h"
#include "DynaModel.h"
#include "DeviceContainer.h"


using namespace DFW2;

CDeviceContainer::CDeviceContainer(CDynaModel *pDynaModel) : m_pDynaModel(pDynaModel)
{

}

// очистка контейнера перед заменой содержимого или из под деструктора
void CDeviceContainer::CleanUp()
{
	// если добавляли отдельные устройства - удаляем устройства по отдельности
	if(m_MemoryManagement == ContainerMemoryManagementType::ByPieces)
		for (auto&& it : m_DevVec)
			delete it;
	// сбрасываем тип управления
	m_MemoryManagement = ContainerMemoryManagementType::Unspecified;
	m_Links.clear();
	m_DevVec.clear();
}

CDeviceContainer::~CDeviceContainer()
{
	CleanUp();
}

void CDeviceContainer::RemoveDeviceByIndex(ptrdiff_t nIndex)
{
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_DevVec.size()))
	{
		auto it = m_DevVec.begin() + nIndex;
		// если в контейнера индивидуальные устройства - удаляем прямо устройство
		// если под управлением вектор из фабрики - просто удаляем из вектора указатель
		if(m_MemoryManagement == ContainerMemoryManagementType::ByPieces)
			delete *it;
		m_DevVec.erase(it);
		m_DevSet.clear();
	}
}

void CDeviceContainer::RemoveDevice(ptrdiff_t nId)
{
	for (auto&& it = m_DevVec.begin(); it != m_DevVec.end() ; it++ )
	{
		if ((*it)->GetId() == nId)
		{
			// если в контейнера индивидуальные устройства - удаляем прямо устройство
			// если под управлением вектор из фабрики - просто удаляем из вектора указатель
			if (m_MemoryManagement == ContainerMemoryManagementType::ByPieces)
				delete* it;
			m_DevVec.erase(it);
			m_DevSet.clear();
			break;
		}
	}
}

void CDeviceContainer::SettleDevice(CDevice *pDevice, ptrdiff_t nIndex)
{
	// отмечаем в устройстве его индекс в контейнере
	// этот индекс необходим для связи уравненеий устройств
	pDevice->m_nInContainerIndex = nIndex;
	// сообщаем устройству что оно в контейнере
	pDevice->SetContainer(this);
	// очищаем сет для поиска, так как идет добавление устройств
	// и сет в конце этого процесса должен быть перестроен заново
	m_DevSet.clear();
}

// добавляет устройство в подготовленный контейнер
void CDeviceContainer::AddDevice(CDevice* pDevice)
{
	if (!pDevice)
		throw dfw2error("CDeviceContainer::AddDevice - nullptr passed");
	SetMemoryManagement(ContainerMemoryManagementType::ByPieces);
	SettleDevice(pDevice, m_DevVec.size());
	// помещаем устройство в контейнер
	m_DevVec.push_back(pDevice);
}

// добавление переменной состояния в контейнер
// Требуются имя перменной (уникальное), индекс и единицы измерения
// если переменная с таким именем уже есть возвращает false
bool CDeviceContainer::RegisterVariable(std::string_view VarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits)
{
	bool bInserted = m_ContainerProps.m_VarMap.insert(std::make_pair(VarName, CVarIndex(nVarIndex, eVarUnits))).second;
	return bInserted;
}

// добавление перменной константы устройства (константа - параметр не изменяемый в процессе расчета и пользуемый при инициализации)
// Требуются имя, индекс и тип константы. Индексы у констант и переменных состояния разные
bool CDeviceContainer::RegisterConstVariable(std::string_view VarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits, eDEVICEVARIABLETYPE eDevVarType)
{
	bool bInserted = m_ContainerProps.m_ConstVarMap.insert(std::make_pair(VarName, CConstVarIndex(nVarIndex, eVarUnits, eDevVarType))).second;
	return bInserted;
}

// управление выводом переменной в результаты
bool CDeviceContainer::VariableOutputEnable(std::string_view VarName, bool bOutputEnable)
{
	// ищем переменную по имени в карте переменных контейнера
	auto it = m_ContainerProps.m_VarMap.find(VarName);
	if (it != m_ContainerProps.m_VarMap.end())
	{
		// если нашли - ставим заданный атрибут вывода 
		it->second.m_bOutput = bOutputEnable;
		return true;
	}
	else
		return false;
}

// получить индекс переменной устройства по названию
ptrdiff_t CDeviceContainer::GetVariableIndex(std::string_view VarName) const
{
	auto fnFind = [this](std::string_view VarName) -> ptrdiff_t
	{
		if (auto it{ m_ContainerProps.m_VarMap.find(VarName) }; it != m_ContainerProps.m_VarMap.end())
			return it->second.m_nIndex;
		else
			return -1;
	};

	ptrdiff_t nIndex{ fnFind(VarName) };
	// если переменную не нашли по заданному имени, ищем по алиасам
	if (nIndex < 0)
		if(auto it { m_ContainerProps.m_VarAliasMap.find(VarName) }; it != m_ContainerProps.m_VarAliasMap.end())
			nIndex = fnFind(it->second);
	
	return nIndex;
}
// получить индекс константной переменной по названию
ptrdiff_t CDeviceContainer::GetConstVariableIndex(std::string_view VarName) const
{
	auto fnFind = [this](std::string_view VarName) -> ptrdiff_t
	{
		if (auto it{ m_ContainerProps.m_ConstVarMap.find(VarName) }; it != m_ContainerProps.m_ConstVarMap.end())
			return it->second.m_nIndex;
		else
			return -1;
	};

	ptrdiff_t nIndex{ fnFind(VarName) };
	// если переменную не нашли по заданному имени, ищем по алиасам
	if(nIndex < 0)
		if (auto it{ m_ContainerProps.m_VarAliasMap.find(VarName) }; it != m_ContainerProps.m_VarAliasMap.end())
			nIndex = fnFind(it->second);

	return nIndex;
}

CDevice* CDeviceContainer::GetDeviceByIndex(ptrdiff_t nIndex)
{
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(m_DevVec.size()))
		return m_DevVec[nIndex];
	else
		return nullptr;
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
		for (auto&& it : m_DevVec)
		{
			// ищем устройство из вектора в сете по идентификатору
			if (!m_DevSet.insert(it).second)
			{
				// если такое устройство есть - это дубль.
				if (AlreadyReported.find(it) == AlreadyReported.end())
				{
					// если про дубль устройства еще не сообщали - сообщаем и добавляем устройство в сет дублей
					it->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszDuplicateDevice, it->GetVerbalName()));
					AlreadyReported.insert(it);
				}
				bRes = false;	// если обнаружены дубли - это ошибка
			}
		}
	}
	return bRes;
}

CDevice* CDeviceContainer::GetDevice(CDeviceId* pDeviceId)
{
	CDevice *pRes(nullptr);
	if (SetUpSearch())
	{
		if (auto it{ m_DevSet.find(pDeviceId) }; it != m_DevSet.end())
			pRes = static_cast<CDevice*>(*it);
	}
	return pRes;
}


CDevice* CDeviceContainer::GetDevice(ptrdiff_t nId)
{
	CDeviceId Id(nId);
	return GetDevice(&Id);
}

size_t CDeviceContainer::Count() const
{
	return m_DevVec.size();
}

size_t CDeviceContainer::CountNonPermanentOff() const
{
	return std::count_if(m_DevVec.begin(), m_DevVec.end(), [](const CDevice* pDev)->bool {return !pDev->IsPermanentOff(); });
}

// получить количество переменных, которое нужно выводить в результаты
size_t CDeviceContainer::GetResultVariablesCount()
{
	size_t nCount = 0;
	// определяем простым подсчетом переменных состояния с признаком вывода
	for (auto&& vit : m_ContainerProps.m_VarMap)
		if (vit.second.m_bOutput)
			nCount++;

	return nCount;
}


void CDeviceContainer::Log(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t nDBIndex)
{
	if (m_pDynaModel)
		m_pDynaModel->Log(Status, Message, nDBIndex);
}

bool CDeviceContainer::CreateLink(CDeviceContainer* pLinkContainer)
{
	bool bRes = true;
	// выделяем память под ссылки в данном контейнере и внешнем контейнере, с которым надо установить связь
	// делается для каждого из контейнеров один раз, в качестве флага используется указатель на выделенную память
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
				// просматриваем связи _к_ данному устройству и ищем номер связи, соответствующиий определенной выше LinkFrom
				// если будет найдена связь - внешний контейнер будет трактоваться как соответсвующий типу этой связи
				for (auto && it : m_ContainerProps.m_LinksFrom)
					if (it.second.nLinkIndex == LinkFrom.nLinkIndex)
					{
						TreatAs = it.first;	// если нашли - запоминаем
						break;
					}

				if (TreatAs != DEVTYPE_UNKNOWN)
				{
					// получаем номер связи по найденному типу
					ptrdiff_t nLinkIndex = GetLinkIndex(TreatAs);

					// store new link to links, it will be used by LinkToContainer from container to be linked
					// добавляем к нашим ссылкам еще одну временную связь
					m_Links.emplace_back(pLinkContainer, Count());
					//m_Links.push_back();
					CMultiLink& pLink(m_Links.back());

					// give container to be linked control to link to this container
					// supply this and index of new link

					// связь будет строить внешний контейнер
					// отдаем ему this и индекс временной только что созданной связи

					//ptrdiff_t nOldLinkIndex = LinkFrom.nLinkIndex;		// сохраняем исходный номер связи из свойств контейнера
					LinkFrom.nLinkIndex = m_Links.size() - 1;			// "обманываем" внешний контейнер, заставляя его работать с временной связью вместо основной
					// выполняем связь средствами внешнего контейнера
					bRes = (*pLinkContainer->begin())->LinkToContainer(this, pContLead, LinkTo, LinkFrom);
					_ASSERTE(bRes);

					// если связь выполнена успешно
					if (bRes && nLinkIndex >= 0)
					{
						// объединяем исходную связь со временной
						m_Links[nLinkIndex].Join(pLink);
						// и удаляем временную связь
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
			m_ppSingleLinks = std::make_unique<DevicePtr>(nPossibleLinksCount * Count());
			CDevice **ppLinkPtr = m_ppSingleLinks.get();
			// обходим все устройства в векторе контейнера
			for (auto&& it : m_DevVec)
			{
				// каждому из устройств сообщаем адрес, откуда можно брать связи
				it->SetSingleLinkStart(ppLinkPtr);
				ppLinkPtr += nPossibleLinksCount;
				_ASSERTE(ppLinkPtr <= m_ppSingleLinks.get() + nPossibleLinksCount * Count());
			}
		}
	}
}

// проверяет наличие связи по индексу
bool CDeviceContainer::CheckLink(ptrdiff_t nLinkIndex, LINKSVEC& LinksVec)
{
	return nLinkIndex < static_cast<ptrdiff_t>(LinksVec.size());
}

// проверяет наличие связи по индексу
bool CDeviceContainer::CheckLink(ptrdiff_t nLinkIndex)
{
	return CheckLink(nLinkIndex, m_Links);
}

// возвращает связь заданного типа (по индексу) и устройства из заданного вектора ссылок
CMultiLink& CDeviceContainer::GetCheckLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex, LINKSVEC& LinksVec)
{
	if (!CheckLink(nLinkIndex, LinksVec))
		throw dfw2error("CDeviceContainer::GetCheckLink - LinkIndex out of range");
	auto it = LinksVec.begin() + nLinkIndex;
	if (nDeviceIndex >= static_cast<ptrdiff_t>(it->m_LinkInfo.size()))
		throw dfw2error("CDeviceContainer::GetCheckLink - DeviceIndex out of range");
	return *it;
}
// возвращает связь заданного типа (по индексу) и устройства из основного вектора ссылок
CMultiLink& CDeviceContainer::GetCheckLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex)
{
	return GetCheckLink(nLinkIndex, nDeviceIndex, m_Links);
}

// добавляет элемент для связи с устройством 
void CDeviceContainer::IncrementLinkCounter(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex)
{
	// на входе имеем индекс связей и индекс устройства, к которому нужно сделать связь
	IncrementLinkCounter(GetCheckLink(nLinkIndex, nDeviceIndex), nDeviceIndex);
}

// добавляет элемент для связи с устройством 
void CDeviceContainer::IncrementLinkCounter(CMultiLink& pLink, ptrdiff_t nDeviceIndex)
{
	if (nDeviceIndex >= static_cast<ptrdiff_t>(pLink.m_LinkInfo.size()))
		throw dfw2error("CDeviceContainer::IncrementLinkCounter - DeviceIndex out of range");
	pLink.m_LinkInfo[nDeviceIndex].m_nCount++;
}

// распределяет память под мультисвязи для выбранного индексом типа связей
void CDeviceContainer::AllocateLinks(ptrdiff_t nLinkIndex)
{
	AllocateLinks(GetCheckLink(nLinkIndex, 0));
}

// распределяет память под мультисвязи для выбранного индексом типа связей
void CDeviceContainer::AllocateLinks(CMultiLink& pLink)
{
	// считаем количество связей, которое нужно хранить
	size_t nLinksSize = 0;

	for (auto&& it : pLink.m_LinkInfo)
		nLinksSize += it.m_nCount;

	// выделяем память под нужное количество связей
	pLink.m_ppPointers = std::make_unique<DevicePtr>(pLink.m_nCount = nLinksSize);
	CDevice **ppLink = pLink.m_ppPointers.get();

	// обходим связи всех устройств
	for (auto&& it : pLink.m_LinkInfo)
	{
		// для связей каждого из устройств
		// задаем адрес начала его связей
		it.m_pPointer = ppLink;
		// и резервируем место под конкретное количество связей этого устройства
		ppLink += it.m_nCount;
	}
}

// после добавления связей указатели связей смещаются
// данная функция возвращает указатели к исходному значению как после AllocateLinks()
void CDeviceContainer::RestoreLinks(ptrdiff_t nLinkIndex)
{
	RestoreLinks(GetCheckLink(nLinkIndex, 0));
}

// после добавления связей указатели связей смещаются
// данная функция возвращает указатели к исходному значению как после AllocateLinks()
void CDeviceContainer::RestoreLinks(CMultiLink& pLink)
{
	// обходим все связи
	// указатель смещаем в начальное значение просто вычитая количество добавленных элементов
	for (auto && it : pLink.m_LinkInfo)
		it.m_pPointer -= it.m_nCount;
}

// добавляет новую связь заданного типа для выбранного индексом устройства
void CDeviceContainer::AddLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex, CDevice* pDevice)
{
	AddLink(GetCheckLink(nLinkIndex, nDeviceIndex), nDeviceIndex, pDevice);
}

// добавляет новую связь заданного типа для выбранного индексом устройства
void CDeviceContainer::AddLink(CMultiLink& pLink, ptrdiff_t nDeviceIndex, CDevice* pDevice)
{
	// извлекаем данные связи данного устройства
	CLinkPtrCount *pLinkPtr = pLink.GetLink(nDeviceIndex);
	// в текущий указатель связей вводим указатель на связываемое устройство
	*pLinkPtr->m_pPointer = pDevice;
	// переходим к следующей связи
	pLinkPtr->m_pPointer++;
}

void CDeviceContainer::InitNordsieck(CDynaModel *pDynaModel)
{
	for (auto&& it : m_DevInMatrix)
		it->InitNordsiek(pDynaModel);
}

void CDeviceContainer::Predict()
{
	for (auto&& dit : m_DevInMatrix)
		dit->Predict();
}

void CDeviceContainer::EstimateBlock(CDynaModel *pDynaModel)
{
	m_DevInMatrix.clear();
	if (EquationsCount() > 0)
	{
		m_DevInMatrix.reserve(m_DevVec.size());
		for (auto&& it : m_DevVec)
		{
			it->EstimateEquations(pDynaModel);
			if (it->InMatrix())
				m_DevInMatrix.push_back(it);
		}
	}
}

void CDeviceContainer::FinishStep()
{
	for (auto&& dit : m_DevInMatrix)
		dit->FinishStep();
}

void CDeviceContainer::BuildBlock(CDynaModel* pDynaModel)
{
	for (auto&& it : m_DevInMatrix)
		it->BuildEquations(pDynaModel);
}

void CDeviceContainer::BuildRightHand(CDynaModel* pDynaModel)
{
	for (auto&& it : m_DevInMatrix)
		it->BuildRightHand(pDynaModel);
}


void CDeviceContainer::BuildDerivatives(CDynaModel* pDynaModel)
{
	for (auto&& it : m_DevInMatrix)
		it->BuildDerivatives(pDynaModel);
}

void CDeviceContainer::NewtonUpdateBlock(CDynaModel* pDynaModel)
{
	for (auto&& it : m_DevInMatrix)
		it->NewtonUpdateEquation(pDynaModel);
}

bool CDeviceContainer::LeaveDiscontinuityMode(CDynaModel* pDynaModel)
{
	bool bRes = true;
	for (auto&& it : m_DevInMatrix)
		it->LeaveDiscontinuityMode(pDynaModel);
	return bRes;
}

// обработка разрыва устройств в контейнере
eDEVICEFUNCTIONSTATUS CDeviceContainer::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	m_eDeviceFunctionStatus = eDEVICEFUNCTIONSTATUS::DFS_OK;

	if (GetType() == DEVTYPE_BRANCH)
		return m_eDeviceFunctionStatus;

	for (auto&& it : m_DevVec)
	{
		if (it->IsPermanentOff())
			continue;

		switch (it->CheckProcessDiscontinuity(pDynaModel))
		{
		case eDEVICEFUNCTIONSTATUS::DFS_FAILED:
			m_eDeviceFunctionStatus = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
			break;
		case eDEVICEFUNCTIONSTATUS::DFS_NOTREADY:
			m_eDeviceFunctionStatus = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
			break;
		}
	}
	return m_eDeviceFunctionStatus;
}

// для всех устройств контейнера сбрасывает статус выполнения функции
void CDeviceContainer::UnprocessDiscontinuity()
{
	for (auto&& it : m_DevInMatrix)
		it->UnprocessDiscontinuity();
}

CDevice* CDeviceContainer::GetZeroCrossingDevice()
{
	return m_pClosestZeroCrossingDevice;
}

double CDeviceContainer::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double Kh = 1.0;

	m_pClosestZeroCrossingDevice = nullptr;

	for (auto&& it : m_DevInMatrix)
	{
		double Khi = it->CheckZeroCrossing(pDynaModel);

		// если устройство имеет zerocrossing - считаем его в статистику
		if(Khi < 1.0)
			it->IncrementZeroCrossings();

		if (Khi < Kh)
		{
			Kh = Khi;
			m_pClosestZeroCrossingDevice = it;
			if (Kh < 0.0)
				it->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("Negative ZC ratio rH={} at device \"{}\" at t={}",
					Kh,
					it->GetVerbalName(),
					GetModel()->GetCurrentTime()
				));
		}
	}

	return Kh;
}

eDEVICEFUNCTIONSTATUS CDeviceContainer::PreInit(CDynaModel* pDynaModel)
{
	m_eDeviceFunctionStatus = eDEVICEFUNCTIONSTATUS::DFS_OK;
	
	// делаем предварительную инициализацию устройств, чтобы они смогли привести в порядок свои параметры
	// (например генераторы учтут заданное их количество)
	for (auto&& dev : m_DevVec)
		m_eDeviceFunctionStatus  = CDevice::DeviceFunctionResult(m_eDeviceFunctionStatus, dev->PreInit(pDynaModel));

	if (m_eDeviceFunctionStatus == eDEVICEFUNCTIONSTATUS::DFS_OK && m_DevVec.size())
	{
		// делаем валидацию исходных данных
		// для этого берем экземпляр правил валидаци у первого устройства контейнера
		CSerializerValidator Validator(pDynaModel, GetSerializer(), (*m_DevVec.begin())->GetValidator());
		// и отдаем валидатору
		m_eDeviceFunctionStatus = Validator.Validate();
	}

	return m_eDeviceFunctionStatus;
}



eDEVICEFUNCTIONSTATUS CDeviceContainer::Init(CDynaModel* pDynaModel)
{
	m_eDeviceFunctionStatus = eDEVICEFUNCTIONSTATUS::DFS_OK;

	// ветви хотя и тоже устройства, но мы их не инициализируем
	if (GetType() == DEVTYPE_BRANCH)
		return m_eDeviceFunctionStatus;
	
	// обходим устройства в векторе 
	for (auto&& it : m_DevVec)
	{

		if (it->IsPermanentOff())
			continue;

		// проверяем в каком состоянии находится устройство
		switch (it->CheckInit(pDynaModel))
		{
		case eDEVICEFUNCTIONSTATUS::DFS_FAILED:
			m_eDeviceFunctionStatus = eDEVICEFUNCTIONSTATUS::DFS_FAILED;		// если инициализации устройства завалена - завален и контейнер
			break;
		case eDEVICEFUNCTIONSTATUS::DFS_NOTREADY:
			m_eDeviceFunctionStatus = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;		// если инициализация еще не выполнена, отмечаем, что и контейнер нужно инициализировать позже
			break;
		}
	}

	// some of devices can be marked by state 
	return m_eDeviceFunctionStatus;
}

// получить индекс ссылки по типу заданного контейнера
ptrdiff_t CDeviceContainer::GetLinkIndex(CDeviceContainer* pLinkContainer)
{
	ptrdiff_t nRet = -1;
	// обходим вектор ссылок
	for (auto it = m_Links.begin(); it != m_Links.end(); it++)
		if (it->m_pContainer == pLinkContainer)
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
	for (auto it = m_Links.begin(); it != m_Links.end(); it++)
		if (it->m_pContainer->IsKindOfType(eDeviceType))
		{
			// если заданный тип входит в дерево наследования контейнера 
			// возвращаем его индекс в векторе ссылок
			nRet = it - m_Links.begin();
			break;
		}
	return nRet;
}




// объединяет мультизсвяь с другой мультисвязью
// небходима для объединения связей один ко многим при последовательной линковке
// например с точки зрения узла все генераторы одинаковые, но линкуем мы последовательно
// генераторы разных типов. Поэтому после каждой линковки нам нужно объединять текущие связи
// с только что слинкованными. В итоге узел видит все генераторы всех типов в одной связи

void CMultiLink::Join(CMultiLink& pLink)
{
	_ASSERTE(m_LinkInfo.size() == pLink.m_LinkInfo.size());

	// создаем новый вектор указателей на связанные устройства
	// размер = исходный + объединяемый

	DevicesPtrs ppNewPointers = std::make_unique<DevicePtr>(m_nCount = m_nCount + pLink.m_nCount);
	CDevice** ppNewPtr = ppNewPointers.get();

	CLinkPtrCount *pLeft = &m_LinkInfo[0];
	CLinkPtrCount *pRight = &pLink.m_LinkInfo[0];
	CLinkPtrCount *pLeftEnd = pLeft + m_LinkInfo.size();

	// обходим уже существующие связи
	// для каждого устройства в мультисвязи
	while (pLeft < pLeftEnd)
	{
		CDevice **ppB = pLeft->m_pPointer;
		CDevice **ppE = pLeft->m_pPointer + pLeft->m_nCount;
		CDevice **ppNewPtrInitial = ppNewPtr;

		// обходим устройства, связанные с pLeft
		while (ppB < ppE)
		{
			*ppNewPtr = *ppB;		// копируем связанное устройство в новый вектор указателей
			ppB++;
			ppNewPtr++;
		}

		ppB = pRight->m_pPointer;
		ppE = pRight->m_pPointer + pRight->m_nCount;

		// обходим устройства из объединяемой мультисвязи
		while (ppB < ppE)
		{
			*ppNewPtr = *ppB;		// и тоже копируем связанные устройства в новый вектор указателей
			ppB++;
			ppNewPtr++;
		}

		pLeft->m_pPointer = ppNewPtrInitial;		// обновляем параметры мультисвязи текущего устройства :
		pLeft->m_nCount += pRight->m_nCount;		// указатель на начало и количество связанных устройств

		pLeft++;									// переходим к следующим устройствам в мультизсвязи
		pRight++;									// размерности основной и объединяемой мультисвязей одинаковы
	}
	m_ppPointers = std::move(ppNewPointers);		// очищаем старый вектор указателей
}


void CDeviceContainer::PushVarSearchStack(CDevice*pDevice)
{
	m_pDynaModel->PushVarSearchStack(pDevice);
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

// базовая реализация возвращает константу из свойств
// в перекрытии можно возвращать любое количество уравнений
// но при этом в свойствах контейнера должно быть bVolatile = true
ptrdiff_t CDeviceContainer::EquationsCount() const
{
	return m_ContainerProps.nEquationsCount;
}

CDeviceContainer* CDeviceContainer::DetectLinks(CDeviceContainer* pExtContainer, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom)
{
	CDeviceContainer *pRetContainer(nullptr);

	// просматриваем возможные связи _из_ внешнего контейнер
	for (auto&& extlinkstoit : pExtContainer->m_ContainerProps.m_LinksTo)
	{
		if (IsKindOfType(extlinkstoit.first))
		{
			// если данный контейнер подходит по типу для организации связи
			LinkTo = extlinkstoit.second;
			// возвращаем внешний контейнер и подтверждаем что готовы быть с ним связаны
			pRetContainer = pExtContainer;
			// дополнительно просматриваема связи _из_ контенера 
			for (auto&& linksfrom : m_ContainerProps.m_LinksFrom)
			{
				if (pExtContainer->IsKindOfType(linksfrom.first))
				{
					// если можно связаться по типу с внешним контейнером - заполняем связь, по которой это можно сделать
					LinkFrom = linksfrom.second;
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

		for (auto&& linkstoit : m_ContainerProps.m_LinksTo)
		{
			if (pExtContainer->IsKindOfType(linkstoit.first))
			{
				// если внешний контейнер может быть связан с данным
				LinkTo = linkstoit.second;
				// возвращаем данный контейнер и подтверждаем что он готов с связи с внешним
				pRetContainer = this;
				// дополнительно просматриваем связи _из_ внешнего контейнера
				for (auto&& linksfrom : pExtContainer->m_ContainerProps.m_LinksFrom)
				{
					if (IsKindOfType(linksfrom.first))
					{
						// если данный контейнер может быть связан внешним - заполняем связь
						LinkFrom = linksfrom.second;
						break;
					}
				}
				break;
			}
		}
	}

	return pRetContainer;
}


ptrdiff_t CDeviceContainer::GetSingleLinkIndex(eDFW2DEVICETYPE eDevType)
{
	ptrdiff_t nRet(-1);
	// по информации из атрибутов контейнера определяем индекс
	// связи, соответствующий типу
	LINKSFROMMAP& FromLinks = m_ContainerProps.m_LinksFrom;
	auto&& itFrom  = FromLinks.find(eDevType);
	if (itFrom != FromLinks.end())
		nRet = itFrom->second.nLinkIndex;

	// если связанное устройство не найдено
	// пытаемся определить связь "с другой стороны"

	if (nRet < 0)
	{
		LINKSTOMAP& ToLinks = m_ContainerProps.m_LinksTo;
		auto&& itTo = ToLinks.find(eDevType);
		if (itTo != ToLinks.end())
			nRet = itTo->second.nLinkIndex;
	}

	return nRet;
}

CDynaModel* CDeviceContainer::GetModel() 
{ 
	return m_pDynaModel; 
}

const char* CDeviceContainer::GetSystemClassName() const
{
	return m_ContainerProps.GetSystemClassName();
}

bool CDeviceContainer::HasAlias(std::string_view Alias) const
{
	const STRINGLIST& Aliases = m_ContainerProps.m_lstAliases;
	return std::find(Aliases.begin(), Aliases.end(), Alias) != Aliases.end();
}


void CDeviceContainer::SetMemoryManagement(ContainerMemoryManagementType ManagementType)
{
	if (m_MemoryManagement == ContainerMemoryManagementType::Unspecified)
		m_MemoryManagement = ManagementType;
	else
		if (m_MemoryManagement != ManagementType)
			throw dfw2error(fmt::format("Attempt to mix memory management types for Container {}", GetSystemClassName()));
}


SerializerPtr CDeviceContainer::GetSerializer()
{
	SerializerPtr ret;
	if (Count())
	{
		ret = std::make_unique<CSerializerBase>(new CSerializerDataSourceContainer(this));
		m_DevVec.front()->UpdateSerializer(ret.get());
	}
	return ret;
}