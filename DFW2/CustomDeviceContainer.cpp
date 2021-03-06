#include "stdafx.h"
#include "CustomDeviceContainer.h"
#include "CustomDevice.h"

using namespace DFW2;

CCustomDeviceContainer::CCustomDeviceContainer(CDynaModel* pDynaModel) :
	CDeviceContainer(pDynaModel),
	m_DLL(this)
{
}


CCustomDeviceContainer::~CCustomDeviceContainer()
{
	CleanUp();
}

void CCustomDeviceContainer::CleanUp()
{
	PrimitivePoolElement* pBegin = m_PrimitivePool;
	PrimitivePoolElement* pEnd = pBegin + std::size(m_PrimitivePool);
	while (pBegin < pEnd)
	{
		free(pBegin->m_pPrimitive);
		pBegin++;
	}
}

bool CCustomDeviceContainer::ConnectDLL(std::string_view DLLFilePath)
{
	bool bRes(false);
	if (m_DLL.Init(DLLFilePath))
	{
		// копируем в контейнер описания констант
		for (const auto& it : m_DLL.GetConstsInfo())
			ContainerProps_.ConstVarMap_.insert({ it.VarInfo.Name,
					CConstVarIndex(ContainerProps_.ConstVarMap_.size(),
						static_cast<eVARUNITS>(it.VarInfo.eUnits),
						eDVT_CONSTSOURCE) });

		// копируем в контейнер описания уставок
		for (const auto& it : m_DLL.GetSetPointsInfo())
			ContainerProps_.ConstVarMap_.insert({ it.Name,
				CConstVarIndex(ContainerProps_.ConstVarMap_.size(), 
					static_cast<eVARUNITS>(it.eUnits),
					eDVT_INTERNALCONST) });

		// копируем в контейнер описания внутренних переменных
		for (const auto& it : m_DLL.GetInternalsInfo())
			ContainerProps_.VarMap_.insert(std::make_pair(it.Name, CVarIndex(it.nIndex, it.bOutput ? true : false, static_cast<eVARUNITS>(it.eUnits))));

		// копируем в контейнер описания выходных переменных
		for (const auto& it : m_DLL.GetOutputsInfo())
			ContainerProps_.VarMap_.insert(std::make_pair(it.Name, CVarIndex(it.nIndex, it.bOutput ? true : false, static_cast<eVARUNITS>(it.eUnits))));

		bRes = true;
	}
	return bRes;
}


