#include "stdafx.h"
#include "Automatic.h"
#include "DynaModel.h"

using namespace DFW2;

CAutomaticItem::CAutomaticItem(long Type, ptrdiff_t Id, std::wstring_view Name) :
		m_nType(Type),
		m_nId(Id),
		m_strName(Name)
{

}

CAutomaticAction::CAutomaticAction(long Type,
		ptrdiff_t Id,
	    std::wstring_view Name,
		long LinkType,
		std::wstring_view ObjectClass,
		std::wstring_view ObjectKey,
		std::wstring_view ObjectProp,
		long ActionGroup,
		long OutputMode,
		long RunsCount) :

		CAutomaticItem(Type,Id,Name),
		m_nLinkType(LinkType),
		m_strObjectClass(ObjectClass),
		m_strObjectKey(ObjectKey),
		m_strObjectProp(ObjectProp),
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
	std::wstring_view Name,
	std::wstring_view Actions,
	long OutputMode) :

	CAutomaticItem(Type, Id, Name),
	m_nOutputMode(OutputMode),
	m_strActions(Actions)
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

	std::wofstream src;
	src.open(_T("c:\\tmp\\auto.cmp"), std::fstream::out);
	if (src.is_open())
	{
		src << _T("main\n{\n") << source.str() << _T("}\n");
		src.close();
		auto pCompiler = std::make_shared<CCompilerDLL>();
#ifdef _DEBUG
		pCompiler->Init(_T("C:\\Users\\masha\\source\\repos\\AntlrCPP\\Debug DLL\\AntlrCPP.dll"), "CompilerFactory");
#else
		pCompiler->Init(_T("C:\\Users\\masha\\source\\repos\\AntlrCPP\\Release DLL\\AntlrCPP.dll"), "CompilerFactory");
#endif 
		
		std::stringstream Sourceutf8stream;
		CDLLInstanceWrapper<CCompilerDLL> Compiler(pCompiler);
		CDynaModel* pDynaModel(m_pDynaModel);
		
		/*
		Compiler->SetMessageCallBacks(
				MessageCallBacks
				{
					[&pDynaModel](std::string_view message)
					{
						pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, stringutils::utf8_decode(std::string(message)));
						return true;
					},
					[&pDynaModel](std::string_view message)
					{
						pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, stringutils::utf8_decode(std::string(message)));
						return true;
					},
					[&pDynaModel](std::string_view message)
					{
						pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, stringutils::utf8_decode(std::string(message)));
						return true;
					}
				}
		);
		*/

		Sourceutf8stream <<"main\n{\n" << stringutils::utf8_encode(source.str()) << "}\n";
#ifdef _WIN64
		Compiler->SetProperty("Platform", "x64");
#else
		Compiler->SetProperty("Platform", "Win32");
#endif 
#ifdef _DEBUG
		Compiler->SetProperty("Configuration", "Debug");
		Compiler->SetProperty("OutputPath", "c:\\tmp\\auto\\debug\\");
		Compiler->SetProperty("DllLibraryPath", "C:\\tmp\\CustomModels\\dll\\debug\\");
#else
		Compiler->SetProperty("Configuration", "Release");
		Compiler->SetProperty("OutputPath", "c:\\tmp\\auto\\release\\");
		Compiler->SetProperty("DllLibraryPath", "C:\\tmp\\CustomModels\\dll\\release\\");
#endif
		
		Compiler->SetProperty("ProjectName", "automatic");

		if (!Compiler->Compile(Sourceutf8stream))
			throw dfw2error(_T("ќшибка компил€ции"));

		pathAutomaticDLL = Compiler->GetProperty("DllLibraryPath");
		pathAutomaticDLL.append(Compiler->GetProperty("ProjectName")).replace_extension(".dll");
	}

	if (m_spAutomaticCompiler != nullptr)
	{
		try
		{
			m_spAutomaticCompiler->Compile();
			bRes = true;
		}
		catch (_com_error& ex)
		{
			_bstr_t desc(ex.Description());
			std::wstring out;
			if (desc.length())
				out = static_cast<const _TCHAR*>(desc);


			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(_T("{}"), out));
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
							m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, static_cast<const _TCHAR*>(_bstr_t(*ppData)));
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


#define COMSTR(a) std::wstring((a)).c_str()

bool CAutomatic::AddStarter(long Type,
							long Id,
							std::wstring_view Name,
							std::wstring_view Expression,
							long LinkType,
							std::wstring_view ObjectClass,
							std::wstring_view ObjectKey,
						    std::wstring_view ObjectProp)
{
	bool bRes = false;

	source << fmt::format(_T("S{} = starter({}, {})\n"), Id, Expression, ObjectClass);

	if (m_spAutomaticCompiler != nullptr)
	{
		m_spAutomaticCompiler->AddStarter(Type,Id, COMSTR(Name), COMSTR(Expression), LinkType, COMSTR(ObjectClass), COMSTR(ObjectKey), COMSTR(ObjectProp));
		bRes = true;
	}
	else
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszNoCompilerDLL);

	return bRes;
}


