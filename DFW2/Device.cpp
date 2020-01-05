#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

CDevice::CDevice() : m_pContainer(nullptr),
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
	double *pRes(nullptr);
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

	double *pRes(nullptr);
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
		// если устройство не в матрице, возвращаем индекс "не привязано"
		ExtVar.nIndex = AssignedToMatrix() ? A(ExtVar.nIndex) : ExtVar.nIndex = CDevice::nIndexUnassigned;
	}
	else
		ExtVar.pValue = nullptr;

	return ExtVar;
}

// виртуальная функиця. Должна быть перекрыта во всех устройствах
double* CDevice::GetVariablePtr(ptrdiff_t nVarIndex)
{
	return nullptr;
}

// виртуальная функиця. Должна быть перекрыта во всех устройствах
double* CDevice::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	return nullptr;
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

	double *pRes(nullptr);
	if (m_pContainer)
		pRes = const_cast<CDevice*>(this)->GetVariablePtr(m_pContainer->GetVariableIndex(cszVarName));
	return pRes;
}

const double* CDevice::GetConstVariableConstPtr(const _TCHAR* cszVarName) const
{
	_ASSERTE(m_pContainer);

	double *pRes(nullptr);
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
			CDevice *pLinkDev(nullptr);

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
						pAlreadyLinked = nullptr;

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
							pLinkDev->IncrementLinkCounter(LinkFrom.nLinkIndex);
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
			// если у связываемого контейнера был режим мультисвязи
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
void CDevice::IncrementLinkCounter(ptrdiff_t nLinkIndex)
{
	_ASSERTE(m_pContainer);
	m_pContainer->IncrementLinkCounter(nLinkIndex, m_nInContainerIndex);
}

// устройство может быть связано с несколькими типами разных устройств,
// причем связей каждого из типов тоже может быть несколько
// функция возвращает связи устройства с заданным типом. Тип задается не явно, а в виде
// индекса слоя связей
CLinkPtrCount* CDevice::GetLink(ptrdiff_t nLinkIndex)
{
	_ASSERTE(m_pContainer);
	// возвращаем объект, в котором есть список ссылок данного устройства
	// ссылки хранятся в контейнере
	// извлекаем список ссылок по заданному типу и индексу текущего устройства
	CMultiLink& pMultiLink = m_pContainer->GetCheckLink(nLinkIndex, m_nInContainerIndex);
	return pMultiLink.GetLink(m_nInContainerIndex);
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
			p = nullptr;
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
	p = nullptr;
	return false;
}

// Определяет нужны ли уравнения для этого устройства
bool CDevice::InMatrix()
{
	return IsStateOn();
}

// функция увеличивает размерность модели на количество уравнений устройства, 
// и заодно фиксирует строку матрицы, с которой начинается блок его уравнений
void CDevice::EstimateEquations(CDynaModel *pDynaModel)
{
	m_nMatrixRow = InMatrix() ? pDynaModel->AddMatrixSize(m_pContainer->EquationsCount()) : nIndexUnassigned;
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
	for (auto&& it : m_Primitives)
		bRes = bRes && it->BuildEquations(pDynaModel);
	return bRes;
}

bool CDevice::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;
	for (auto&& it : m_Primitives)
		bRes = bRes && it->BuildRightHand(pDynaModel);
	return bRes;

}

bool CDevice::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	for (auto&& it : m_Primitives)
		bRes = bRes && it->BuildDerivatives(pDynaModel);
	return bRes;
}

void CDevice::NewtonUpdateEquation(CDynaModel *pDynaModel)
{
}

// сбрасывает указатель просмотренных ссылок устройства
void CDevice::ResetVisited()
{
	_ASSERTE(m_pContainer);
	// если списка просмотра нет - создаем его
	if (!m_pContainer->m_ppDevicesAux)
		m_pContainer->m_ppDevicesAux = make_unique<DevicePtr>(m_pContainer->Count());
	// обнуляем счетчик просмотренных ссылок
	m_pContainer->m_nVisitedCount = 0;
}

