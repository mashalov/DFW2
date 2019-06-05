#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

CDevice::CDevice() : m_pContainer(NULL),
					 m_eInitStatus(DFS_NOTREADY),
					 m_State(DS_ON),
					 m_StateCause(DSC_INTERNAL),
					 m_nMatrixRow(0)

{
	
}


CDevice::~CDevice()
{

}

// у устройства нет собственного типа. Тип задается из контейнера
// к которому устройство принадлежит. Если контейнера нет - тип неизвестен
eDFW2DEVICETYPE CDevice::GetType() const
{
	_ASSERTE(m_pContainer);

	if (m_pContainer)
		return m_pContainer->GetType();
	else
		return eDFW2DEVICETYPE::DEVTYPE_UNKNOWN;
}
// функция проверяет входит ли устройство в цепочку наследования от заданного типа
// например - Генератор Мустанг наследован от ШБМ
bool CDevice::IsKindOfType(eDFW2DEVICETYPE eType)
{
	_ASSERTE(m_pContainer);
	if (m_pContainer)
	{
		TYPEINFOSET& TypeInfoSet = m_pContainer->m_ContainerProps.m_TypeInfoSet;
		return TypeInfoSet.find(eType) != TypeInfoSet.end();
	}
	return false;
}

void CDevice::SetContainer(CDeviceContainer* pContainer)
{
	m_pContainer = pContainer;
}

// получить указатель на переменную устройства по имени
double* CDevice::GetVariablePtr(const _TCHAR* cszVarName)
{
	_ASSERTE(m_pContainer);

	// имена переменных хранятся в контейнере
	double *pRes = NULL;
	// если устройство привязано к контейнеру - то можно получить от него индекс
	// переменной, а по нему уже указатель
	if (m_pContainer)
		pRes = GetVariablePtr(m_pContainer->GetVariableIndex(cszVarName));
	return pRes;
}

// получить указатель на константную переменную по имени
// аналогична по смыслу double* GetVariablePtr(const _TCHAR*)
double* CDevice::GetConstVariablePtr(const _TCHAR* cszVarName)
{
	_ASSERTE(m_pContainer);

	double *pRes = NULL;
	if (m_pContainer)
		pRes = GetConstVariablePtr(m_pContainer->GetConstVariableIndex(cszVarName));
	return pRes;
}

// получить описание внешней переменной по имени
ExternalVariable CDevice::GetExternalVariable(const _TCHAR* cszVarName)
{
	_ASSERTE(m_pContainer);

	ExternalVariable ExtVar;
	// в контейнере находим индекс переменной по имени
	ExtVar.nIndex = m_pContainer->GetVariableIndex(cszVarName);

	if (ExtVar.nIndex >= 0)
	{
		// извлекаем указатель на переменную
		ExtVar.pValue = GetVariablePtr(ExtVar.nIndex);
		// извлекаем номер строки в Якоби
		ExtVar.nIndex = A(ExtVar.nIndex);
	}
	else
		ExtVar.pValue = NULL;

	return ExtVar;
}

// виртуальная функиця. Должна быть перекрыта во всех устройствах
double* CDevice::GetVariablePtr(ptrdiff_t nVarIndex)
{
	return NULL;
}

// виртуальная функиця. Должна быть перекрыта во всех устройствах
double* CDevice::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	return NULL;
}

const double* CDevice::GetVariableConstPtr(ptrdiff_t nVarIndex) const
{
	return const_cast<CDevice*>(this)->GetVariablePtr(nVarIndex);
}

const double* CDevice::GetConstVariableConstPtr(ptrdiff_t nVarIndex) const
{
	return const_cast<CDevice*>(this)->GetConstVariablePtr(nVarIndex);
}


const double* CDevice::GetVariableConstPtr(const _TCHAR* cszVarName) const
{
	_ASSERTE(m_pContainer);

	double *pRes = NULL;
	if (m_pContainer)
		pRes = const_cast<CDevice*>(this)->GetVariablePtr(m_pContainer->GetVariableIndex(cszVarName));
	return pRes;
}

