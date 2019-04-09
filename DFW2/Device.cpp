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

eDFW2DEVICETYPE CDevice::GetType() const
{
	_ASSERTE(m_pContainer);

	if (m_pContainer)
		return m_pContainer->GetType();
	else
		return eDFW2DEVICETYPE::DEVTYPE_UNKNOWN;
}


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

double* CDevice::GetVariablePtr(const _TCHAR* cszVarName)
{
	_ASSERTE(m_pContainer);

	double *pRes = NULL;
	if (m_pContainer)
		pRes = GetVariablePtr(m_pContainer->GetVariableIndex(cszVarName));
	return pRes;
}

double* CDevice::GetConstVariablePtr(const _TCHAR* cszVarName)
{
	_ASSERTE(m_pContainer);

	double *pRes = NULL;
	if (m_pContainer)
		pRes = GetConstVariablePtr(m_pContainer->GetConstVariableIndex(cszVarName));
	return pRes;
}

ExternalVariable CDevice::GetExternalVariable(const _TCHAR* cszVarName)
{
	_ASSERTE(m_pContainer);

	ExternalVariable ExtVar;
	ExtVar.nIndex = m_pContainer->GetVariableIndex(cszVarName);

	if (ExtVar.nIndex >= 0)
	{
		ExtVar.pValue = GetVariablePtr(ExtVar.nIndex);
		ExtVar.nIndex = A(ExtVar.nIndex);
	}
	else
		ExtVar.pValue = NULL;

	return ExtVar;
}

double* CDevice::GetVariablePtr(ptrdiff_t nVarIndex)
{
	return NULL;
}

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

bool CDevice::LinkToContainer(CDeviceContainer *pContainer, CDeviceContainer *pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom)
{
	bool bRes = false;
	if (m_pContainer && pContLead)
	{
		bRes = true;
		DEVICEVECTORITR it;
		CDeviceContainer *pContSlave = (pContLead == pContainer) ? m_pContainer : pContainer;

		for (it = pContLead->begin(); it != pContLead->end(); it++)
		{
			CDevice *pDev = *it;

			/*
			if (!pDev->SetSingleLink(LinkTo.nLinkIndex, NULL))
			{
				bRes = false;
				break;
			}
			*/

			CDevice *pLinkDev = NULL;

			const double *pdDevId = pDev->GetConstVariableConstPtr(LinkTo.strIdField.c_str());
			ptrdiff_t DevId = (pdDevId) ? static_cast<ptrdiff_t>(*pdDevId) : -1;
			if (DevId > 0)
			{
				if (pContSlave->GetDevice(DevId, pLinkDev))
				{
					CDevice *pAlreadyLinked = pDev->GetSingleLink(LinkTo.nLinkIndex);

					// block already linked count for multilink
					if (LinkTo.eLinkMode == DLM_MULTI)
						pAlreadyLinked = NULL;

					if (!pAlreadyLinked)
					{
						if (!pDev->SetSingleLink(LinkTo.nLinkIndex, pLinkDev))
						{
							bRes = false;
							break;
						}

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
							bRes = pLinkDev->IncrementLinkCounter(LinkFrom.nLinkIndex) && bRes;
						}
					}
					else
					{
						pDev->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDeviceAlreadyLinked, pDev->GetVerbalName(), pLinkDev->GetVerbalName(), pAlreadyLinked->GetVerbalName()));
						bRes = false;
					}
				}
				else
				{
					pDev->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDeviceForDeviceNotFound, DevId, pDev->GetVerbalName()));
					bRes = false;
				}
			}
		}

		if (bRes && LinkFrom.eLinkMode == DLM_MULTI)
		{
			pContSlave->AllocateLinks(LinkFrom.nLinkIndex);
			for (it = pContLead->begin(); it != pContLead->end(); it++)
			{
				CDevice *pDev = *it;
				CDevice *pDevLinked = pDev->GetSingleLink(LinkTo.nLinkIndex);
				if (pDevLinked)
					pContSlave->AddLink(LinkFrom.nLinkIndex, pDevLinked->m_nInContainerIndex, pDev);
			}
			pContSlave->RestoreLinks(LinkFrom.nLinkIndex);

		}
	}
	return bRes;
}

bool CDevice::IncrementLinkCounter(ptrdiff_t nLinkIndex)
{
	_ASSERTE(m_pContainer);

	bool bRes = false;
	if (m_pContainer)
		bRes = m_pContainer->IncrementLinkCounter(nLinkIndex, m_nInContainerIndex);
	return bRes;
}

