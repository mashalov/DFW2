#include "stdafx.h"
#include "CustomDeviceDLL.h"
#include "DeviceContainer.h"

using namespace DFW2;

FARPROC CCustomDeviceDLL::GetProcAddress(const char *cszFunctionName)
{
	_ASSERTE(m_hDLL);
	FARPROC pRet = ::GetProcAddress(m_hDLL, cszFunctionName);
	if (!pRet)
		m_nGetProcAddresFailureCount++;
	return pRet;
}

void CCustomDeviceDLL::ClearFns()
{
	m_hDLL = nullptr;
	m_nGetProcAddresFailureCount = 0;
	m_pFnDestroy = nullptr;
	m_pFnGetBlocksCount = nullptr;
	m_pFnGetBlockPinsCount = nullptr;
	m_pFnGetBlockPinsIndexes = nullptr;
	m_pFnGetBlockParametersCount = nullptr;
	m_pFnGetBlockParametersValues = nullptr;
	m_pFnBuildEquations = nullptr;
	m_pFnBuildRightHand = nullptr;
	m_pFnBuildDerivatives = nullptr;
	m_pFnGetInputsCount = nullptr;
	m_pFnGetOutputsCount = nullptr;
	m_pFnGetSetPointsCount = nullptr;
	m_pFnGetConstantsCount = nullptr;
	m_pFnGetInternalsCount = nullptr;
	m_pFnGetSetPointsInfos = nullptr;
	m_pFnGetConstantsInfos = nullptr;
	m_pFnGetOutputsInfos = nullptr;
	m_pFnGetInputsInfos = nullptr;
	m_pFnGetInternalsInfos = nullptr;
	m_pFnGetTypesCount = nullptr;
	m_pFnGetTypes = nullptr;
	m_pFnGetLinksCount = nullptr;
	m_pFnGetLinks = nullptr;
	m_pFnGetDeviceTypeName = nullptr;
	m_pFnDeviceInit = nullptr;
	m_pFnProcessDiscontinuity = nullptr;

}

CCustomDeviceDLL::CCustomDeviceDLL(CDeviceContainer *pDeviceContainer)
{
	m_pDeviceContainer = pDeviceContainer;
	ClearFns();
}


CCustomDeviceDLL::~CCustomDeviceDLL()
{
	CleanUp();
}

void CCustomDeviceDLL::CleanUp()
{
	if (m_hDLL)
	{
		if (m_pFnDestroy)
			(m_pFnDestroy)();

		FreeLibrary(m_hDLL);

		ClearFns();

		m_BlockDescriptions.clear();
		m_BlockPinsIndexes.clear();

	}
}

