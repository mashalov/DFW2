#include "stdafx.h"
#include "Automatic.h"
#include "DynaModel.h"

using namespace DFW2;

std::string CAutomaticItem::GetVerbalName()
{
	return fmt::format("{} - \"{}\"", m_nId, m_strName);
}


CAutomaticItem::CAutomaticItem(long Type, ptrdiff_t Id, std::string_view Name) :
		m_nType(Type),
		m_nId(Id),
		m_strName(Name)
{

}

CAutomaticAction::CAutomaticAction(long Type,
		ptrdiff_t Id,
	    std::string_view Name,
		long LinkType,
		std::string_view ObjectClass,
		std::string_view ObjectKey,
		std::string_view ObjectProp,
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

CAutomaticLogic::CAutomaticLogic(long Type,
	ptrdiff_t Id,
	std::string_view Name,
	std::string_view Actions,
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
#ifdef _MSC_VER
	CoFreeUnusedLibraries();
#endif
	Clean();
}

void CAutomatic::CompileModels()
{
	std::ofstream src;
	const std::filesystem::path& root(m_pDynaModel->Platform().Automatic());
	std::filesystem::path autoFile(root);
	autoFile.append("auto.cmp");


	src.open(autoFile, std::fstream::out);
	if (src.is_open())
	{
		src << "main\n{\n" << source.str() << "}\n";
		src.close();
		auto pCompiler = std::make_shared<CCompilerDLL>("UMC.dll", "CompilerFactory");
		
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

		Sourceutf8stream <<"main\n{\n" << source.str() << "}\n";
		Compiler->SetProperty("Platform", m_pDynaModel->Platform().Platform());
		Compiler->SetProperty("Configuration", m_pDynaModel->Platform().Configuration());
		Compiler->SetProperty("OutputPath", m_pDynaModel->Platform().AutomaticBuild().string());
		Compiler->SetProperty("DllLibraryPath", m_pDynaModel->Platform().AutomaticModules().string());
		Compiler->SetProperty("ProjectName", m_pDynaModel->Platform().AutomaticModuleName());
		Compiler->SetProperty("ReferenceSources", m_pDynaModel->Platform().SourceReference().string());

		if (!Compiler->Compile(Sourceutf8stream))
			throw dfw2error("Ошибка компиляции");

		pathAutomaticModule = Compiler->CompiledModulePath();
		//pathAutomaticDLL.append(Compiler->GetProperty("ProjectName")).replace_extension(m_pDynaModel->Platform().ModuleExtension());
	}
}


#define COMSTR(a) std::string((a)).c_str()

bool CAutomatic::AddStarter(long Type,
							long Id,
							std::string_view Name,
							std::string_view Expression,
							long LinkType,
							std::string_view ObjectClass,
							std::string_view ObjectKey,
						    std::string_view ObjectProp)
{
	source << fmt::format("S{} = starter({}, {}[{}].{})\n", Id, 
		Expression.empty() ? "V" : Expression,
		ObjectClass, 
		ObjectKey, 
		ObjectProp);

	return true;
}


bool CAutomatic::AddLogic(long Type,
						  long Id,
						  std::string_view Name,
						  std::string_view Expression,
						  std::string_view Actions,
						  std::string_view DelayExpression,
						  long OutputMode)
{
	source << fmt::format("L{} = {}\n", Id, Expression);
	source << fmt::format("LT{} = relay(L{}, 0.0, {})\n", Id, Id, DelayExpression.empty() ? "0" : DelayExpression);

	m_lstLogics.push_back(std::make_unique<CAutomaticLogic>(Type, Id, Name, Actions, OutputMode));
	m_mapLogics.insert(std::make_pair(Id, m_lstLogics.back().get()));

	return true;
}

bool CAutomatic::AddAction(long Type,
						   long Id,
						   std::string_view Name,
						   std::string_view Expression,
						   long LinkType,
						   std::string_view ObjectClass,
						   std::string_view ObjectKey,
						   std::string_view ObjectProp,
						   long ActionGroup,
						   long OutputMode,
						   long RunsCount)
{
	source << fmt::format("A{} = action({}, {}[{}].{})\n", Id,
		Expression.empty() ? "V" : Expression,
		ObjectClass,
		ObjectKey,
		ObjectProp);

	m_lstActions.push_back(std::make_unique<CAutomaticAction>(Type, Id, Name, LinkType, ObjectClass, ObjectKey, ObjectProp, ActionGroup, OutputMode, RunsCount));
	m_AutoActionGroups[ActionGroup].push_back(m_lstActions.back().get());
	return true;
}

void CAutomatic::Init()
{
	CDeviceContainer *pAutomaticContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_AUTOMATIC);
	if(!pAutomaticContainer)
		throw dfw2error("CAutomatic::Init AutomaticContainer not available");
	CCustomDevice *pCustomDevice = static_cast<CCustomDevice*>(pAutomaticContainer->GetDeviceByIndex(0));
	if (!pCustomDevice)
		throw dfw2error("CAutomatic::Init CustomDevice for automatic not available");

	bool bRes = true;
	STRINGLIST ActionList;

	for (auto&& it : m_lstLogics)
	{
		CAutomaticItem *pLogic= it.get();
		std::string strVarName = fmt::format("LT{}", pLogic->GetId());

		// находим в автоматике выходное реле элемента логики по имени выхода
		CRelayDelay *pActionRelay = static_cast<CRelayDelay*>(pCustomDevice->GetPrimitiveForNamedOutput(strVarName.c_str()));
		if (!pActionRelay)
			throw dfw2error(fmt::format(CDFW2Messages::m_cszLogicNotFoundInDLL, strVarName));

		pActionRelay->SetDiscontinuityId(pLogic->GetId());
		CAutomaticLogic *pLogicItem = static_cast<CAutomaticLogic*>(pLogic);
		const std::string strActions = pLogicItem->GetActions();
		stringutils::split(strActions, ";,", ActionList);

		for (auto&& sit : ActionList)
		{
			std::string strAction = sit;
			stringutils::trim(strAction);
			if (!strAction.empty())
			{
				ptrdiff_t nId(0);
#ifdef _MSC_VER
				if (sscanf_s(strAction.c_str(), "A%td", &nId) == 1)
#else
				if (sscanf(strAction.c_str(), "A%td", &nId) == 1)
#endif
				{
					auto mit = m_AutoActionGroups.find(nId);
					if (mit != m_AutoActionGroups.end())
					{
						if (!pLogicItem->AddActionGroupId(nId))
						{
							m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszDuplicateActionGroupInLogic,
																							nId, 
																							strActions, 
																							pLogicItem->GetId()));
						}
					}
					else
					{
						m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszNoActionGroupFoundInLogic,
																					nId, 
																					strActions, 
																					pLogicItem->GetId()));
						bRes = false;
					}
				}
				else
				{
					m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongActionInLogicList,
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
		bRes = (m_pAction->Do(pDynaModel, *m_pValue) != eDFW2_ACTION_STATE::AS_ERROR);
		m_nRunsCount--;
	}

	return bRes;
}