const double* CDevice::GetConstVariableConstPtr(const _TCHAR* cszVarName) const
{
	_ASSERTE(m_pContainer);

	double *pRes = NULL;
	if (m_pContainer)
		pRes = const_cast<CDevice*>(this)->GetConstVariablePtr(m_pContainer->GetConstVariableIndex(cszVarName));
	return pRes;
}

double CDevice::GetValue(ptrdiff_t nVarIndex) const
{
	const double *pRes = GetVariableConstPtr(nVarIndex);
	if (pRes)
		return *pRes;
	else
		return 0.0;
}

// установить значение переменной по индексу и вернуть исходное значение
double CDevice::SetValue(ptrdiff_t nVarIndex, double Value)
{
	double *pRes = GetVariablePtr(nVarIndex);
	double OldValue = 0.0;
	if (pRes)
	{
		OldValue = *pRes;
		*pRes = Value;
	}
	return OldValue;
}

double CDevice::GetValue(const _TCHAR* cszVarName) const
{
	const double *pRes = GetVariableConstPtr(cszVarName);
	if (pRes)
		return *pRes;
	else
		return 0.0;
}

// установить значение переменной по имени и вернуть исходное значение
double CDevice::SetValue(const _TCHAR* cszVarName, double Value)
{
	double *pRes = GetVariablePtr(cszVarName);
	double OldValue = 0.0;
	if (pRes)
	{
		OldValue = *pRes;
		*pRes = Value;
	}
	return OldValue;
}

void CDevice::Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage)
{
	// TODO - add device type information

	if (m_pContainer)
		m_pContainer->Log(Status, cszMessage, GetDBIndex());
	else
		_tcprintf(_T("\n%s Status %d DBIndex %d"), cszMessage, Status, GetDBIndex());
}

// связь экземпляров устройств по информации из из контейнеров
bool CDevice::LinkToContainer(CDeviceContainer *pContainer, CDeviceContainer *pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom)
{
	// на входе pCountainer - внешний контейнер
	// pContLead - контейнер по данным которого надо делать связи
	bool bRes = false;

	if (m_pContainer && pContLead)
	{
		bRes = true;
		DEVICEVECTORITR it;
		// выбирает подчиненный контейнер по pContLead
		// если pContLead совпадает с внешним контейнером - подчиненный - контейнер данного устройства
		// если совпадает в контейнером данного устройства - подчиненный - внешний контейнер
		CDeviceContainer *pContSlave = (pContLead == pContainer) ? m_pContainer : pContainer;

		// заранее определяем индекс константы, в которой лежит номер связываемого устройства
		ptrdiff_t nIdFieldIndex = pContLead->GetConstVariableIndex(LinkTo.strIdField.c_str());

		for (it = pContLead->begin(); it != pContLead->end(); it++)
		{
			// проходим по устройствам мастер-контейнера
			CDevice *pDev = *it;
			CDevice *pLinkDev = NULL;

			// пытаемся получить идентификатор устройства, с которым должно быть связано устройство из мастер-контейнера
			const double *pdDevId = pDev->GetConstVariableConstPtr(nIdFieldIndex);
			ptrdiff_t DevId = (pdDevId) ? static_cast<ptrdiff_t>(*pdDevId) : -1;

			if (DevId > 0)
			{
				// если идентификатор найден и ненулевой
				// ищем устройство с этим идентификатором в подчиненном контейнере
				if (pContSlave->GetDevice(DevId, pLinkDev))
				{
					// достаем устройство, которое уже связано с текущим по данному типу связи
					CDevice *pAlreadyLinked = pDev->GetSingleLink(LinkTo.nLinkIndex);

					// если предусмотрена связь один с несколькими игнорируем уже связанное устройство
					if (LinkTo.eLinkMode == DLM_MULTI)
						pAlreadyLinked = NULL;

					if (!pAlreadyLinked)
					{
						// если уже связанного устройства нет - привязываем устройство мастер-контейнера к найденному в подчиненном контейнере
						if (!pDev->SetSingleLink(LinkTo.nLinkIndex, pLinkDev))
						{
							bRes = false;
							break;
						}

						// если режим связи один к одному - привязываем и подчиненное устройство к мастер-устройству
						if (LinkFrom.eLinkMode == DLM_SINGLE)
						{
							if (!pLinkDev->SetSingleLink(LinkFrom.nLinkIndex, pDev))
							{
								bRes = false;
								break;
							}
						}
						else
						{
							// если режим связи много к одному - добавляем элемент мультисвязи
							bRes = pLinkDev->IncrementLinkCounter(LinkFrom.nLinkIndex) && bRes;
						}
					}
					else
					{
						// если устройство уже было сязано с каким-то и связь один к одному - выдаем ошибку
						// указываея мастер, подчиенное и уже связанное
						pDev->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDeviceAlreadyLinked, pDev->GetVerbalName(), pLinkDev->GetVerbalName(), pAlreadyLinked->GetVerbalName()));
						bRes = false;
					}
				}
				else
				{
					// если устройство для связи по идентификатору не нашли - выдаем ошибку
					pDev->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDeviceForDeviceNotFound, DevId, pDev->GetVerbalName()));
					bRes = false;
				}
			}
		}

		if (bRes && LinkFrom.eLinkMode == DLM_MULTI)
		{
			// если у связываемого контейнера бьл режим мультисвязи
			// размечаем мультисвязи в связываемом контейнере
			pContSlave->AllocateLinks(LinkFrom.nLinkIndex);
			// для каждого из устройств мастер-контейнера
			for (it = pContLead->begin(); it != pContLead->end(); it++)
			{
				CDevice *pDev = *it;
				// извлекаем связанное устройство данного типа из мастер-устройства
				CDevice *pDevLinked = pDev->GetSingleLink(LinkTo.nLinkIndex);

				// и добавляем связь с мастер-устройством в мультисвязь подчиненного устройства
				if (pDevLinked)
					pContSlave->AddLink(LinkFrom.nLinkIndex, pDevLinked->m_nInContainerIndex, pDev);
			}
			// после добавления связей восстанавливаем внутренние указатели связей
			pContSlave->RestoreLinks(LinkFrom.nLinkIndex);

		}
	}
	return bRes;
}