bool CCustomDeviceContainer::BuildStructure()
{
	bool bRes = true;
	if (!m_DLL.IsConnected())
		return false;

	_ASSERTE(m_DLL.GetBlocksDescriptions().size() == m_DLL.GetBlocksPinsIndexes().size());

	// можно организовать общий для контейнера пул блоков и раздавать их по устройствам
	// чтобы избежать множества мелких new

	m_nBlockEquationsCount = 0;
	m_nDoubleVarsCount = 0;
	m_nVariableIndexesCount = 0;
	m_nExternalVarsCount = 0;

	BLOCKDESCRIPTIONS::const_iterator	it = m_DLL.GetBlocksDescriptions().begin();
	BLOCKSPINSINDEXES::const_iterator  iti = m_DLL.GetBlocksPinsIndexes().begin();

	// просматриваем хост-блоки и их соединения
	for (; it != m_DLL.GetBlocksDescriptions().end(); it++, iti++)
	{
		if (it->eType > PrimitiveBlockType::PBT_UNKNOWN && it->eType < PrimitiveBlockType::PBT_LAST)
		{
			// если блок имеет правильный индекс 
			m_nBlockEquationsCount += PrimitiveEquationsCount(it->eType); // считаем количество уравнений хост-блоков
			m_PrimitivePool[it->eType].nCount++;						  // учитываем количество блоков данного типа
		}
		else
		{
			Log(DFW2::DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongPrimitiveinDLL, m_DLL.GetModuleFilePath(), it->eType));
			bRes = false;
		}
	}

	size_t nCount = Count();

	if (bRes)
	{
		// выделяем память под каждый тип хост-блока для всех устройств контейнера
		for (const auto& it :m_DLL.GetBlocksDescriptions())
		{
			if (!m_PrimitivePool[it.eType].m_pPrimitive) 
			{
				// выделение памяти для общего пула хост-блоков
				m_PrimitivePool[it.eType].m_pPrimitive = (unsigned char*)malloc(m_PrimitivePool[it.eType].nCount * nCount * PrimitiveSize(it.eType));
				// указатель на начало пула
				m_PrimitivePool[it.eType].m_pHead = m_PrimitivePool[it.eType].m_pPrimitive;
			}

		}

		m_nExternalVarsCount = GetInputsCount();

		// создаем пул для входных переменных
		m_VariableIndexExternalPool.reserve(m_nExternalVarsCount * nCount);
		// количество уравнений пользовательского устройства равно количеству уравнений для хост-блоков + количество уравнений внутренних переменных
		ContainerProps_.EquationsCount = m_nBlockEquationsCount + m_DLL.GetInternalsInfo().size();
		// общее количество double на 1 устройство = константы + уставки
		m_nDoubleVarsCount = GetConstsCount() + GetSetPointsCount();
		//  количество уравнений VariableIndexesCount
		m_nVariableIndexesCount = ContainerProps_.EquationsCount;
		// создаем пул для переменных типа double
		m_DoubleVarsPool.reserve(m_nDoubleVarsCount * nCount);
		// создаем пул для внешних переменных
		m_ExternalVarsPool.reserve(m_nExternalVarsCount * nCount);
		// создаем пул для VariableIndexes
		m_VariableIndexPool.reserve(m_nVariableIndexesCount * nCount);

		// когда все пулы подготовлены - инициализируем структуру каждого устройства контейнера
		for (auto&& it : *this)
		{
			bRes = static_cast<CCustomDevice*>(it)->BuildStructure();
			if (!bRes)
				break;
		}
	}
	return bRes;
}

bool CCustomDeviceContainer::InitDLLEquations(BuildEquationsArgs* pArgs)
{
	bool bRes = true;
	std::fill(pArgs->pEquations, pArgs->pEquations + EquationsCount(), DLLVariableIndex{0,0.0});
	bRes = m_DLL.InitEquations(pArgs);
	return bRes;
}

void CCustomDeviceContainer::BuildDLLEquations(BuildEquationsArgs* pArgs)
{
	if (!m_DLL.BuildEquations(pArgs))
		throw dfw2error(fmt::format("CCustomDeviceContainer::BuildDLLEquations - failed for {}", m_DLL.GetModuleFilePath()));
}

void CCustomDeviceContainer::BuildDLLRightHand(BuildEquationsArgs* pArgs)
{
	if(!m_DLL.BuildRightHand(pArgs))
		throw dfw2error(fmt::format("CCustomDeviceContainer::BuildDLLRightHand - failed for {}", m_DLL.GetModuleFilePath()));
}

void CCustomDeviceContainer::BuildDLLDerivatives(BuildEquationsArgs* pArgs)
{
	if(!m_DLL.BuildDerivatives(pArgs))
		throw dfw2error(fmt::format("CCustomDeviceContainer::BuildDLLDerivatives - failed for {}", m_DLL.GetModuleFilePath()));
}

void CCustomDeviceContainer::ProcessDLLDiscontinuity(BuildEquationsArgs* pArgs)
{
	if(!m_DLL.ProcessDiscontinuity(pArgs))
		throw dfw2error(fmt::format("CCustomDeviceContainer::ProcessDLLDiscontinuity - failed for {}", m_DLL.GetModuleFilePath()));
}