bool CAutomaticAction::Init(CDynaModel* pDynaModel, CCustomDevice *pCustomDevice)
{
	bool bRes = false;
	_ASSERTE(!m_pAction);
	_ASSERTE(!m_pValue);
	std::string strVarName = fmt::format(cszActionTemplate, m_nId);
	m_pValue = pCustomDevice->GetVariableConstPtr(strVarName.c_str());
	if (m_pValue)
	{
		switch (m_nType)
		{
			case 1: // объект
			{
				CDevice* pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, m_strObjectProp);
				if (pDev)
				{
					if (m_strObjectProp == "r")
					{
						m_pAction = std::make_unique<CModelActionChangeBranchR>(static_cast<CDynaBranch*>(pDev), *m_pValue);
						bRes = true;
					}
					else if (m_strObjectProp == "x")
					{
						m_pAction = std::make_unique<CModelActionChangeBranchX>(static_cast<CDynaBranch*>(pDev), *m_pValue);
						bRes = true;
					}
				}
				break;
			}
			case 3:	// состояние ветви
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasBranch;
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, m_strObjectProp);
				if (pDev)
				{
					m_pAction = std::make_unique<CModelActionChangeBranchState>(static_cast<CDynaBranch*>(pDev), CDynaBranch::BranchState::BRANCH_OFF);
					bRes = true;
				}
				break;
			}
			case 4:	// g-шунт узла
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
			case 5: // b-шунт узла
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
			case 6:	// r-шунт узла
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
			case 7:	// x-шунт узла
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
			default:
				pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszActionTypeNotFound, GetVerbalName(), m_nType));
				break;
		}
	}
	else
		pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszActionNotFoundInDLL, strVarName));

	return bRes;
}


const char* CAutomaticAction::cszActionTemplate = "A{}";