// добавляет элемент связи для хранения связи с очередным устройством
bool CDevice::IncrementLinkCounter(ptrdiff_t nLinkIndex)
{
	_ASSERTE(m_pContainer);

	bool bRes = false;
	if (m_pContainer)
		bRes = m_pContainer->IncrementLinkCounter(nLinkIndex, m_nInContainerIndex);
	return bRes;
}

// устройство может быть связано с несколькими типами разных устройств,
// причем связей каждого из типов тоже может быть несколько
// функция возвращает связи устройства с заданным типом. Тип задается не явно, а в виде
// индекса слоя связей
CLinkPtrCount* CDevice::GetLink(ptrdiff_t nLinkIndex)
{
	_ASSERTE(m_pContainer);

	// возвращаем объект, в котором есть список ссылок данного устройства
	CLinkPtrCount *pLink(NULL);
	// ссылки хранятся в контейнере
	if (m_pContainer)
	{
		// извлекаем список ссылок по заданному типу и индексу текущего устройства
		CMultiLink *pMultiLink = m_pContainer->GetCheckLink(nLinkIndex, m_nInContainerIndex);
		// если список ссылок есть - возвращаем список
		if (pMultiLink)
			pLink = &pMultiLink->m_pLinkInfo[m_nInContainerIndex];
	}
	return pLink;
}

// функция обхода связей устройства (типа обход ветвей узла, генераторов узла и т.п.)
// на входе указатель на указатель устройства, с которым связь. Каждый следующий
// вызов In() возвращает очередную связь и true, или false - если связи закончились
// начало последовательности требует чтобы на вход был передан указатель на null
bool CLinkPtrCount::In(CDevice ** & p)
{
	if (!p)
	{
		// если передан указатель на null
		if (m_nCount)
		{
			// если связи есть - возвращаем первую
			p = m_pPointer;
			return true;
		}
		else
		{
			// если связей нет - завершаем обход
			p = NULL;
			return false;
		}
	}

	// если передан указатель не на null, это
	// означает, что мы уже начали обходить связи
	// переходм к следующей
	p++;

	// проверяем, не достили ли конца списка связей
	if (p < m_pPointer + m_nCount)
		return true;

	// если достигли - завершаем обход
	p = NULL;
	return false;
}