CLinkPtrCount* CDevice::GetLink(ptrdiff_t nLinkIndex)
{
	_ASSERTE(m_pContainer);

	CLinkPtrCount *pLink(NULL);
	if (m_pContainer)
	{
		CMultiLink *pMultiLink = m_pContainer->GetCheckLink(nLinkIndex, m_nInContainerIndex);
		if (pMultiLink)
			pLink = &pMultiLink->m_pLinkInfo[m_nInContainerIndex];
	}
	return pLink;
}

bool CLinkPtrCount::In(CDevice ** & p)
{
	if (!p)
	{
		if (m_nCount)
		{
			p = m_pPointer;
			return true;
		}
		else
		{
			p = NULL;
			return false;
		}
	}

	p++;

	if (p < m_pPointer + m_nCount)
		return true;

	p = NULL;

	return false;
}

void CDevice::EstimateEquations(CDynaModel *pDynaModel)
{
	m_nMatrixRow = pDynaModel->AddMatrixSize(m_pContainer->EquationsCount());
}

void CDevice::InitNordsiek(CDynaModel* pDynaModel)
{
	_ASSERTE(m_pContainer);

	struct RightVector *pRv = pDynaModel->GetRightVector(A(0));
	ptrdiff_t nEquationsCount = m_pContainer->EquationsCount();

	for (ptrdiff_t z = 0; z < nEquationsCount ; z++)
	{
		pRv->pValue = GetVariablePtr(z);
		_ASSERTE(pRv->pValue);
		pRv->pDevice = this;
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

void CDevice::ResetVisited()
{
	_ASSERTE(m_pContainer);

	if (!m_pContainer->m_ppDevicesAux)
		m_pContainer->m_ppDevicesAux = new CDevice*[m_pContainer->Count()];
	m_pContainer->m_nVisitedCount = 0;
}

bool CDevice::CheckAddVisited(CDevice* pDevice)
{
	_ASSERTE(m_pContainer);

	bool bRes = false;
	CDevice **ppDevice = m_pContainer->m_ppDevicesAux;
	CDevice **ppEnd = ppDevice + m_pContainer->m_nVisitedCount;
	for (; ppDevice < ppEnd; ppDevice++)
		if (*ppDevice == pDevice)
			break;
	if (ppDevice == ppEnd)
	{
		*ppDevice = pDevice;
		m_pContainer->m_nVisitedCount++;
		bRes = true;
	}
	return bRes;
}

eDEVICEFUNCTIONSTATUS CDevice::CheckInit(CDynaModel* pDynaModel)
{
	if (m_eInitStatus != DFS_OK)
	{
		m_eInitStatus = MastersReady(&CDevice::CheckMasterDeviceInit);

		if (IsPresent())
		{
			if(m_eInitStatus == DFS_OK)
				m_eInitStatus = Init(pDynaModel);

			if (m_eInitStatus == DFS_DONTNEED)
				m_eInitStatus = DFS_OK;
		}
		else
			m_eInitStatus = DFS_OK;
	}
	return m_eInitStatus;
}

bool CDevice::InitExternalVariables(CDynaModel *pDynaModel)
{
	return true;
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


void CDevice::UpdateVerbalName()
{
	CDeviceId::UpdateVerbalName();
	if (m_pContainer)
		m_strVerbalName = Cex(_T("%s %s"), m_pContainer->GetTypeName(), m_strVerbalName.c_str());
}


CDevice* CDevice::GetSingleLink(ptrdiff_t nIndex)
{
	return m_DeviceLinks.GetLink(nIndex);
}

CDevice* CDevice::GetSingleLink(eDFW2DEVICETYPE eDevType)
{
	_ASSERTE(m_pContainer);

	CDevice *pRetDev = NULL;

	if (m_pContainer)
	{
		LINKSFROMMAP& FromLinks = m_pContainer->m_ContainerProps.m_LinksFrom;
		LINKSFROMMAPITR itFrom = FromLinks.find(eDevType);
		if (itFrom != FromLinks.end())
			pRetDev = GetSingleLink(itFrom->second.nLinkIndex);

#ifdef _DEBUG
		CDevice *pRetDevTo = NULL;
		LINKSTOMAP& ToLinks = m_pContainer->m_ContainerProps.m_LinksTo;
		LINKSTOMAPITR itTo = ToLinks.find(eDevType);

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