ptrdiff_t CDevice::CheckAddVisited(CDevice* pDevice)
{
	_ASSERTE(m_pContainer);

	CDevice **ppDevice = m_pContainer->m_ppDevicesAux.get();
	CDevice **ppEnd = ppDevice + m_pContainer->m_nVisitedCount;
	// просматриваем список просмотренных
	for (; ppDevice < ppEnd; ppDevice++)
		if (*ppDevice == pDevice)
			return ppDevice - m_pContainer->m_ppDevicesAux.get() ;	// если нашли заданное устройство - выходим и возвращаем номер
																	// с которым это устройство уже было добавлено
	// если дошли до конца списка и не нашли запрошенного устройства
	// добавляем его в список просмотренных
	*ppDevice = pDevice;
	// и счетчик просмотренных увеличиваем
	m_pContainer->m_nVisitedCount++;
	// возвращаем -1, так как устройство не было найдено
	return -1;
}

// проверка инициализации/инициализация устройства
eDEVICEFUNCTIONSTATUS CDevice::CheckInit(CDynaModel* pDynaModel)
{
	if (m_eInitStatus != DFS_OK)
	{
		// если успешной инициализации еще не выполнено
		// проверяем что с инициализацией ведущих устройств
		m_eInitStatus = MastersReady(&CDevice::CheckMasterDeviceInit);
		if(m_eInitStatus == DFS_OK)
			m_eInitStatus = Init(pDynaModel);		// пытаемся инициализировать устройство
		// если устройство не нуждается в инициализации
		if (m_eInitStatus == DFS_DONTNEED)	
			m_eInitStatus = DFS_OK;					// делаем вид что инициализация прошла успешно
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
	// если устройство было отключено навсегда - попытка изменения его состояния (даже отключение) вызывает исключение
	// по крайней мере для отладки
	if (m_StateCause == eDEVICESTATECAUSE::DSC_INTERNAL_PERMANENT)
		throw dfw2error(Cex(CDFW2Messages::m_cszCannotChangePermanentDeviceState, GetVerbalName()));

	m_StateCause = eStateCause;
	// если устройство отключается навсегда - обнуляем все его переменные, чтобы не было мусора
	// в результатах (мы такие устройства и так не пишем, но на всякий случай
	if (eStateCause == eDEVICESTATECAUSE::DSC_INTERNAL_PERMANENT)
		for (int i = 0; i < m_pContainer->EquationsCount(); *GetVariablePtr(i++) = 0.0);

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

// обработка разрыва устройства
eDEVICEFUNCTIONSTATUS CDevice::CheckProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_eInitStatus != DFS_OK)
	{
		// проверяем готовы ли ведущие устройства
		m_eInitStatus = MastersReady(&CheckMasterDeviceDiscontinuity);

		// если ведущие готовы, обрабатываем разрыв устройства
		if (m_eInitStatus == DFS_OK)
			m_eInitStatus = ProcessDiscontinuity(pDynaModel);

		// если устройство не требует обработки - считаем что обработано успешно
		if (m_eInitStatus == DFS_DONTNEED)
			m_eInitStatus = DFS_OK;
	}

	_ASSERTE(m_eInitStatus != DFS_FAILED);
	return m_eInitStatus;
}

// функция обработки разрыва по умолчанию
eDEVICEFUNCTIONSTATUS CDevice::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	// обрабатываем разрывы в примитивах
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	for (auto&& it : m_Primitives)
	{
		switch (it->ProcessDiscontinuity(pDynaModel))
		{
		case DFS_FAILED:
			Status = DFS_FAILED;		// если отказ - выходим сразу
			break;
		case DFS_NOTREADY:
			Status = DFS_NOTREADY;		// если не готов - выходим сразу
			break;
		}
	}
	return Status;
}