// функция увеличивает размерность модели на количество уравнений устройства, 
// и заодно фиксирует строку матрицы, с которой начинается блок его уравнений
void CDevice::EstimateEquations(CDynaModel *pDynaModel)
{
	m_nMatrixRow = pDynaModel->AddMatrixSize(m_pContainer->EquationsCount());
}

// базовая функция инициализации Nordsieck
// на самом деле c Nordsieck функция не работает, а только 
// участвует в CDynaModel::InitNordsieck() для инициализации правой части уравнений
void CDevice::InitNordsiek(CDynaModel* pDynaModel)
{
	_ASSERTE(m_pContainer);

	struct RightVector *pRv = pDynaModel->GetRightVector(A(0));
	ptrdiff_t nEquationsCount = m_pContainer->EquationsCount();

	// инициализация заключается в обходе всех переменных устройства и:
	for (ptrdiff_t z = 0; z < nEquationsCount ; z++)
	{
		pRv->pValue = GetVariablePtr(z);	// передачи значения переменной устройства в структуру вектора правой части
		_ASSERTE(pRv->pValue);
		pRv->pDevice = this;				// передачи данных об устройстве в структуру правой части
		pRv++;
	}
}

bool CDevice::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	return bRes;
}

bool CDevice::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;
	return bRes;
}

bool CDevice::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	return bRes;
}

bool CDevice::NewtonUpdateEquation(CDynaModel *pDynaModel)
{
	bool bRes = true;
	return bRes;
}

// сбрасывает указатель просмотренных ссылок устройства
void CDevice::ResetVisited()
{
	_ASSERTE(m_pContainer);
	// если списка просмотра нет - создаем его
	if (!m_pContainer->m_ppDevicesAux)
		m_pContainer->m_ppDevicesAux = new CDevice*[m_pContainer->Count()];
	// обнуляем счетчик просмотренных ссылок
	m_pContainer->m_nVisitedCount = 0;
}

bool CDevice::CheckAddVisited(CDevice* pDevice)
{
	_ASSERTE(m_pContainer);

	bool bRes = false;
	CDevice **ppDevice = m_pContainer->m_ppDevicesAux;
	CDevice **ppEnd = ppDevice + m_pContainer->m_nVisitedCount;
	// просматриваем список просмотренных
	for (; ppDevice < ppEnd; ppDevice++)
		if (*ppDevice == pDevice)
			break; // если нашли заданное устройство - выходим с false

	if (ppDevice == ppEnd)
	{
		// если дошли до конца списка и не нашли запрошенного устройства
		// добавляем его в список просмотренных
		*ppDevice = pDevice;
		// и счетчик просмотренных увеличиваем
		m_pContainer->m_nVisitedCount++;
		bRes = true;
	}
	return bRes;
}

// проверка инициализации/инициализация устройства
eDEVICEFUNCTIONSTATUS CDevice::CheckInit(CDynaModel* pDynaModel)
{
	if (m_eInitStatus != DFS_OK)
	{
		// если успешной инициализации еще не выполнено
		// проверяем что с инициализацией ведущих устройств
		m_eInitStatus = MastersReady(&CDevice::CheckMasterDeviceInit);

		if (IsPresent())
		{
			// если устройство представлено в модели
			// и ведущие устройства инициализированы 
			if(m_eInitStatus == DFS_OK)
				m_eInitStatus = Init(pDynaModel);		// пытаемся инициализировать устройство

			// если устройство не нуждается в инициализации
			if (m_eInitStatus == DFS_DONTNEED)	
				m_eInitStatus = DFS_OK;					// делаем вид что инициализация прошла успешно
		}
		else
			m_eInitStatus = DFS_OK;						// для отсутствующих в модели устройств также возвращаем успешную инициализацию
														// так как далее мы их просто удалим из модели
	}
	return m_eInitStatus;
}

eDEVICEFUNCTIONSTATUS CDevice::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return DFS_DONTNEED;
}

eDEVICEFUNCTIONSTATUS CDevice::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eStatus = DFS_OK;
	return eStatus;
}

eDEVICEFUNCTIONSTATUS CDevice::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause)
{
	m_State = eState;
	m_StateCause = eStateCause;
	return DFS_OK;
}