bool CCustomDeviceDLL::Init(const _TCHAR *cszDLLFilePath)
{
	m_bConnected = false;
	m_hDLL = LoadLibrary(cszDLLFilePath);
	if (m_hDLL)
	{
		m_strModulePath = cszDLLFilePath;

		m_pFnDestroy = (DLLDESTROYPTR)GetProcAddress("Destroy");
		m_pFnGetBlocksCount = (DLLGETBLOCKSCOUNT)GetProcAddress("GetBlocksCount");
		m_pFnGetBlocksDescriptions = (DLLGETBLOCKSDESCRIPTIONS)GetProcAddress("GetBlocksDescriptions");
		m_pFnGetBlockPinsCount = (DLLGETBLOCKPINSCOUNT)GetProcAddress("GetBlockPinsCount");
		m_pFnGetBlockPinsIndexes = (DLLGETBLOCKPINSINDEXES)GetProcAddress("GetBlockPinsIndexes");
		m_pFnGetBlockParametersCount = (DLLGETBLOCKPARAMETERSCOUNT)GetProcAddress("GetBlockParametersCount");
		m_pFnGetBlockParametersValues = (DLLGETBLOCKPARAMETERSVALUES)GetProcAddress("GetBlockParametersValues");
		m_pFnBuildEquations = (DLLBUILDEQUATIONS)GetProcAddress("BuildEquations");
		m_pFnBuildRightHand = (DLLBUILDRIGHTHAND)GetProcAddress("BuildRightHand");
		m_pFnBuildDerivatives = (DLLBUILDDERIVATIVES)GetProcAddress("BuildDerivatives");
		m_pFnGetInputsCount = (DLLGETINPUTSCOUNT)GetProcAddress("GetInputsCount");
		m_pFnGetOutputsCount = (DLLGETOUTPUTSCOUNT)GetProcAddress("GetOutputsCount");
		m_pFnGetSetPointsCount = (DLLGETSETPOINTSCOUNT)GetProcAddress("GetSetPointsCount");
		m_pFnGetConstantsCount = (DLLGETCONSTANTSCOUNT)GetProcAddress("GetConstantsCount");
		m_pFnGetInternalsCount = (DLLGETINTERNALSCOUNT)GetProcAddress("GetInternalsCount");
		m_pFnGetSetPointsInfos = (DLLGETSETPOINTSINFOS)GetProcAddress("GetSetPointsInfos");
		m_pFnGetConstantsInfos = (DLLGETCONSTANTSINFOS)GetProcAddress("GetConstantsInfos");
		m_pFnGetOutputsInfos = (DLLGETOUTPUTSINFOS)GetProcAddress("GetOutputsInfos");
		m_pFnGetInputsInfos = (DLLGETINPUTSINFOS)GetProcAddress("GetInputsInfos");
		m_pFnGetInternalsInfos = (DLLGETINTERNALSINFOS)GetProcAddress("GetInternalsInfos");
		m_pFnGetTypesCount = (DLLGETTYPESCOUNT)GetProcAddress("GetTypesCount");
		m_pFnGetTypes = (DLLGETTYPES)GetProcAddress("GetTypes");
		m_pFnGetLinksCount = (DLLGETLINKSCOUNT)GetProcAddress("GetLinksCount");
		m_pFnGetLinks = (DLLGETLINKS)GetProcAddress("GetLinks");
		m_pFnGetDeviceTypeName = (DLLGETDEVICETYPENAME)GetProcAddress("GetDeviceTypeName");
		m_pFnDeviceInit = (DLLDEVICEINIT)GetProcAddress("DeviceInit");
		m_pFnProcessDiscontinuity = (DLLPROCESSDISCONTINUITY)GetProcAddress("ProcessDiscontinuity");

		if (!m_nGetProcAddresFailureCount)
			m_bConnected = true;
		else
			m_pDeviceContainer->Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDLLFuncMissing, cszDLLFilePath));
	}
	else
		m_pDeviceContainer->Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDLLLoadFailed, cszDLLFilePath));

	if (m_bConnected)
	{
		m_BlockDescriptions.clear();
		m_BlockPinsIndexes.clear();
		long nCount = (m_pFnGetBlocksCount)();
		m_BlockDescriptions.resize(nCount);
		m_BlockPinsIndexes.resize(nCount);
		m_BlockParametersCount.resize(nCount);

		if (nCount)
		{
			if (nCount == (m_pFnGetBlocksDescriptions)(&m_BlockDescriptions[0]))
			{
				for (long nIndex = 0; nIndex < nCount && m_bConnected; nIndex++)
				{
					long nInputsCount = (m_pFnGetBlockPinsCount)(nIndex);
					if (nInputsCount > 0)
					{
						m_BlockPinsIndexes[nIndex].resize(nInputsCount);
						if (nInputsCount == (m_pFnGetBlockPinsIndexes)(nIndex, &m_BlockPinsIndexes[nIndex][0]))
						{
							if (m_BlockPinsIndexes[nIndex].begin()->Location == eVL_INTERNAL)
							{
								long nParametersCount = (m_pFnGetBlockParametersCount)(nIndex);
								if (nParametersCount >= 0)
									m_BlockParametersCount[nIndex] = nParametersCount;
								else
									m_bConnected = false;
							}
							else
							{
								m_pDeviceContainer->Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszPrimitiveExternalOutput, cszDLLFilePath, m_BlockDescriptions[nIndex].Name));
								m_bConnected = false;
							}
						}
						else
						{
							m_bConnected = false;
						}
					}
					else
						m_bConnected = false;
				}
			}
			else
			{
				m_bConnected = false;
			}
		}

		if (m_bConnected && m_ConstInfos.SetVarsCount((m_pFnGetConstantsCount)()))
			if ((m_pFnGetConstantsInfos)(m_ConstInfos.m_pVarInfo) != m_ConstInfos.m_nVarsCount) m_bConnected = false;

		if (m_bConnected && m_SetPointInfos.SetVarsCount((m_pFnGetSetPointsCount)()))
			if ((m_pFnGetSetPointsInfos)(m_SetPointInfos.m_pVarInfo) != m_SetPointInfos.m_nVarsCount) m_bConnected = false;

		if (m_bConnected && m_OutputInfos.SetVarsCount((m_pFnGetOutputsCount)()))
			if ((m_pFnGetOutputsInfos)(m_OutputInfos.m_pVarInfo) != m_OutputInfos.m_nVarsCount) m_bConnected = false;

		if (m_bConnected && m_InputInfos.SetVarsCount((m_pFnGetInputsCount)()))
			if ((m_pFnGetInputsInfos)(m_InputInfos.m_pVarInfo) != m_InputInfos.m_nVarsCount) m_bConnected = false;

		if (m_bConnected && m_InternalInfos.SetVarsCount((m_pFnGetInternalsCount)()))
			if ((m_pFnGetInternalsInfos)(m_InternalInfos.m_pVarInfo) != m_InternalInfos.m_nVarsCount) m_bConnected = false;

		if (m_bConnected)
		{
			long nTypesCount = (m_pFnGetTypesCount)();
			if (nTypesCount)
			{
				std::vector<long> Types(nTypesCount);
				long *pTypes = &Types[0];
				if (nTypesCount == (m_pFnGetTypes)(pTypes))
				{
					while (nTypesCount)
					{
						m_pDeviceContainer->m_ContainerProps.SetType(static_cast<eDFW2DEVICETYPE>(*pTypes));
						pTypes++;
						nTypesCount--;
					}
				}
				else
					m_bConnected = false;
			}
			else
				m_bConnected = false;
		}

		if (m_bConnected)
		{
			long nLinksCount = (m_pFnGetLinksCount)();
			if (nLinksCount)
			{
				std::vector<LinkType> Links(nLinksCount);
				LinkType* pLinks = &Links[0];
				if (nLinksCount == (m_pFnGetLinks)(pLinks))
				{
					while (nLinksCount)
					{
						if (pLinks->eLinkType == eLINKTYPE::eLINK_FROM)
							m_pDeviceContainer->m_ContainerProps.AddLinkFrom(static_cast<eDFW2DEVICETYPE>(pLinks->eDeviceType), 
																			 static_cast<eDFW2DEVICELINKMODE>(pLinks->eLinkMode), 
																			 static_cast<eDFW2DEVICEDEPENDENCY>(pLinks->eDependency));
						else
							m_pDeviceContainer->m_ContainerProps.AddLinkTo(static_cast<eDFW2DEVICETYPE>(pLinks->eDeviceType),
																		   static_cast<eDFW2DEVICELINKMODE>(pLinks->eLinkMode),
																		   static_cast<eDFW2DEVICEDEPENDENCY>(pLinks->eDependency),
																		   pLinks->LinkField);
						pLinks++;
						nLinksCount--;
					}
				}
				else
					m_bConnected = false;
			}
			else
				m_bConnected = false;
		}

		if (m_bConnected)
			m_pDeviceContainer->m_ContainerProps.SetClassName((m_pFnGetDeviceTypeName)(), _T(""));
		
		if (!m_bConnected)
			m_pDeviceContainer->Log(DFW2::CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDLLBadBlocks, cszDLLFilePath));
	}

	if (!m_bConnected)
		CleanUp();

	return m_bConnected;
}