PrimitiveInfo CCustomDeviceContainer::GetPrimitiveInfo(PrimitiveBlockType eType)
{

	switch (eType)
	{
	case PBT_LAG:					return PrimitiveInfo(CLimitedLag::PrimitiveSize(), CLimitedLag::EquationsCount());
	case PBT_LIMITEDLAG:			return PrimitiveInfo(CLimitedLag::PrimitiveSize(), CLimitedLag::EquationsCount());
	case PBT_DERLAG:				return PrimitiveInfo(CDerlag::PrimitiveSize(), CDerlagContinuous::EquationsCount());
	case PBT_INTEGRATOR:			return PrimitiveInfo(CLimitedLag::PrimitiveSize(), CLimitedLag::EquationsCount());
	case PBT_LIMITEDINTEGRATOR:		return PrimitiveInfo(CLimitedLag::PrimitiveSize(), CLimitedLag::EquationsCount());
	case PBT_LIMITERCONST:			return PrimitiveInfo(CLimiterConst::PrimitiveSize(), CLimiterConst::EquationsCount());
	case PBT_RELAY:					return PrimitiveInfo(CRelay::PrimitiveSize(), CRelay::EquationsCount());
	case PBT_RELAYDELAY:			return PrimitiveInfo(CRelayDelay::PrimitiveSize(), CRelayDelay::EquationsCount());
	case PBT_RELAYDELAYLOGIC:		return PrimitiveInfo(CRelayDelayLogic::PrimitiveSize(), CRelayDelayLogic::EquationsCount());
	case PBT_RELAYMINDELAYLOGIC:	return PrimitiveInfo(CRelayMinDelayLogic::PrimitiveSize(), CRelayMinDelayLogic::EquationsCount());
	case PBT_RSTRIGGER:				return PrimitiveInfo(CRSTrigger::PrimitiveSize(), CRSTrigger::EquationsCount());
	case PBT_HIGHER:				return PrimitiveInfo(CComparator::PrimitiveSize(), CComparator::EquationsCount());
	case PBT_LOWER:					return PrimitiveInfo(CComparator::PrimitiveSize(), CComparator::EquationsCount());
	case PBT_BIT:					return PrimitiveInfo(CZCDetector::PrimitiveSize(), CZCDetector::EquationsCount());
	case PBT_AND:					return PrimitiveInfo(CAnd::PrimitiveSize(), CAnd::EquationsCount());
	case PBT_OR:					return PrimitiveInfo(COr::PrimitiveSize(), COr::EquationsCount());
	case PBT_NOT:					return PrimitiveInfo(CNot::PrimitiveSize(), CNot::EquationsCount());
	case PBT_ABS:					return PrimitiveInfo(CAbs::PrimitiveSize(), CAbs::EquationsCount());
	case PBT_DEADBAND:				return PrimitiveInfo(CDeadBand::PrimitiveSize(), CDeadBand::EquationsCount());
	case PBT_EXPAND:				return PrimitiveInfo(CExpand::PrimitiveSize(), CExpand::EquationsCount());
	case PBT_SHRINK:				return PrimitiveInfo(CShrink::PrimitiveSize(), CShrink::EquationsCount());
	}
	_ASSERTE(0);
	return PrimitiveInfo(-1, -1);
}


size_t CCustomDeviceContainer::PrimitiveSize(PrimitiveBlockType eType)
{
	return GetPrimitiveInfo(eType).nSize;
}

long CCustomDeviceContainer::PrimitiveEquationsCount(PrimitiveBlockType eType)
{
	return GetPrimitiveInfo(eType).nEquationsCount;
}

double* CCustomDeviceContainer::NewDoubleVariables()
{
	if (!m_nDoubleVarsCount)
		return nullptr;

	_ASSERTE(m_DoubleVarsPool.size() + m_nDoubleVarsCount <= m_DoubleVarsPool.capacity());
	m_DoubleVarsPool.resize(m_DoubleVarsPool.size() + m_nDoubleVarsCount);
	return &*(m_DoubleVarsPool.end() - m_nDoubleVarsCount);
}

