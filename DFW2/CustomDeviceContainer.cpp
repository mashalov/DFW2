#include "stdafx.h"
#include "CustomDeviceContainer.h"
#include "CustomDevice.h"

using namespace DFW2;

CCustomDeviceContainer::CCustomDeviceContainer(CDynaModel* pDynaModel) :
	CDeviceContainer(pDynaModel),
	m_DLL(this),
	m_pPrimitiveVarsPool(nullptr),
	m_pDoubleVarsPool(nullptr),
	m_pPrimitiveExtVarsPool(nullptr),
	m_pExternalVarsPool(nullptr)
{
}


CCustomDeviceContainer::~CCustomDeviceContainer()
{
	CleanUp();
}

void CCustomDeviceContainer::CleanUp()
{
	PrimitivePoolElement* pBegin = m_PrimitivePool;
	PrimitivePoolElement* pEnd = pBegin + PrimitiveBlockType::PBT_LAST;
	while (pBegin < pEnd)
	{
		free(pBegin->m_pPrimitive);
		pBegin++;
	}

	free(m_pPrimitiveVarsPool);
	free(m_pPrimitiveExtVarsPool);
}

bool CCustomDeviceContainer::ConnectDLL(const _TCHAR* cszDLLFilePath)
{
	bool bRes = false;
	if (m_DLL.Init(cszDLLFilePath))
	{
		bRes = true;

		size_t nConstsCount = m_DLL.GetConstsCount();
		for (size_t nConstIndex = 0; nConstIndex < nConstsCount; nConstIndex++)
		{
			const ConstVarsInfo* pConstInfo = m_DLL.GetConstInfo(nConstIndex);
			m_ContainerProps.m_ConstVarMap.insert(std::make_pair(pConstInfo->VarInfo.Name,
				CConstVarIndex(m_ContainerProps.m_ConstVarMap.size(), eDVT_CONSTSOURCE)));
		}

		size_t nSetPointsCount = m_DLL.GetSetPointsCount();
		for (size_t nSetPointndex = 0; nSetPointndex < nSetPointsCount; nSetPointndex++)
		{
			const VarsInfo* pVarInfo = m_DLL.GetSetPointInfo(nSetPointndex);
			m_ContainerProps.m_ConstVarMap.insert(std::make_pair(pVarInfo->Name,
				CConstVarIndex(m_ContainerProps.m_ConstVarMap.size(), eDVT_INTERNALCONST)));
		}

		size_t nInternalsCount = m_DLL.GetInternalsCount();
		for (size_t nInternalsIndex = 0; nInternalsIndex < nInternalsCount; nInternalsIndex++)
		{
			const VarsInfo* pVarInfo = m_DLL.GetInternalInfo(nInternalsIndex);
			m_ContainerProps.m_VarMap.insert(std::make_pair(pVarInfo->Name,
				CVarIndex(pVarInfo->nIndex, pVarInfo->bOutput ? true : false, static_cast<eVARUNITS>(pVarInfo->eUnits))));
		}

		size_t nOutputsCount = m_DLL.GetOutputsCount();
		for (size_t nOutputIndex = 0; nOutputIndex < nOutputsCount; nOutputIndex++)
		{
			const VarsInfo* pVarInfo = m_DLL.GetOutputInfo(nOutputIndex);
			m_ContainerProps.m_VarMap.insert(std::make_pair(pVarInfo->Name,
				CVarIndex(pVarInfo->nIndex, pVarInfo->bOutput ? true : false, static_cast<eVARUNITS>(pVarInfo->eUnits))));
		}
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
	m_nPrimitiveVarsCount = 0;
	m_nDoubleVarsCount = 0;
	m_nExternalVarsCount = 0;

	BLOCKDESCRIPTIONS::const_iterator	it = m_DLL.GetBlocksDescriptions().begin();
	BLOCKSPINSINDEXES::const_iterator  iti = m_DLL.GetBlocksPinsIndexes().begin();

	for (; it != m_DLL.GetBlocksDescriptions().end(); it++, iti++)
	{
		if (it->eType > PrimitiveBlockType::PBT_UNKNOWN && it->eType < PrimitiveBlockType::PBT_LAST)
		{
			m_nBlockEquationsCount += PrimitiveEquationsCount(it->eType);

			m_PrimitivePool[it->eType].nCount++;
			for (BLOCKPININDEXVECTOR::const_iterator ciit = iti->begin(); ciit != iti->end(); ciit++)
			{
				switch (ciit->Location)
				{
				case eVL_INTERNAL:
					m_nPrimitiveVarsCount++;
					break;
				case eVL_EXTERNAL:
					;
				}
			}
		}
		else
		{
			Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongPrimitiveinDLL, m_DLL.GetModuleFilePath(), it->eType));
			bRes = false;
		}
	}

	size_t nCount = Count();

	if (bRes)
	{
		for (BLOCKDESCRIPTIONS::const_iterator it = m_DLL.GetBlocksDescriptions().begin(); it != m_DLL.GetBlocksDescriptions().end(); it++)
		{
			if (!m_PrimitivePool[it->eType].m_pPrimitive) // get memory for each type only once
			{
				m_PrimitivePool[it->eType].m_pPrimitive = (unsigned char*)malloc(m_PrimitivePool[it->eType].nCount * nCount * PrimitiveSize(it->eType));
				m_PrimitivePool[it->eType].m_pHead = m_PrimitivePool[it->eType].m_pPrimitive;
			}

		}

		m_nExternalVarsCount = GetInputsCount();

		m_pPrimitiveVarsPool = (PrimitiveVariable*)malloc(m_nPrimitiveVarsCount * nCount * sizeof(PrimitiveVariable));
		m_pPrimitiveExtVarsPool = (PrimitiveVariableExternal*)malloc(m_nExternalVarsCount * nCount * sizeof(PrimitiveVariableExternal));

		m_ContainerProps.nEquationsCount = m_nBlockEquationsCount + m_DLL.GetInternalsCount();
		m_nDoubleVarsCount = GetConstsCount() + GetSetPointsCount() + m_ContainerProps.nEquationsCount;

		m_pDoubleVarsPool = std::make_unique<double[]>(m_nDoubleVarsCount * nCount);
		m_pExternalVarsPool = std::make_unique<ExternalVariable[]>(m_nExternalVarsCount * nCount);

		m_pPrimitiveVarsHead = m_pPrimitiveVarsPool;
		m_pPrimitiveExtVarsHead = m_pPrimitiveExtVarsPool;
		m_pDoubleVarsHead = m_pDoubleVarsPool.get();
		m_pExternalVarsHead = m_pExternalVarsPool.get();

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
	std::fill(pArgs->pEquations, pArgs->pEquations + EquationsCount(), 0.0);
	bRes = m_DLL.InitEquations(pArgs);
	return bRes;
}