const _TCHAR* CDevice::VariableNameByPtr(double *pVariable)
{
	_ASSERTE(m_pContainer);

	const _TCHAR *pName = CDFW2Messages::m_cszUnknown;
	ptrdiff_t nEquationsCount = m_pContainer->EquationsCount();

	for (ptrdiff_t i = 0; i < nEquationsCount ; i++)
	{
		const double *pVar = GetVariableConstPtr(i);
		if (pVar == pVariable)
		{
			VARINDEXMAPCONSTITR it = m_pContainer->VariablesBegin();
			for ( ; it != m_pContainer->VariablesEnd(); it++)
			{
				if (it->second.m_nIndex == i)
				{
					pName = static_cast<const _TCHAR*>(it->first.c_str());
					break;
				}
			}
#ifdef _DEBUG
			if (it == m_pContainer->VariablesEnd())
			{
				_tcsncpy_s(UnknownVarIndex, 80, Cex(_T("Unknown name Index - %d"), i), 80);
				pName = static_cast<_TCHAR*>(UnknownVarIndex);
			}
#endif

		}
	}
	return pName;
}

bool CDevice::LeaveDiscontinuityMode(CDynaModel* pDynaModel)
{
	return true;
}

eDEVICEFUNCTIONSTATUS CDevice::CheckProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_eInitStatus != DFS_OK)
	{
		m_eInitStatus = MastersReady(&CheckMasterDeviceDiscontinuity);

		if (m_eInitStatus == DFS_OK)
			m_eInitStatus = ProcessDiscontinuity(pDynaModel);

		if (m_eInitStatus == DFS_DONTNEED)
			m_eInitStatus = DFS_OK;
	}

	_ASSERTE(m_eInitStatus != DFS_FAILED);
	return m_eInitStatus;
}


eDEVICEFUNCTIONSTATUS CDevice::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	return DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDevice::DeviceFunctionResult(eDEVICEFUNCTIONSTATUS Status1, eDEVICEFUNCTIONSTATUS Status2)
{
	if (Status1 == DFS_FAILED || Status2 == DFS_FAILED)
		return DFS_FAILED;

	if (Status1 == DFS_NOTREADY || Status2 == DFS_NOTREADY)
		return DFS_NOTREADY;

	return DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDevice::DeviceFunctionResult(eDEVICEFUNCTIONSTATUS Status1, bool Status2)
{
	if (!Status2)
		return DFS_FAILED;

	return Status1;
}

eDEVICEFUNCTIONSTATUS CDevice::DeviceFunctionResult(bool Status1)
{
	if (!Status1)
		return DFS_FAILED;

	return DFS_OK;
}

bool CDevice::InitExternalVariable(PrimitiveVariableExternal& ExtVar, CDevice* pFromDevice, const _TCHAR* cszName, eDFW2DEVICETYPE eLimitDeviceType)
{
	_ASSERTE(m_pContainer);

	bool bRes = false;
	ExtVar.UnIndex();


	if (eLimitDeviceType == DEVTYPE_MODEL)
	{
		bRes = m_pContainer->GetModel()->InitExternalVariable(ExtVar, pFromDevice, cszName);
	}
	else
	{
		if (pFromDevice)
		{
			CDevice *pInitialDevice = pFromDevice;
			m_pContainer->ResetStack();
			if (m_pContainer->PushVarSearchStack(pFromDevice))
			{
				while (m_pContainer->PopVarSearchStack(pFromDevice))
				{
					bool bTryGet = true;
					if (eLimitDeviceType != DEVTYPE_UNKNOWN)
						if (!pFromDevice->IsKindOfType(eLimitDeviceType))
							bTryGet = false;

					if (bTryGet)
					{
						ExternalVariable extVar = pFromDevice->GetExternalVariable(cszName);
						if (extVar.pValue)
						{
							ExtVar.IndexAndValue(extVar.nIndex - A(0), extVar.pValue);
							bRes = true;
						}
					}

					if (!bRes)
					{
						const SingleLinksRange& LinkRange = pFromDevice->GetSingleLinks().GetLinksRange();
						for (CDevice **ppStart = LinkRange.m_ppLinkStart; ppStart < LinkRange.m_ppLinkEnd; ppStart++)
						{
							if (*ppStart != pInitialDevice)
								if (!m_pContainer->PushVarSearchStack(*ppStart))
									break;
						}
					}
					else
						break;
				}
			}

			if (!bRes)
			{
				Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszExtVarNotFoundInDevice, GetVerbalName(), cszName, pInitialDevice->GetVerbalName()));
				bRes = false;
			}
		}
		else
		{
			Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszExtVarNoDeviceFor, cszName, GetVerbalName()));
			bRes = false;
		}
	}
	return bRes;
}


