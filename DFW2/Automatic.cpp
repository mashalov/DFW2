#include "stdafx.h"
#include "Automatic.h"
#include "DynaModel.h"

using namespace DFW2;

CAutomaticItem::CAutomaticItem(long Type, ptrdiff_t Id, const _TCHAR* cszName) :
		m_nType(Type),
		m_nId(Id),
		m_strName(cszName)
{

}

CAutomaticAction::CAutomaticAction(long Type,
		ptrdiff_t Id,
		const _TCHAR* cszName,
		long LinkType,
		const _TCHAR* cszObjectClass,
		const _TCHAR* cszObjectKey,
		const _TCHAR *cszObjectProp,
		long ActionGroup,
		long OutputMode,
		long RunsCount) :

		CAutomaticItem(Type,Id,cszName),
		m_nLinkType(LinkType),
		m_strObjectClass(cszObjectClass),
		m_strObjectKey(cszObjectKey),
		m_strObjectProp(cszObjectProp),
		m_nActionGroup(ActionGroup),
		m_nOutputMode(OutputMode),
		m_nRunsCount(RunsCount),
		m_pValue(nullptr)
	{

	}

CAutomaticAction::~CAutomaticAction()
{

}

CAutomaticLogic::CAutomaticLogic(long Type,
	ptrdiff_t Id,
	const _TCHAR* cszName,
	const _TCHAR* cszActions,
	long OutputMode) :

	CAutomaticItem(Type, Id, cszName),
	m_nOutputMode(OutputMode),
	m_strActions(cszActions)
{

}

bool CAutomaticLogic::AddActionGroupId(ptrdiff_t nActionId)
{
	bool bRes = find(m_ActionGroupIds.begin(), m_ActionGroupIds.end(), nActionId) == m_ActionGroupIds.end();
	if (bRes)
		m_ActionGroupIds.push_back(nActionId);
	return bRes;
}


CAutomatic::CAutomatic(CDynaModel* pDynaModel) : m_pDynaModel(pDynaModel)
{
}

void CAutomatic::Clean()
{
	for (auto&& it : m_lstActions)
		delete it;
	for (auto&& it : m_lstLogics)
		delete it;
	m_lstActions.clear();
	m_lstLogics.clear();
	m_mapLogics.clear();
	m_AutoActionGroups.clear();
}

CAutomatic::~CAutomatic()
{
	m_spAutomaticCompiler.Release();
	CoFreeUnusedLibraries();
	Clean();
}


bool CAutomatic::PrepareCompiler()
{
	bool bRes = false;

	if (SUCCEEDED(m_spAutomaticCompiler.CreateInstance(CLSID_AutomaticCompiler)))
	{
		bRes = true;
	}
	else
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszNoCompilerDLL);

	return bRes;
}

bool CAutomatic::CompileModels()
{
	bool bRes = false;

	if (m_spAutomaticCompiler != nullptr)
	{
		try
		{
			m_spAutomaticCompiler->Compile();
			bRes = true;
		}
		catch (_com_error& ex)
		{
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, _T("%s"), ex.Description());
			variant_t vtLog = m_spAutomaticCompiler->Log;
			if ( vtLog.vt == (VT_BSTR | VT_ARRAY) )
			{
				LONG LBound, UBound;
				if (SafeArrayGetDim(vtLog.parray) == 1 &&
					SUCCEEDED(SafeArrayGetLBound(vtLog.parray, 1, &LBound)) &&
					SUCCEEDED(SafeArrayGetUBound(vtLog.parray, 1, &UBound)))
				{
					BSTR *ppData;
					if (SUCCEEDED(SafeArrayAccessData(vtLog.parray, (void**)&ppData)))
					{
						while (LBound <= UBound)
						{
							m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, _bstr_t(*ppData));
							LBound++;
							ppData++;
						}
						SafeArrayUnaccessData(vtLog.parray);
					}
				}
			}
			vtLog.Clear();
		}
	}
	else
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszNoCompilerDLL);

	return bRes;
}


bool CAutomatic::AddStarter(long Type,
							long Id,
							const _TCHAR* cszName,
							const _TCHAR* cszExpression,
							long LinkType,
							const _TCHAR* cszObjectClass,
							const _TCHAR* cszObjectKey,
							const _TCHAR* cszObjectProp)
{
	bool bRes = false;

	if (m_spAutomaticCompiler != nullptr)
	{
		m_spAutomaticCompiler->AddStarter(Type,Id,cszName,cszExpression,LinkType,cszObjectClass,cszObjectKey,cszObjectProp);
		bRes = true;
	}
	else
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszNoCompilerDLL);

	return bRes;
}