// выбор наиболее жесткого результата из двух результатов выполнения функций
eDEVICEFUNCTIONSTATUS CDevice::DeviceFunctionResult(eDEVICEFUNCTIONSTATUS Status1, eDEVICEFUNCTIONSTATUS Status2)
{
	// если один из статусов - отказ - возвращаем отказ
	if (Status1 == DFS_FAILED || Status2 == DFS_FAILED)
		return DFS_FAILED;

	// если один из статусов - не готов - возвращаем не готов
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
		bRes = GetModel()->InitExternalVariable(ExtVar, pFromDevice, cszName);
	}
	else
	{
		if (pFromDevice)
		{
			CDevice *pInitialDevice = pFromDevice;
			m_pContainer->ResetStack();
			m_pContainer->PushVarSearchStack(pFromDevice);
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
						// если устроство имеет уравнения - возвращаем индекс относительно индекса устройства, иначе - индекс "не назначено"
						ExtVar.IndexAndValue(AssignedToMatrix() ? extVar.nIndex - A(0) : CDevice::nIndexUnassigned, extVar.pValue);
						bRes = true;
					}
				}

				if (!bRes)
				{
					const SingleLinksRange& LinkRange = pFromDevice->GetSingleLinks().GetLinksRange();
					for (CDevice **ppStart = LinkRange.m_ppLinkStart; ppStart < LinkRange.m_ppLinkEnd; ppStart++)
					{
						if (*ppStart != pInitialDevice)
							m_pContainer->PushVarSearchStack(*ppStart);
					}
				}
				else
					break;
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
		m_pContainer->ResetStack();
		m_pContainer->PushVarSearchStack(pFromDevice);
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
						m_pContainer->PushVarSearchStack(*ppStart);
				}
			}
			else
				break;
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

// подробное имя устройства формируется с описанием типа. Остальное по правилам CDeviceId::UpdateVerbalName
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
		auto& FromLinks = m_pContainer->m_ContainerProps.m_LinksFrom;
		auto& itFrom = FromLinks.find(eDevType);
		if (itFrom != FromLinks.end())
			pRetDev = GetSingleLink(itFrom->second.nLinkIndex);
	
		// если связанное устройство не найдено
		// пытаемся определить связь "с другой стороны"

#ifdef _DEBUG
		CDevice *pRetDevTo(nullptr);
		LINKSTOMAP& ToLinks = m_pContainer->m_ContainerProps.m_LinksTo;
		auto& itTo = ToLinks.find(eDevType);

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
			auto& itTo = ToLinks.find(eDevType);
			if (itTo != ToLinks.end())
				pRetDev = GetSingleLink(itTo->second.nLinkIndex);
		}
#endif
	}
	return pRetDev;
}


eDEVICEFUNCTIONSTATUS CDevice::CheckMasterDeviceInit(CDevice *pDevice, LinkDirectionFrom const * pLinkFrom)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	_ASSERTE(pLinkFrom->eDependency == DPD_MASTER);

	CDevice *pDev = pDevice->GetSingleLink(pLinkFrom->nLinkIndex);
	Status = pDev->Initialized();
	_ASSERTE(Status != DFS_FAILED);
	return Status;
}


eDEVICEFUNCTIONSTATUS CDevice::CheckMasterDeviceDiscontinuity(CDevice *pDevice, LinkDirectionFrom const * pLinkFrom)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	_ASSERTE(pLinkFrom->eDependency == DPD_MASTER);
	CDevice *pDev = pDevice->GetSingleLink(pLinkFrom->nLinkIndex);

	if (pDev)
	{
		Status = pDev->DiscontinuityProcessed();
		/*
		if (CDevice::IsFunctionStatusOK(Status))
		{
			if (!pDev->IsStateOn())
			{
				pDevice->SetState(DS_OFF, DSC_INTERNAL);
			}
		}
		*/
	}

	_ASSERTE(Status != DFS_FAILED);

	return Status;
}