bool CDevice::InitConstantVariable(double& ConstVar, CDevice* pFromDevice, const _TCHAR* cszName, eDFW2DEVICETYPE eLimitDeviceType)
{
	_ASSERTE(m_pContainer);

	bool bRes = false;
	m_pContainer->ResetStack();
	if (pFromDevice)
	{
		CDevice *pInitialDevice = pFromDevice;

		if (m_pContainer->PushVarSearchStack(pFromDevice))
		{
			while (m_pContainer->PopVarSearchStack(pFromDevice))
			{
				bool bTryGet = true;
				if (eLimitDeviceType != DEVTYPE_UNKNOWN)
					if (!pFromDevice->IsKindOfType(eLimitDeviceType))
						bTryGet = false;

				if (bTryGet)
				{
					double *pConstVar = pFromDevice->GetConstVariablePtr(cszName);
					if (pConstVar)
					{
						ConstVar = *pConstVar;
						bRes = true;
					}
				}

				if (!bRes)
				{
					const SingleLinksRange& LinkRange = pFromDevice->GetSingleLinks().GetLinksRange();
					for (CDevice **ppStart = LinkRange.m_ppLinkStart; ppStart < LinkRange.m_ppLinkEnd; ppStart++)
					{
						if (*ppStart != pInitialDevice)
							if (!m_pContainer->PushVarSearchStack(*ppStart))
								break;
					}
				}
				else
					break;
			}
		}

		if (!bRes)
		{
			Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszConstVarNotFoundInDevice, GetVerbalName(), cszName, pInitialDevice->GetVerbalName()));
			bRes = false;
		}
	}
	else
	{
		Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszConstVarNoDeviceFor, cszName, GetVerbalName()));
		bRes = false;
	}

	return bRes;
}

// установить адрес, начиная с которого устройство сможет получить
// адреса связанных устройств
void CDevice::SetSingleLinkStart(CDevice **ppLinkStart)
{
	_ASSERTE(m_pContainer);
	m_DeviceLinks.SetRange(SingleLinksRange(ppLinkStart, ppLinkStart + m_pContainer->GetPossibleSingleLinksCount()));
}

bool CDevice::SetSingleLink(ptrdiff_t nIndex, CDevice *pDevice)
{
	if (!m_DeviceLinks.SetLink(nIndex, pDevice))
	{
		Log(CDFW2Messages::DFW2LOG_FATAL, Cex(CDFW2Messages::m_cszWrongSingleLinkIndex, nIndex, GetVerbalName(), m_pContainer->GetPossibleSingleLinksCount()));
		return false;
	}
	return true;
}

// подробное имя устройства формируется по с описанием типа. Остальное по правилам CDeviceId::UpdateVerbalName
void CDevice::UpdateVerbalName()
{
	CDeviceId::UpdateVerbalName();
	if (m_pContainer)
		m_strVerbalName = Cex(_T("%s %s"), m_pContainer->GetTypeName(), m_strVerbalName.c_str());
}

// получить связанное устройство по индексу связи
CDevice* CDevice::GetSingleLink(ptrdiff_t nIndex)
{
	return m_DeviceLinks.GetLink(nIndex);
}