const BLOCKDESCRIPTIONS& CCustomDeviceDLL::GetBlocksDescriptions() const
{
	return m_BlockDescriptions;
}

const BLOCKSPINSINDEXES& CCustomDeviceDLL::GetBlocksPinsIndexes() const
{
	return m_BlockPinsIndexes;
}

const _TCHAR* CCustomDeviceDLL::GetModuleFilePath() const
{
	return m_strModulePath.c_str();
}

long CCustomDeviceDLL::GetBlockParametersCount(long nBlockIndex)
{
	if (nBlockIndex >= 0 && nBlockIndex < static_cast<long>(m_BlockParametersCount.size()))
		return m_BlockParametersCount[nBlockIndex];
	return -1;
}

long CCustomDeviceDLL::GetBlockParametersValues(long nBlockIndex, BuildEquationsArgs* pArgs, double *pValues)
{
	if (nBlockIndex >= 0 && nBlockIndex < static_cast<long>(m_BlockParametersCount.size()))
		return (m_pFnGetBlockParametersValues)(nBlockIndex, pArgs, pValues);
	return -1;
}


const ConstVarsInfo* CCustomDeviceDLL::GetConstInfo(size_t nConstIndex) const
{
	ConstVarsInfo *pVarInfo(NULL);
	if (nConstIndex >= 0 && nConstIndex < m_ConstInfos.m_nVarsCount)
		pVarInfo = m_ConstInfos.m_pVarInfo + nConstIndex;
	return pVarInfo;
}

