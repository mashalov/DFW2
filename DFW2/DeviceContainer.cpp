#include "stdafx.h"
#include "DynaModel.h"
#include "DeviceContainer.h"


using namespace DFW2;

CDeviceContainer::CDeviceContainer(CDynaModel *pDynaModel) : pDynaModel_(pDynaModel)
{

}

// очистка контейнера перед заменой содержимого или из под деструктора
void CDeviceContainer::CleanUp()
{
	// если добавляли отдельные устройства - удаляем устройства по отдельности
	if(MemoryManagement_ == ContainerMemoryManagementType::ByPieces)
		for (auto&& it : DevVec)
			delete it;
	// сбрасываем тип управления
	MemoryManagement_ = ContainerMemoryManagementType::Unspecified;
	Links_.clear();
	DevVec.clear();
}

CDeviceContainer::~CDeviceContainer()
{
	CleanUp();
}

void CDeviceContainer::RemoveDeviceByIndex(ptrdiff_t nIndex)
{
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(DevVec.size()))
	{
		auto it { DevVec.begin() + nIndex };
		// если в контейнера индивидуальные устройства - удаляем прямо устройство
		// если под управлением вектор из фабрики - просто удаляем из вектора указатель
		if(MemoryManagement_ == ContainerMemoryManagementType::ByPieces)
			delete *it;
		DevVec.erase(it);
		DevSet.clear();
	}
}

void CDeviceContainer::RemoveDevice(ptrdiff_t nId)
{
	for (auto&& it = DevVec.begin(); it != DevVec.end() ; it++ )
	{
		if ((*it)->GetId() == nId)
		{
			// если в контейнера индивидуальные устройства - удаляем прямо устройство
			// если под управлением вектор из фабрики - просто удаляем из вектора указатель
			if (MemoryManagement_ == ContainerMemoryManagementType::ByPieces)
				delete* it;
			DevVec.erase(it);
			DevSet.clear();
			break;
		}
	}
}

void CDeviceContainer::SettleDevice(CDevice *pDevice, ptrdiff_t Index)
{
	// отмечаем в устройстве его индекс в контейнере
	// этот индекс необходим для связи уравненеий устройств
	// сообщаем устройству что оно в контейнере
	pDevice->SetContainer(this, Index);
	// очищаем сет для поиска, так как идет добавление устройств
	// и сет в конце этого процесса должен быть перестроен заново
	DevSet.clear();
}

// добавляет устройство в подготовленный контейнер
void CDeviceContainer::AddDevice(CDevice* pDevice)
{
	if (!pDevice)
		throw dfw2error("CDeviceContainer::AddDevice - nullptr passed");
	SetMemoryManagement(ContainerMemoryManagementType::ByPieces);
	SettleDevice(pDevice, DevVec.size());
	// помещаем устройство в контейнер
	DevVec.push_back(pDevice);
}

// добавление переменной состояния в контейнер
// Требуются имя перменной (уникальное), индекс и единицы измерения
// если переменная с таким именем уже есть возвращает false
bool CDeviceContainer::RegisterVariable(std::string_view VarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits)
{
	bool bInserted{ ContainerProps_.VarMap_.insert(std::make_pair(VarName, CVarIndex(nVarIndex, eVarUnits))).second };
	return bInserted;
}

// добавление перменной константы устройства (константа - параметр не изменяемый в процессе расчета и пользуемый при инициализации)
// Требуются имя, индекс и тип константы. Индексы у констант и переменных состояния разные
bool CDeviceContainer::RegisterConstVariable(std::string_view VarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits, eDEVICEVARIABLETYPE eDevVarType)
{
	bool bInserted{ ContainerProps_.ConstVarMap_.insert(std::make_pair(VarName, CConstVarIndex(nVarIndex, eVarUnits, eDevVarType))).second };
	return bInserted;
}