// получить связанное устройство по типу связанного устройства
CDevice* CDevice::GetSingleLink(eDFW2DEVICETYPE eDevType)
{
	_ASSERTE(m_pContainer);

	CDevice *pRetDev = NULL;

	if (m_pContainer)
	{
		// по информации из атрибутов контейнера определяем индекс
		// связи, соответствующий типу
		LINKSFROMMAP& FromLinks = m_pContainer->m_ContainerProps.m_LinksFrom;
		LINKSFROMMAPITR itFrom = FromLinks.find(eDevType);
		if (itFrom != FromLinks.end())
			pRetDev = GetSingleLink(itFrom->second.nLinkIndex);
	
		// если связанное устройство не найдено
		// пытаемся определить связь "с другой стороны"

#ifdef _DEBUG
		CDevice *pRetDevTo = NULL;
		LINKSTOMAP& ToLinks = m_pContainer->m_ContainerProps.m_LinksTo;
		LINKSTOMAPITR itTo = ToLinks.find(eDevType);

		// в режиме отладки проверяем однозначность определения связи
		if (itTo != ToLinks.end())
		{
			pRetDevTo = GetSingleLink(itTo->second.nLinkIndex);
			if (pRetDev && pRetDevTo)
				_ASSERTE(!_T("CDevice::GetSingleLink - Ambiguous link"));
			else
				pRetDev = pRetDevTo;
		}
#else
		if(!pRetDev)
		{
			LINKSTOMAP& ToLinks = m_pContainer->m_ContainerProps.m_LinksTo;
			LINKSTOMAPITR itTo = ToLinks.find(eDevType);
			if (itTo != ToLinks.end())
				pRetDev = GetSingleLink(itTo->second.nLinkIndex);
		}
#endif
	}
	return pRetDev;
}


eDEVICEFUNCTIONSTATUS CDevice::CheckMasterDeviceInit(CDevice *pDevice, LinkDirectionFrom& LinkFrom)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	_ASSERTE(LinkFrom.eDependency == DPD_MASTER);

	CDevice *pDev = pDevice->GetSingleLink(LinkFrom.nLinkIndex);

	if (pDev && pDev->IsPresent())
	{
		Status = pDev->Initialized();

		if (CDevice::IsFunctionStatusOK(Status))
		{
			if (!pDev->IsStateOn())
			{
				pDevice->SetState(DS_OFF, DSC_INTERNAL);
			}
		}
	}
	else
	{
		pDevice->SetState(DS_ABSENT, DSC_INTERNAL);
	}

	_ASSERTE(Status != DFS_FAILED);

	return Status;
}


eDEVICEFUNCTIONSTATUS CDevice::CheckMasterDeviceDiscontinuity(CDevice *pDevice, LinkDirectionFrom& LinkFrom)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	_ASSERTE(LinkFrom.eDependency == DPD_MASTER);
	CDevice *pDev = pDevice->GetSingleLink(LinkFrom.nLinkIndex);

	if (pDev)
	{
		Status = pDev->DiscontinuityProcessed();

		if (CDevice::IsFunctionStatusOK(Status))
		{
			if (!pDev->IsStateOn())
			{
				pDevice->SetState(DS_OFF, DSC_INTERNAL);
			}
		}
	}

	_ASSERTE(Status != DFS_FAILED);

	return Status;
}

eDEVICEFUNCTIONSTATUS CDevice::MastersReady(CheckMasterDeviceFunction* pFnCheckMasterDevice)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;
	CDeviceContainerProperties &Props = m_pContainer->m_ContainerProps;

	// use links to masters, prepared in CDynaModel::Link() instead of original links
	LINKSTOMAP	 &LinksTo = Props.m_MasterLinksTo;
	for (LINKSTOMAPITR it1 = LinksTo.begin(); it1 != LinksTo.end(); it1++)
	{
		Status = CDevice::DeviceFunctionResult(Status, (*pFnCheckMasterDevice)(this, it1->second));
		if (!CDevice::IsFunctionStatusOK(Status)) return Status;
	}


	LINKSFROMMAP &LinksFrom = Props.m_MasterLinksFrom;
	for (LINKSFROMMAPITR it2 = LinksFrom.begin(); it2 != LinksFrom.end(); it2++)
	{
		Status = CDevice::DeviceFunctionResult(Status, (*pFnCheckMasterDevice)(this, it2->second));
		if (!CDevice::IsFunctionStatusOK(Status)) return Status;
	}

	return Status;
}

#ifdef _DEBUG
	_TCHAR CDevice::UnknownVarIndex[80];
#endif