bool CAutomatic::AddLogic(long Type,
						  long Id,
						  const _TCHAR* cszName,
						  const _TCHAR* cszExpression,
						  const _TCHAR* cszActions,
						  const _TCHAR* cszDelayExpression,
						  long OutputMode)
{
	bool bRes = false;

	if (m_spAutomaticCompiler != nullptr)
	{
		m_spAutomaticCompiler->AddLogic(Type, Id, cszName, cszExpression, cszActions, cszDelayExpression, OutputMode);
		CAutomaticLogic *pNewLogic = new CAutomaticLogic(Type, Id, cszName, cszActions, OutputMode);
		m_lstLogics.push_back(pNewLogic);
		m_mapLogics.insert(std::make_pair(Id, pNewLogic));
		bRes = true;
	}
	else
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszNoCompilerDLL);

	return bRes;
}

bool CAutomatic::AddAction(long Type,
						   long Id,
						   const _TCHAR* cszName,
						   const _TCHAR* cszExpression,
						   long LinkType,
						   const _TCHAR* cszObjectClass,
						   const _TCHAR* cszObjectKey,
						   const _TCHAR *cszObjectProp,
						   long ActionGroup,
						   long OutputMode,
						   long RunsCount)
{
	bool bRes = false;

	if (m_spAutomaticCompiler != nullptr)
	{
		m_spAutomaticCompiler->AddAction(Type, Id, cszName, cszExpression, LinkType, cszObjectClass, cszObjectKey, cszObjectProp, ActionGroup, OutputMode, RunsCount);
		CAutomaticAction *pNewAction = new CAutomaticAction(Type, Id, cszName, LinkType, cszObjectClass, cszObjectKey, cszObjectProp, ActionGroup, OutputMode, RunsCount);
		m_lstActions.push_back(pNewAction);
		m_AutoActionGroups[ActionGroup].push_back(pNewAction);
		bRes = true;
	}
	else
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszNoCompilerDLL);

	return bRes;
}

void CAutomatic::Init()
{
	CDeviceContainer *pAutomaticContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_AUTOMATIC);
	if(!pAutomaticContainer)
		throw dfw2error(_T("CAutomatic::Init AutomaticContainer not available"));
	CCustomDevice *pCustomDevice = static_cast<CCustomDevice*>(pAutomaticContainer->GetDeviceByIndex(0));
	if (!pCustomDevice)
		throw dfw2error(_T("CAutomatic::Init CustomDevice for automatic not available"));

	bool bRes = true;
	STRINGLIST ActionList;

	for (auto&& it : m_lstLogics)
	{
		CAutomaticItem *pLogic= it;
		std::wstring strVarName = Cex(_T("LT%d"), pLogic->GetId());

		// находим в автоматике выходное реле элемента логики по имени выхода
		CRelayDelay *pActionRelay = static_cast<CRelayDelay*>(pCustomDevice->GetPrimitiveForNamedOutput(strVarName.c_str()));
		if (!pActionRelay)
			throw dfw2error(Cex(CDFW2Messages::m_cszLogicNotFoundInDLL, strVarName.c_str()));

		pActionRelay->SetDiscontinuityId(pLogic->GetId());
		CAutomaticLogic *pLogicItem = static_cast<CAutomaticLogic*>(pLogic);
		const std::wstring strActions = pLogicItem->GetActions();
		stringutils::split(strActions, _T(";,"), ActionList);

		for (auto&& sit : ActionList)
		{
			std::wstring strAction = sit;
			stringutils::trim(strAction);
			if (!strAction.empty())
			{
				ptrdiff_t nId(0);
				if (_stscanf_s(strAction.c_str(), CAutomaticAction::cszActionTemplate, &nId) == 1)
				{
					AUTOITEMGROUPITR mit = m_AutoActionGroups.find(nId);
					if (mit != m_AutoActionGroups.end())
					{
						if (!pLogicItem->AddActionGroupId(nId))
						{
							m_pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszDuplicateActionGroupInLogic,
								nId, strActions.c_str(), pLogicItem->GetId()));
						}
					}
					else
					{
						m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszNoActionGroupFoundInLogic,
							nId, strActions.c_str(), pLogicItem->GetId()));
						bRes = false;
					}
				}
				else
				{
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongActionInLogicList, 
																strAction.c_str(), 
																strActions.c_str(), 
																pLogic->GetId()));
					bRes = false;
				}
			}
		}
	}
			
	for (auto&& it : m_lstActions)
	{
		CAutomaticAction *pAction = static_cast<CAutomaticAction*>(it);
		if (!pAction->Init(m_pDynaModel, pCustomDevice))
			bRes = false;
	}

	if (!bRes)
		throw dfw2error(Cex(CDFW2Messages::m_cszAutomaticOrScenarioFailedToInitialize));
}