// управление выводом переменной в результаты
bool CDeviceContainer::VariableOutputEnable(std::string_view VarName, bool OutputEnable)
{
	// ищем переменную по имени в карте переменных контейнера
	auto it{ ContainerProps_.VarMap_.find(VarName) };
	if (it != ContainerProps_.VarMap_.end())
	{
		// если нашли - ставим заданный атрибут вывода 
		it->second.Output_ = OutputEnable;
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
		if (auto it{ ContainerProps_.VarMap_.find(VarName) }; it != ContainerProps_.VarMap_.end())
			return it->second.Index_;
		else
			return -1;
	};

	ptrdiff_t nIndex{ fnFind(VarName) };
	// если переменную не нашли по заданному имени, ищем по алиасам
	if (nIndex < 0)
		if(auto it { ContainerProps_.VarAliasMap_.find(VarName) }; it != ContainerProps_.VarAliasMap_.end())
			nIndex = fnFind(it->second);
	
	return nIndex;
}
// получить индекс константной переменной по названию
ptrdiff_t CDeviceContainer::GetConstVariableIndex(std::string_view VarName) const
{
	auto fnFind = [this](std::string_view VarName) -> ptrdiff_t
	{
		if (auto it{ ContainerProps_.ConstVarMap_.find(VarName) }; it != ContainerProps_.ConstVarMap_.end())
			return it->second.Index_;
		else
			return -1;
	};

	ptrdiff_t nIndex{ fnFind(VarName) };
	// если переменную не нашли по заданному имени, ищем по алиасам
	if(nIndex < 0)
		if (auto it{ ContainerProps_.VarAliasMap_.find(VarName) }; it != ContainerProps_.VarAliasMap_.end())
			nIndex = fnFind(it->second);

	return nIndex;
}

CDevice* CDeviceContainer::GetDeviceByIndex(ptrdiff_t nIndex)
{
	if (nIndex >= 0 && nIndex < static_cast<ptrdiff_t>(DevVec.size()))
		return DevVec[nIndex];
	else
		return nullptr;
}