VariableIndex* CCustomDeviceContainer::NewVariableIndexVariables()
{
	if (!m_nVariableIndexesCount)
		return nullptr;
	_ASSERTE(m_VariableIndexPool.size() + m_nVariableIndexesCount <= m_VariableIndexPool.capacity());
	m_VariableIndexPool.resize(m_VariableIndexPool.size() + m_nVariableIndexesCount);
	return &*(m_VariableIndexPool.end() - m_nVariableIndexesCount);
}

DLLExternalVariable* CCustomDeviceContainer::NewExternalVariables()
{
	if (!m_nExternalVarsCount)
		return nullptr;

	_ASSERTE(m_ExternalVarsPool.size() + m_nExternalVarsCount <= m_ExternalVarsPool.capacity());
	m_ExternalVarsPool.resize(m_ExternalVarsPool.size() + m_nExternalVarsCount);
	return &*(m_ExternalVarsPool.end() - m_nExternalVarsCount);
}

VariableIndexExternal* CCustomDeviceContainer::NewVariableIndexExternals()
{
	if (!m_nExternalVarsCount)
		return nullptr;
	_ASSERTE(m_VariableIndexExternalPool.size() + m_nExternalVarsCount <= m_VariableIndexExternalPool.capacity());
	m_VariableIndexExternalPool.resize(m_VariableIndexExternalPool.size() + m_nExternalVarsCount);
	return &*(m_VariableIndexExternalPool.end() - m_nExternalVarsCount);
}

void* CCustomDeviceContainer::NewPrimitive(PrimitiveBlockType eType)
{
	_ASSERTE(eType > PrimitiveBlockType::PBT_UNKNOWN && eType < PrimitiveBlockType::PBT_LAST);
	PrimitivePoolElement* pElement = m_PrimitivePool + eType;
	size_t nSize = PrimitiveSize(eType);
	_ASSERTE(pElement->m_pHead < pElement->m_pPrimitive + pElement->nCount * nSize * Count());
	void* temp = pElement->m_pHead;
	pElement->m_pHead += nSize;
	return temp;
}


long CCustomDeviceContainer::GetParametersValues(ptrdiff_t nId, BuildEquationsArgs* pArgs, long nBlockIndex, DOUBLEVECTOR& Parameters)
{
	long nCount = m_DLL.GetBlockParametersCount(nBlockIndex);

	if (nCount > 0)
	{
		Parameters.clear();
		Parameters.resize(nCount);
		if (nCount != m_DLL.GetBlockParametersValues(nBlockIndex, pArgs, &Parameters[0]))
			nCount = -1; 
	}
	return nCount;
}



CCustomDeviceCPPContainer::CCustomDeviceCPPContainer(CDynaModel* pDynaModel) : CDeviceContainer(pDynaModel) {}

CCustomDeviceCPPContainer::~CCustomDeviceCPPContainer()
{

}


void CCustomDeviceCPPContainer::ConnectDLL(std::filesystem::path DLLFilePath)
{
	m_pDLL = std::make_shared<CCustomDeviceCPPDLL>(DLLFilePath, "CustomDeviceFactory");
	CCustomDeviceDLLWrapper pDevice(m_pDLL);
	pDevice->GetDeviceProperties(ContainerProps_);
	for (const auto& prim : pDevice->GetPrimitives())
		m_PrimitivePools.CountPrimitive(prim.eBlockType);
	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszUserModelModuleLoaded, 
		stringutils::utf8_encode(DLLFilePath.c_str())));
}



void CCustomDeviceCPPContainer::BuildStructure()
{
	m_PrimitivePools.Allocate(DevVec.size());
	for (auto& dit : DevVec)
		static_cast<CCustomDeviceCPP*>(dit)->CreateDLLDeviceInstance(*this);
}