bool CCustomDeviceContainer::BuildDLLEquations(BuildEquationsArgs* pArgs)
{
	bool bRes = true;
	bRes = m_DLL.BuildEquations(pArgs);
	return bRes;
}

bool CCustomDeviceContainer::BuildDLLRightHand(BuildEquationsArgs* pArgs)
{
	bool bRes = true;
	bRes = m_DLL.BuildRightHand(pArgs);
	return bRes;
}

bool CCustomDeviceContainer::BuildDLLDerivatives(BuildEquationsArgs* pArgs)
{
	bool bRes = true;
	bRes = m_DLL.BuildDerivatives(pArgs);
	return bRes;
}

bool CCustomDeviceContainer::ProcessDLLDiscontinuity(BuildEquationsArgs* pArgs)
{
	bool bRes = true;
	bRes = m_DLL.ProcessDiscontinuity(pArgs);
	return bRes;
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

PrimitiveVariable* CCustomDeviceContainer::NewPrimitiveVariable(ptrdiff_t nIndex, double& Value)
{
	new(m_pPrimitiveVarsHead)PrimitiveVariable(nIndex, Value);
	PrimitiveVariable* temp = m_pPrimitiveVarsHead;
	_ASSERTE(m_pPrimitiveVarsHead < m_pPrimitiveVarsPool + m_nPrimitiveVarsCount * Count());
	m_pPrimitiveVarsHead++;
	return temp;
}

double* CCustomDeviceContainer::NewDoubleVariables()
{
	new(m_pDoubleVarsHead) double[m_nDoubleVarsCount];
	double* temp = m_pDoubleVarsHead;
	_ASSERTE(m_pDoubleVarsHead <= m_pDoubleVarsPool.get() + m_nDoubleVarsCount * Count());
	m_pDoubleVarsHead += m_nDoubleVarsCount;
	return temp;
}

ExternalVariable* CCustomDeviceContainer::NewExternalVariables()
{
	new(m_pExternalVarsHead) ExternalVariable[m_nExternalVarsCount];
	ExternalVariable* temp = m_pExternalVarsHead;
	_ASSERTE(m_pExternalVarsHead <= m_pExternalVarsPool.get() + m_nExternalVarsCount * Count());
	m_pExternalVarsHead += m_nExternalVarsCount;
	return temp;
}

PrimitiveVariableExternal* CCustomDeviceContainer::NewPrimitiveExtVariables()
{
	new(m_pPrimitiveExtVarsHead)PrimitiveVariableExternal[m_nExternalVarsCount];
	PrimitiveVariableExternal* temp = m_pPrimitiveExtVarsHead;
	_ASSERTE(m_pPrimitiveExtVarsHead <= m_pPrimitiveExtVarsPool + m_nExternalVarsCount * Count());
	m_pPrimitiveExtVarsHead += m_nExternalVarsCount;
	return temp;
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


long CCustomDeviceContainer::GetParametersValues(ptrdiff_t nId, BuildEquationsArgs* pArgs, long nBlockIndex, double **ppParameters)
{
	long nCount = m_DLL.GetBlockParametersCount(nBlockIndex);

	if (nCount > 0)
	{
		if (nCount > static_cast<long>(m_ParameterBuffer.size()))
			m_ParameterBuffer.resize(max(nCount, 100));
		
		if (nCount != m_DLL.GetBlockParametersValues(nBlockIndex, pArgs, &m_ParameterBuffer[0]))
			nCount = -1; 
	}

	if (nCount <= 0)
		ppParameters = nullptr;
	else
		*ppParameters = &m_ParameterBuffer[0];

	return nCount;
}