// формирование сета для поиска устройств в контейнере
bool CDeviceContainer::SetUpSearch()
{
	bool bRes{ true };
	// добавляем устройства в сет только если он был пуст
	// по этому признаку обходим обновление сета,
	// так как SetUpSearch вызывается перед любым поиском устройства
	if (!DevSet.size())
	{
		// сет для контроля дублей
		DEVSEARCHSET AlreadyReported;
		for (auto&& it : DevVec)
		{
			// ищем устройство из вектора в сете по идентификатору
			if (!DevSet.insert(it).second)
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
	CDevice* pRes{ nullptr };
	if (SetUpSearch())
	{
		if (auto it{ DevSet.find(pDeviceId) }; it != DevSet.end())
			pRes = static_cast<CDevice*>(*it);
	}
	return pRes;
}


CDevice* CDeviceContainer::GetDevice(ptrdiff_t nId)
{
	CDeviceId Id{ nId };
	return GetDevice(&Id);
}

size_t CDeviceContainer::Count() const noexcept
{
	return DevVec.size();
}

size_t CDeviceContainer::CountNonPermanentOff() const 
{
	return std::count_if(DevVec.begin(), DevVec.end(), [](const CDevice* pDev)->bool {return !pDev->IsPermanentOff(); });
}

// получить количество переменных, которое нужно выводить в результаты
size_t CDeviceContainer::GetResultVariablesCount()
{
	size_t nCount{ 0 };
	// определяем простым подсчетом переменных состояния с признаком вывода
	for (auto&& vit : ContainerProps_.VarMap_)
		if (vit.second.Output_)
			nCount++;

	return nCount;
}

void CDeviceContainer::DebugLog(const std::string_view Message)
{
	if (pDynaModel_)
		pDynaModel_->DebugLog(Message);
}

void CDeviceContainer::Log(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t nDBIndex)
{
	if (pDynaModel_)
		pDynaModel_->Log(Status, Message, nDBIndex);
}

bool CDeviceContainer::CreateLink(CDeviceContainer* pLinkContainer)
{
	bool bRes{ true };
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
				for (auto && it : ContainerProps_.LinksFrom_)
					if (it.second.nLinkIndex == LinkFrom.nLinkIndex)
					{
						TreatAs = it.first;	// если нашли - запоминаем
						break;
					}

				if (TreatAs != DEVTYPE_UNKNOWN)
				{
					// получаем номер связи по найденному типу
					ptrdiff_t LinkIndex{ GetLinkIndex(TreatAs) };

					// store new link to links, it will be used by LinkToContainer from container to be linked
					// добавляем к нашим ссылкам еще одну временную связь
					Links_.emplace_back(pLinkContainer, Count());
					CMultiLink& pLink(Links_.back());

					// give container to be linked control to link to this container
					// supply this and index of new link

					// связь будет строить внешний контейнер
					// отдаем ему this и индекс временной только что созданной связи

					//ptrdiff_t nOldLinkIndex = LinkFrom.nLinkIndex;		// сохраняем исходный номер связи из свойств контейнера
					LinkFrom.nLinkIndex = Links_.size() - 1;				// "обманываем" внешний контейнер, заставляя его работать с временной связью вместо основной
					// выполняем связь средствами внешнего контейнера
					bRes = (*pLinkContainer->begin())->LinkToContainer(this, pContLead, LinkTo, LinkFrom);
					_ASSERTE(bRes);

					// если связь выполнена успешно
					if (bRes && LinkIndex >= 0)
					{
						// объединяем исходную связь со временной
						Links_[LinkIndex].Join(pLink);
						// и удаляем временную связь
						Links_.pop_back();
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
	const TYPEINFOSET& TypeInfoSet{ ContainerProps_.TypeInfoSet_ };
	// и если находим - возвращаем true - "да, это устройство может быть трактовано как требуемое"
	return TypeInfoSet.find(eType) != TypeInfoSet.end();
}

// распределение памяти под связи устройства
void CDeviceContainer::PrepareSingleLinks()
{
	// если память еще не была распределена и количество устройств в контейнере не нулевое
	if (!ppSingleLinks && Count() > 0)
	{
		// получаем количество возможных связей
		const ptrdiff_t PossibleLinksCount{ GetPossibleSingleLinksCount() };
		// если это количество не нулевое
		if (PossibleLinksCount > 0)
		{
			// выделяем общий буфер под все устройства
			ppSingleLinks = std::make_unique<DevicePtr>(PossibleLinksCount * Count());
			CDevice** ppLinkPtr{ ppSingleLinks.get() };
			// обходим все устройства в векторе контейнера
			for (auto&& it : DevVec)
			{
				// каждому из устройств сообщаем адрес, откуда можно брать связи
				it->SetSingleLinkStart(ppLinkPtr);
				ppLinkPtr += PossibleLinksCount;
				_ASSERTE(ppLinkPtr <= ppSingleLinks.get() + PossibleLinksCount * Count());
			}
		}
	}
}

// проверяет наличие связи по индексу
bool CDeviceContainer::CheckLink(ptrdiff_t LinkIndex, LINKSVEC& LinksVec)
{
	return LinkIndex < static_cast<ptrdiff_t>(LinksVec.size());
}

// проверяет наличие связи по индексу
bool CDeviceContainer::CheckLink(ptrdiff_t LinkIndex)
{
	return CheckLink(LinkIndex, Links_);
}

// возвращает связь заданного типа (по индексу) и устройства из заданного вектора ссылок
CMultiLink& CDeviceContainer::GetCheckLink(ptrdiff_t LinkIndex, ptrdiff_t DeviceIndex, LINKSVEC& LinksVec)
{
	if (!CheckLink(LinkIndex, LinksVec))
		throw dfw2error("CDeviceContainer::GetCheckLink - LinkIndex out of range");
	auto it{ LinksVec.begin() + LinkIndex };
	if (DeviceIndex >= static_cast<ptrdiff_t>(it->LinkInfo_.size()))
		throw dfw2error("CDeviceContainer::GetCheckLink - DeviceIndex out of range");
	return *it;
}
// возвращает связь заданного типа (по индексу) и устройства из основного вектора ссылок
CMultiLink& CDeviceContainer::GetCheckLink(ptrdiff_t LinkIndex, ptrdiff_t DeviceIndex)
{
	return GetCheckLink(LinkIndex, DeviceIndex, Links_);
}

// добавляет элемент для связи с устройством 
void CDeviceContainer::IncrementLinkCounter(ptrdiff_t LinkIndex, ptrdiff_t DeviceIndex)
{
	// на входе имеем индекс связей и индекс устройства, к которому нужно сделать связь
	IncrementLinkCounter(GetCheckLink(LinkIndex, DeviceIndex), DeviceIndex);
}

// добавляет элемент для связи с устройством 
void CDeviceContainer::IncrementLinkCounter(CMultiLink& pLink, ptrdiff_t DeviceIndex)
{
	if (DeviceIndex >= static_cast<ptrdiff_t>(pLink.LinkInfo_.size()))
		throw dfw2error("CDeviceContainer::IncrementLinkCounter - DeviceIndex out of range");
	// в качестве счетчика ссылок используем указатель на конец диапазона
	// после конструктора он равен нулю
	pLink.LinkInfo_[DeviceIndex].pPointerEnd_++;
}

// распределяет память под мультисвязи для выбранного индексом типа связей
void CDeviceContainer::AllocateLinks(ptrdiff_t LinkIndex)
{
	AllocateLinks(GetCheckLink(LinkIndex, 0));
}

// распределяет память под мультисвязи для выбранного индексом типа связей
void CDeviceContainer::AllocateLinks(CMultiLink& pLink)
{
	// считаем количество связей, которое нужно хранить
	size_t LinksSize{ 0 };

	// считаем общее количество ссылок для контейнера
	for (auto&& it : pLink.LinkInfo_)
		LinksSize += it.Count();

	// выделяем память под нужное количество связей
	pLink.ppPointers_ = std::make_unique<DevicePtr>(pLink.Count_ = LinksSize);
	CDevice** ppLink = { pLink.ppPointers_.get() };

	// обходим связи всех устройств
	for (auto&& it : pLink.LinkInfo_)
	{
		// для связей каждого из устройств
		// задаем адрес начала его связей
		// количество ссылок рассчитано pPointerEnd_, pPointerBegin_ = nullptr;
		// запоминаем количество ссылок, потому что далее мы изменим указатели
		const size_t Count{ it.Count() };	
		it.pPointerBegin_ = it.pPointerEnd_ = ppLink;
		// резервируем место под конкретное количество связей этого устройства
		ppLink += Count;
	}
}

// добавляет новую связь заданного типа для выбранного индексом устройства
void CDeviceContainer::AddLink(ptrdiff_t LinkIndex, ptrdiff_t DeviceIndex, CDevice* pDevice)
{
	AddLink(GetCheckLink(LinkIndex, DeviceIndex), DeviceIndex, pDevice);
}

// добавляет новую связь заданного типа для выбранного индексом устройства
void CDeviceContainer::AddLink(CMultiLink& pLink, ptrdiff_t DeviceIndex, CDevice* pDevice)
{
	// извлекаем данные связи данного устройства
	CLinkPtrCount* const pLinkPtr{ pLink.GetAddLink(DeviceIndex) };
	// в текущий указатель связей вводим указатель на связываемое устройство
	// оперируем указателем на конец диапазона, начало не трогаем
	*pLinkPtr->pPointerEnd_ = pDevice;
	// переходим к следующей связи
	pLinkPtr->pPointerEnd_++;
}

void CDeviceContainer::InitNordsieck(CDynaModel *pDynaModel)
{
	for (auto&& it : DevInMatrix)
		it->InitNordsiek(pDynaModel);
}

void CDeviceContainer::Predict()
{
	for (auto&& dit : DevInMatrix)
		dit->Predict();
}

void CDeviceContainer::EstimateBlock(CDynaModel *pDynaModel)
{
	DevInMatrix.clear();
	if (EquationsCount() > 0)
	{
		DevInMatrix.reserve(DevVec.size());
		for (auto&& it : DevVec)
		{
			it->EstimateEquations(pDynaModel);
			if (it->InMatrix())
				DevInMatrix.push_back(it);
		}
	}
}

void CDeviceContainer::FinishStep(const CDynaModel& DynaModel)
{
	for (auto&& dit : DevInMatrix)
		dit->FinishStep(DynaModel);
}

void CDeviceContainer::BuildBlock(CDynaModel* pDynaModel)
{
	for (auto&& it : DevInMatrix)
		it->BuildEquations(pDynaModel);
}

void CDeviceContainer::BuildRightHand(CDynaModel* pDynaModel)
{
	for (auto&& it : DevInMatrix)
		it->BuildRightHand(pDynaModel);
}


void CDeviceContainer::BuildDerivatives(CDynaModel* pDynaModel)
{
	for (auto&& it : DevInMatrix)
		it->BuildDerivatives(pDynaModel);
}

void CDeviceContainer::NewtonUpdateBlock(CDynaModel* pDynaModel)
{
	for (auto&& it : DevInMatrix)
		it->NewtonUpdateEquation(pDynaModel);
}

bool CDeviceContainer::LeaveDiscontinuityMode(CDynaModel* pDynaModel)
{
	bool bRes{ true };
	for (auto&& it : DevInMatrix)
		it->LeaveDiscontinuityMode(pDynaModel);
	return bRes;
}

// обработка разрыва устройств в контейнере
eDEVICEFUNCTIONSTATUS CDeviceContainer::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDeviceFunctionStatus_ = eDEVICEFUNCTIONSTATUS::DFS_OK;

	if (GetType() == DEVTYPE_BRANCH)
		return eDeviceFunctionStatus_;

	for (auto&& it : DevVec)
	{
		if (it->IsPermanentOff())
			continue;

		switch (it->CheckProcessDiscontinuity(pDynaModel))
		{
		case eDEVICEFUNCTIONSTATUS::DFS_FAILED:
			eDeviceFunctionStatus_ = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
			break;
		case eDEVICEFUNCTIONSTATUS::DFS_NOTREADY:
			eDeviceFunctionStatus_ = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
			break;
		}
	}
	return eDeviceFunctionStatus_;
}

// для всех устройств контейнера сбрасывает статус выполнения функции
void CDeviceContainer::UnprocessDiscontinuity()
{
	for (auto&& it : DevInMatrix)
		it->UnprocessDiscontinuity();
}

CDevice* CDeviceContainer::GetZeroCrossingDevice()
{
	return pClosestZeroCrossingDevice_;
}

double CDeviceContainer::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double Kh{ 1.0 };

	pClosestZeroCrossingDevice_ = nullptr;

	for (auto&& it : DevInMatrix)
	{
		double Khi{ it->CheckZeroCrossing(pDynaModel) };

		// если устройство имеет zerocrossing - считаем его в статистику
		if(Khi < 1.0)
			it->IncrementZeroCrossings();

		if (Khi < Kh)
		{
			Kh = Khi;
			pClosestZeroCrossingDevice_ = it;
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
	eDeviceFunctionStatus_= eDEVICEFUNCTIONSTATUS::DFS_OK;
	
	// делаем предварительную инициализацию устройств, чтобы они смогли привести в порядок свои параметры
	// (например генераторы учтут заданное их количество)
	for (auto&& dev : DevVec)
		eDeviceFunctionStatus_  = CDevice::DeviceFunctionResult(eDeviceFunctionStatus_, dev->PreInit(pDynaModel));

	if (eDeviceFunctionStatus_ == eDEVICEFUNCTIONSTATUS::DFS_OK && DevVec.size())
	{
		// делаем валидацию исходных данных
		// для этого берем экземпляр правил валидаци у первого устройства контейнера
		CSerializerValidator Validator(pDynaModel, GetSerializer(), (*DevVec.begin())->GetValidator());
		// и отдаем валидатору
		eDeviceFunctionStatus_ = Validator.Validate();
	}

	return eDeviceFunctionStatus_;
}



eDEVICEFUNCTIONSTATUS CDeviceContainer::Init(CDynaModel* pDynaModel)
{
	eDeviceFunctionStatus_ = eDEVICEFUNCTIONSTATUS::DFS_OK;

	// ветви хотя и тоже устройства, но мы их не инициализируем
	if (GetType() == DEVTYPE_BRANCH)
		return eDeviceFunctionStatus_;
	
	// обходим устройства в векторе 
	for (auto&& it : DevVec)
	{

		if (it->IsPermanentOff())
			continue;

		// проверяем в каком состоянии находится устройство
		switch (it->CheckInit(pDynaModel))
		{
		case eDEVICEFUNCTIONSTATUS::DFS_FAILED:
			eDeviceFunctionStatus_ = eDEVICEFUNCTIONSTATUS::DFS_FAILED;		// если инициализации устройства завалена - завален и контейнер
			break;
		case eDEVICEFUNCTIONSTATUS::DFS_NOTREADY:
			eDeviceFunctionStatus_ = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;		// если инициализация еще не выполнена, отмечаем, что и контейнер нужно инициализировать позже
			break;
		}
	}

	// some of devices can be marked by state 
	return eDeviceFunctionStatus_;
}

// получить индекс ссылки по типу заданного контейнера
ptrdiff_t CDeviceContainer::GetLinkIndex(CDeviceContainer* pLinkContainer)
{
	ptrdiff_t nRet{ -1 };
	// обходим вектор ссылок
	for (auto it = Links_.begin(); it != Links_.end(); it++)
		if (it->pContainer_ == pLinkContainer)
		{
			// если заданный контейнер входит в набор возможных ссылок 
			// возвращаем его индекс в векторе ссылок
			nRet = it - Links_.begin();
			break;
		}
	return nRet;
}

// получить индекс ссылки по типу устройства
ptrdiff_t CDeviceContainer::GetLinkIndex(eDFW2DEVICETYPE eDeviceType)
{
	ptrdiff_t nRet{ -1 };
	// обходим вектор ссылок
	for (auto it = Links_.begin(); it != Links_.end(); it++)
		if (it->pContainer_->IsKindOfType(eDeviceType))
		{
			// если заданный тип входит в дерево наследования контейнера 
			// возвращаем его индекс в векторе ссылок
			nRet = it - Links_.begin();
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
	_ASSERTE(LinkInfo_.size() == pLink.LinkInfo_.size());

	// создаем новый вектор указателей на связанные устройства
	// размер = исходный + объединяемый

	DevicesPtrs ppNewPointers{ std::make_unique<DevicePtr>(Count_ = Count_ + pLink.Count_) };
	CDevice** ppNewPtr{ ppNewPointers.get() };

	CLinkPtrCount* pLeft{ &LinkInfo_[0] }, *pRight{ &pLink.LinkInfo_[0] }, *pLeftEnd{ pLeft + LinkInfo_.size() };

	// обходим уже существующие связи
	// для каждого устройства в мультисвязи
	while (pLeft < pLeftEnd)
	{
		CDevice* const* ppB{ pLeft->begin() };
		CDevice* const* ppE{ pLeft->end() };

		// начало диапазона объединенной сылки
		CDevice** ppNewPtrInitial{ ppNewPtr };

		// обходим устройства, связанные с pLeft
		while (ppB < ppE)
		{
			*ppNewPtr = *ppB;		// копируем связанное устройство в новый вектор указателей
			ppB++;
			ppNewPtr++;
		}

		ppB = pRight->begin();
		ppE = pRight->end();

		// обходим устройства из объединяемой мультисвязи
		while (ppB < ppE)
		{
			*ppNewPtr = *ppB;		// и тоже копируем связанные устройства в новый вектор указателей
			ppB++;
			ppNewPtr++;
		}


		// начало диапазона 
		pLeft->pPointerBegin_ = ppNewPtrInitial;
		// конец диапазона мультисвязи отсчитали от нового начала. Размерность равна сумме своих ссылок и объединяемых
		pLeft->pPointerEnd_ = ppNewPtr;

		pLeft++;									// переходим к следующим устройствам в мультизсвязи
		pRight++;									// размерности основной и объединяемой мультисвязей одинаковы
	}
	ppPointers_ = std::move(ppNewPointers);		// очищаем старый вектор указателей
}


void CDeviceContainer::PushVarSearchStack(CDevice*pDevice)
{
	pDynaModel_->PushVarSearchStack(pDevice);
}

bool CDeviceContainer::PopVarSearchStack(CDevice* &pDevice)
{
	return pDynaModel_->PopVarSearchStack(pDevice);
}

void CDeviceContainer::ResetStack()
{
	pDynaModel_->ResetStack();
}

ptrdiff_t CDeviceContainer::GetPossibleSingleLinksCount()
{
	return ContainerProps_.PossibleLinksCount;
}

// базовая реализация возвращает константу из свойств
// в перекрытии можно возвращать любое количество уравнений
// но при этом в свойствах контейнера должно быть bVolatile = true
ptrdiff_t CDeviceContainer::EquationsCount() const
{
	return ContainerProps_.EquationsCount;
}

CDeviceContainer* CDeviceContainer::DetectLinks(CDeviceContainer* pExtContainer, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom)
{
	CDeviceContainer* pRetContainer{ nullptr };

	// просматриваем возможные связи _из_ внешнего контейнер
	for (auto&& extlinkstoit : pExtContainer->ContainerProps_.LinksTo_)
	{
		if (IsKindOfType(extlinkstoit.first))
		{
			// если данный контейнер подходит по типу для организации связи
			LinkTo = extlinkstoit.second;
			// возвращаем внешний контейнер и подтверждаем что готовы быть с ним связаны
			pRetContainer = pExtContainer;
			// дополнительно просматриваема связи _из_ контенера 
			for (auto&& linksfrom : ContainerProps_.LinksFrom_)
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

		for (auto&& linkstoit : ContainerProps_.LinksTo_)
		{
			if (pExtContainer->IsKindOfType(linkstoit.first))
			{
				// если внешний контейнер может быть связан с данным
				LinkTo = linkstoit.second;
				// возвращаем данный контейнер и подтверждаем что он готов с связи с внешним
				pRetContainer = this;
				// дополнительно просматриваем связи _из_ внешнего контейнера
				for (auto&& linksfrom : pExtContainer->ContainerProps_.LinksFrom_)
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
	ptrdiff_t nRet{ -1 };
	// по информации из атрибутов контейнера определяем индекс
	// связи, соответствующий типу
	const LINKSFROMMAP& FromLinks{ ContainerProps_.LinksFrom_ };
	auto&& itFrom{ FromLinks.find(eDevType) };
	if (itFrom != FromLinks.end())
		nRet = itFrom->second.nLinkIndex;

	// если связанное устройство не найдено
	// пытаемся определить связь "с другой стороны"

	if (nRet < 0)
	{
		const LINKSTOMAP& ToLinks{ ContainerProps_.LinksTo_ };
		auto&& itTo = ToLinks.find(eDevType);
		if (itTo != ToLinks.end())
			nRet = itTo->second.nLinkIndex;
	}

	return nRet;
}

CDynaModel* CDeviceContainer::GetModel() noexcept
{ 
	return pDynaModel_; 
}

const char* CDeviceContainer::GetSystemClassName() const noexcept
{
	return ContainerProps_.GetSystemClassName();
}

bool CDeviceContainer::HasAlias(std::string_view Alias) const
{
	const STRINGLIST& Aliases{ ContainerProps_.Aliases_ };
	return std::find(Aliases.begin(), Aliases.end(), Alias) != Aliases.end();
}


void CDeviceContainer::SetMemoryManagement(ContainerMemoryManagementType ManagementType)
{
	if (MemoryManagement_ == ContainerMemoryManagementType::Unspecified)
		MemoryManagement_ = ManagementType;
	else
		if (MemoryManagement_ != ManagementType)
			throw dfw2error(fmt::format("Attempt to mix memory management types for Container {}", GetSystemClassName()));
}


SerializerPtr CDeviceContainer::GetSerializer()
{
	SerializerPtr ret;
	if (Count())
	{
		ret = std::make_unique<CSerializerBase>(new CSerializerDataSourceContainer(this));
		DevVec.front()->UpdateSerializer(ret.get());
	}
	return ret;
}