bool CAutomatic::NotifyRelayDelay(const CRelayDelayLogic* pRelay)
{
	bool bRes = false;
	_ASSERTE(pRelay);

	if (!pRelay)
		return bRes;

	if (pRelay->GetDiscontinuityId() <= 0)
		return true;

	if (*pRelay->Output() == 0)
		return true;


	AUTOITEMSMAPITR mit = m_mapLogics.find(pRelay->GetDiscontinuityId());

	_ASSERTE(mit != m_mapLogics.end());

	if (mit != m_mapLogics.end())
	{
		CAutomaticLogic *pLogic = static_cast<CAutomaticLogic*>(mit->second);
		for (CONSTINTLISTITR git = pLogic->GetGroupIds().begin(); git != pLogic->GetGroupIds().end(); git++)
		{
			AUTOITEMGROUPITR autoGroupIt = m_AutoActionGroups.find(*git);

			_ASSERTE(autoGroupIt != m_AutoActionGroups.end());

			if (autoGroupIt != m_AutoActionGroups.end())
			{
				AUTOITEMS& lstActions = autoGroupIt->second;
				for (AUTOITEMSITR autoItemIt = lstActions.begin(); autoItemIt != lstActions.end(); autoItemIt++)
				{
					CAutomaticAction *pAction = static_cast<CAutomaticAction*>(*autoItemIt);
					pAction->Do(m_pDynaModel);
					bRes = true;
				}
			}
		}
	}

	return bRes;
}

bool CAutomaticAction::Do(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (m_pAction && m_pValue && m_nRunsCount)
	{
		bRes = (m_pAction->Do(pDynaModel, *m_pValue) != AS_ERROR);
		m_nRunsCount--;
	}

	return bRes;
}

bool CAutomaticAction::Init(CDynaModel* pDynaModel, CCustomDevice *pCustomDevice)
{
	bool bRes = true;
	_ASSERTE(!m_pAction);
	_ASSERTE(!m_pValue);
	std::wstring strVarName = Cex(cszActionTemplate, m_nId);
	m_pValue = pCustomDevice->GetVariableConstPtr(strVarName.c_str());
	if (m_pValue)
	{
		switch (m_nType)
		{
			case 3:
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasBranch;
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass.c_str(), m_strObjectKey.c_str(), m_strObjectProp.c_str());
				if (pDev)
				{
					m_pAction = std::make_unique<CModelActionChangeBranchState>(static_cast<CDynaBranch*>(pDev), CDynaBranch::BRANCH_OFF);
					bRes = true;
				}
				break;
			}
			case 4:
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasNode;
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass.c_str(), m_strObjectKey.c_str(), m_strObjectProp.c_str());
				if (pDev)
				{
					m_pAction = std::make_unique<CModelActionChangeNodeShuntG>(static_cast<CDynaNode*>(pDev), *m_pValue);
					bRes = true;
				}
				break;
			}
			case 5:
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasNode;
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass.c_str(), m_strObjectKey.c_str(), m_strObjectProp.c_str());
				if (pDev)
				{
					m_pAction = std::make_unique<CModelActionChangeNodeShuntB>(static_cast<CDynaNode*>(pDev), *m_pValue);
					bRes = true;
				}
				break;
			}
			case 6:
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasNode;
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass.c_str(), m_strObjectKey.c_str(), m_strObjectProp.c_str());
				if (pDev)
				{
					m_pAction = std::make_unique<CModelActionChangeNodeShuntR>(static_cast<CDynaNode*>(pDev), *m_pValue);
					bRes = true;
				}
				break;
			}
			case 7:
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasNode;
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass.c_str(), m_strObjectKey.c_str(), m_strObjectProp.c_str());
				if (pDev)
				{
					m_pAction = std::make_unique<CModelActionChangeNodeShuntX>(static_cast<CDynaNode*>(pDev), *m_pValue);
					bRes = true;
				}
				break;
			}
		}
	}
	else
	{
		pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszActionNotFoundInDLL, strVarName.c_str()));
		bRes = false;
	}

	return bRes;
}


const _TCHAR* CAutomaticAction::cszActionTemplate = _T("A%d");