const VarsInfo* CCustomDeviceDLL::GetSetPointInfo(size_t nSetPointIndex) const
{
	VarsInfo *pVarInfo(NULL);
	if (nSetPointIndex >= 0 && nSetPointIndex < m_SetPointInfos.m_nVarsCount)
		pVarInfo = m_SetPointInfos.m_pVarInfo + nSetPointIndex;
	return pVarInfo;
}

const VarsInfo* CCustomDeviceDLL::GetInternalInfo(size_t nInternalIndex) const
{
	VarsInfo *pVarInfo(NULL);
	if (nInternalIndex >= 0 && nInternalIndex < m_InternalInfos.m_nVarsCount)
		pVarInfo = m_InternalInfos.m_pVarInfo + nInternalIndex;
	return pVarInfo;
}

const VarsInfo* CCustomDeviceDLL::GetOutputInfo(size_t nOutputIndex) const
{
	VarsInfo *pVarInfo(NULL);
	if (nOutputIndex >= 0 && nOutputIndex < m_OutputInfos.m_nVarsCount)
		pVarInfo = m_OutputInfos.m_pVarInfo + nOutputIndex;
	return pVarInfo;
}

const InputVarsInfo* CCustomDeviceDLL::GetInputInfo(size_t nInputIndex) const
{
	InputVarsInfo *pVarInfo(NULL);
	if (nInputIndex >= 0 && nInputIndex < m_InputInfos.m_nVarsCount)
		pVarInfo = m_InputInfos.m_pVarInfo + nInputIndex;
	return pVarInfo;
}

bool CCustomDeviceDLL::IsConnected()
{
	return m_bConnected;
}


bool CCustomDeviceDLL::BuildEquations(BuildEquationsArgs *pArgs)
{
	bool bRes = false;
	long nRes = (m_pFnBuildEquations)(pArgs);
	if (nRes >= 0)
		bRes = true;
	return bRes;
}

bool CCustomDeviceDLL::InitEquations(BuildEquationsArgs *pArgs)
{
	bool bRes = false;
	long nRes = (m_pFnDeviceInit)(pArgs);
	if (nRes >= 0)
		bRes = true;
	return bRes;
}


bool CCustomDeviceDLL::ProcessDiscontinuity(BuildEquationsArgs *pArgs)
{
	bool bRes = false;
	long nRes = (m_pFnProcessDiscontinuity)(pArgs);
	if (nRes >= 0)
		bRes = true;
	return bRes;
}

bool CCustomDeviceDLL::BuildRightHand(BuildEquationsArgs *pArgs)
{
	bool bRes = false;
	long nRes = (m_pFnBuildRightHand)(pArgs);
	if (nRes >= 0)
		bRes = true;
	return bRes;
}

bool CCustomDeviceDLL::BuildDerivatives(BuildEquationsArgs *pArgs)
{
	bool bRes = false;
	long nRes = (m_pFnBuildDerivatives)(pArgs);
	if (nRes >= 0)
		bRes = true;
	return bRes;
}