// проверяет готовы ли ведущие устройства
eDEVICEFUNCTIONSTATUS CDevice::MastersReady(CheckMasterDeviceFunction* pFnCheckMasterDevice)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;	// по умолчанию все хорошо
	CDeviceContainerProperties &Props = m_pContainer->m_ContainerProps;

	// use links to masters, prepared in CDynaModel::Link() instead of original links

	// если у устройсва есть ведущие устройства, проверяем, готовы ли они

	for (auto&& it : Props.m_Masters)
	{
		Status = CDevice::DeviceFunctionResult(Status, (*pFnCheckMasterDevice)(this, it));
		if (!CDevice::IsFunctionStatusOK(Status)) return Status;
	}

	return Status;
}


double CDevice::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	for (auto&& it : m_Primitives)
	{
		double rHcurrent = it->CheckZeroCrossing(pDynaModel);
		if (rHcurrent < rH)
			rH = rHcurrent;
	}
	return rH;
}



void CDevice::RegisterStatePrimitive(CDynaPrimitiveState *pPrimitive)
{
	m_StatePrimitives.push_back(pPrimitive);
}

void CDevice::RegisterPrimitive(CDynaPrimitive *pPrimitive)
{
	m_Primitives.push_back(pPrimitive);
}

void CDevice::StoreStates()
{
	for (auto&& it : m_StatePrimitives)
		it->StoreState();
}

void CDevice::RestoreStates()
{
	for (auto&& it : m_StatePrimitives)
		it->RestoreState();
}


void CDevice::DumpIntegrationStep(ptrdiff_t nId, ptrdiff_t nStepNumber)
{
	if (m_pContainer)
	{
		CDynaModel *pModel = GetModel();
		if (pModel && GetId() == nId && pModel->GetIntegrationStepNumber() == nStepNumber)
		{
			wstring FileName = Cex(_T("c:\\tmp\\%s_%d.csv"), GetVerbalName(), nStepNumber);
			FILE *flog;
			if (pModel->GetNewtonIterationNumber() == 1)
				_tunlink(FileName.c_str());
			_tfopen_s(&flog, FileName.c_str(), _T("a"));
			if (flog)
			{
				if (pModel->GetNewtonIterationNumber() == 1)
				{
					for (auto& var = m_pContainer->VariablesBegin(); var != m_pContainer->VariablesEnd(); var++)
						_ftprintf(flog, _T("%s;"), var->first.c_str());
					for (auto& var = m_pContainer->VariablesBegin(); var != m_pContainer->VariablesEnd(); var++)
						_ftprintf(flog, _T("d_%s;"), var->first.c_str());
					_ftprintf(flog, _T("\n"));
				}

				for (auto& var = m_pContainer->VariablesBegin(); var != m_pContainer->VariablesEnd(); var++)
					_ftprintf(flog, _T("%g;"), *GetVariablePtr(var->second.m_nIndex));
				for (auto& var = m_pContainer->VariablesBegin(); var != m_pContainer->VariablesEnd(); var++)
					_ftprintf(flog, _T("%g;"), pModel->GetFunction(A(var->second.m_nIndex)));
				_ftprintf(flog, _T("\n"));

				fclose(flog);
			}
		}
	}
}

CDynaModel* CDevice::GetModel()
{
	_ASSERTE(m_pContainer && m_pContainer->GetModel());
	return m_pContainer->GetModel();
}