bool CAutomatic::AddLogic(long Type,
						  long Id,
						  std::wstring_view Name,
						  std::wstring_view Expression,
						  std::wstring_view Actions,
						  std::wstring_view DelayExpression,
						  long OutputMode)
{
	bool bRes = false;

	source << fmt::format(_T("L{} = {}\n"), Id, Expression);
	source << fmt::format(_T("LT{} = relay(L{}, 0.0, {})\n"), Id, Id, DelayExpression);

	if (m_spAutomaticCompiler != nullptr)
	{
		m_spAutomaticCompiler->AddLogic(Type, Id, COMSTR(Name), COMSTR(Expression), COMSTR(Actions), COMSTR(DelayExpression), OutputMode);
		m_lstLogics.push_back(std::make_unique<CAutomaticLogic>(Type, Id, Name, Actions, OutputMode));
		m_mapLogics.insert(std::make_pair(Id, m_lstLogics.back().get()));
		bRes = true;
	}
	else
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszNoCompilerDLL);

	return bRes;
}

bool CAutomatic::AddAction(long Type,
						   long Id,
						   std::wstring_view Name,
						   std::wstring_view Expression,
						   long LinkType,
						   std::wstring_view ObjectClass,
						   std::wstring_view ObjectKey,
						   std::wstring_view ObjectProp,
						   long ActionGroup,
						   long OutputMode,
						   long RunsCount)
{
	bool bRes = false;

	source << fmt::format(_T("A{} = {}\n"), Id, Expression);

	if (m_spAutomaticCompiler != nullptr)
	{
		m_spAutomaticCompiler->AddAction(Type, Id, COMSTR(Name), 
												   COMSTR(Expression), 
											       LinkType, COMSTR(ObjectClass), COMSTR(ObjectKey), 
												   COMSTR(ObjectProp), ActionGroup, OutputMode, RunsCount);
		m_lstActions.push_back(std::make_unique<CAutomaticAction>(Type, Id, Name, LinkType, ObjectClass, ObjectKey, ObjectProp, ActionGroup, OutputMode, RunsCount));
		m_AutoActionGroups[ActionGroup].push_back(m_lstActions.back().get());
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
		CAutomaticItem *pLogic= it.get();
		std::wstring strVarName = fmt::format(_T("LT{}"), pLogic->GetId());

		// находим в автоматике выходное реле элемента логики по имени выхода
		CRelayDelay *pActionRelay = static_cast<CRelayDelay*>(pCustomDevice->GetPrimitiveForNamedOutput(strVarName.c_str()));
		if (!pActionRelay)
			throw dfw2error(fmt::format(CDFW2Messages::m_cszLogicNotFoundInDLL, strVarName));

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
				if (_stscanf_s(strAction.c_str(), _T("A%td"), &nId) == 1)
				{
					auto mit = m_AutoActionGroups.find(nId);
					if (mit != m_AutoActionGroups.end())
					{
						if (!pLogicItem->AddActionGroupId(nId))
						{
							m_pDynaModel->Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszDuplicateActionGroupInLogic,
																							nId, 
																							strActions, 
																							pLogicItem->GetId()));
						}
					}
					else
					{
						m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszNoActionGroupFoundInLogic,
																					nId, 
																					strActions, 
																					pLogicItem->GetId()));
						bRes = false;
					}
				}
				else
				{
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongActionInLogicList, 
																	strAction, 
																	strActions, 
																	pLogic->GetId()));
					bRes = false;
				}
			}
		}
	}
			
	for (auto&& it : m_lstActions)
	{
		CAutomaticAction *pAction = static_cast<CAutomaticAction*>(it.get());
		if (!pAction->Init(m_pDynaModel, pCustomDevice))
			bRes = false;
	}

	if (!bRes)
		throw dfw2error(fmt::format(CDFW2Messages::m_cszAutomaticOrScenarioFailedToInitialize));
}


bool CAutomatic::NotifyRelayDelay(const CRelayDelayLogic* pRelay)
{
	bool bRes = false;
	_ASSERTE(pRelay);

	if (!pRelay)
		return bRes;

	if (pRelay->GetDiscontinuityId() <= 0)
		return true;

	if (pRelay == 0)
		return true;


	auto mit = m_mapLogics.find(pRelay->GetDiscontinuityId());

	_ASSERTE(mit != m_mapLogics.end());

	if (mit != m_mapLogics.end())
	{
		CAutomaticLogic *pLogic = static_cast<CAutomaticLogic*>(mit->second);
		for (auto git : pLogic->GetGroupIds())
		{
			auto autoGroupIt = m_AutoActionGroups.find(git);

			_ASSERTE(autoGroupIt != m_AutoActionGroups.end());

			if (autoGroupIt != m_AutoActionGroups.end())
			{
				auto& lstActions = autoGroupIt->second;
				for (auto & item : lstActions)
				{
					static_cast<CAutomaticAction*>(item)->Do(m_pDynaModel);
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
	std::wstring strVarName = fmt::format(cszActionTemplate, m_nId);
	m_pValue = pCustomDevice->GetVariableConstPtr(strVarName.c_str());
	if (m_pValue)
	{
		switch (m_nType)
		{
			case 3:
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasBranch;
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, m_strObjectProp);
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
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, m_strObjectProp);
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
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, m_strObjectProp);
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
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, m_strObjectProp);
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
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, m_strObjectProp);
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
		pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszActionNotFoundInDLL, strVarName));
		bRes = false;
	}

	return bRes;
}


const _TCHAR* CAutomaticAction::cszActionTemplate = _T("A{}");