eDEVICEFUNCTIONSTATUS CDevice::ChangeState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause)
{
	if (eState == m_State)
		return eDEVICEFUNCTIONSTATUS::DFS_DONTNEED;	// если устройство уже находится в заданном состоянии - выходим, с индикацией отсутствия необходимости

	if (m_StateCause == eDEVICESTATECAUSE::DSC_INTERNAL_PERMANENT)
	{
		Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszCannotChangePermanentDeviceState, GetVerbalName()));
		return eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	}

	if (eState == eDEVICESTATE::DS_ON)
	{
		// если устройство хотят включить - нужно проверить, все ли его masters включены. Если да - то включить, если нет - предупреждение и состояние не изменять
		CDevice *pDeviceOff(nullptr);
		for (auto&& masterdevice : m_pContainer->m_ContainerProps.m_Masters)
		{
			// если было найдено отключенное ведущее устройство - выходим
			if (pDeviceOff)
				break;

			if (masterdevice->eLinkMode == DLM_MULTI)
			{
				// если есть хотя бы одно отключенное устройство - фиксируем его и выходим из мультиссылки
				CLinkPtrCount *pLink(GetLink(masterdevice->nLinkIndex));
				CDevice **ppDevice(nullptr);
				while (pLink->In(ppDevice))
				{
					if (!(*ppDevice)->IsStateOn())
					{
						pDeviceOff = *ppDevice;
						break;
					}
				}
			}
			else
			{
				// если устройство с простой ссылки отключено - фиксируем
				CDevice *pDevice(GetSingleLink(masterdevice->nLinkIndex));
				if (pDevice && !pDevice->IsStateOn())
				{
					pDeviceOff = pDevice;
					break;
				}
			}
		}

		if (pDeviceOff)
		{
			// если зафиксировано отключенное ведущее устройство - отказываем во включении данного устройства
			Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszTurnOnDeviceImpossibleDueToMaster, GetVerbalName(), pDeviceOff->GetVerbalName()));
			return eDEVICEFUNCTIONSTATUS::DFS_NOTREADY; // отказ отключения - модель не готова
		}
		else
		{
			SetState(eState, eStateCause);
			return eDEVICEFUNCTIONSTATUS::DFS_OK;
		}
	}
	else if (eState == eDEVICESTATE::DS_OFF)
	{
		// если устройство хотят выключить - нужно выключить все его slaves и далее по дереву иерархии
		
		// обрабатываем  отключаемые устройства рекурсивно
		stack<CDevice*> offstack;
		offstack.push(this);
		while (!offstack.empty())
		{
			CDevice *pOffDevice = offstack.top();
			offstack.pop();
			pOffDevice->SetState(eState, eStateCause);
			// если отключаем не устройство, которое запросили отключить (первое в стеке), а рекурсивно отключаемое - изменяем причину отключения на внутреннюю
			eStateCause = eDEVICESTATECAUSE::DSC_INTERNAL;

			// перебираем все ведомые устройства текущего устройства из стека
			for (auto&& slavedevice : pOffDevice->m_pContainer->m_ContainerProps.m_Slaves)
			{
				if (slavedevice->eLinkMode == DLM_MULTI)
				{
					// перебираем ведомые устройства на мультиссылке
					CLinkPtrCount *pLink(pOffDevice->GetLink(slavedevice->nLinkIndex));
					CDevice **ppDevice(nullptr);
					while (pLink->In(ppDevice))
					{
						if ((*ppDevice)->IsStateOn())
						{
							// если есть включенное ведомое - помещаем в стек для отключения и дальнейшего просмотра графа связей
							Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszTurningOffDeviceByMasterDevice, (*ppDevice)->GetVerbalName(), pOffDevice->GetVerbalName()));
							offstack.push(*ppDevice);
						}
					}
				}
				else
				{
					// проверяем устройство на простой ссылке
					CDevice *pDevice(pOffDevice->GetSingleLink(slavedevice->nLinkIndex));
					if (pDevice && pDevice->IsStateOn())
					{
						Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszTurningOffDeviceByMasterDevice, pDevice->GetVerbalName(), pOffDevice->GetVerbalName()));
						offstack.push(pDevice);
					}
				}
			}
		}

		return eDEVICEFUNCTIONSTATUS::DFS_OK;
	}

	return eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
}


unique_ptr<CSerializerBase> CDevice::GetSerializer()
{
	unique_ptr<CSerializerBase> extSerializer;
	UpdateSerializer(extSerializer);
	return extSerializer;
}

void CDevice::UpdateSerializer(unique_ptr<CSerializerBase>& Serializer)
{
	if (!Serializer)
		Serializer = std::make_unique<CSerializerBase>(this);
	else
		Serializer->BeginUpdate(this);
}

#ifdef _DEBUG
	_TCHAR CDevice::UnknownVarIndex[80];
#endif