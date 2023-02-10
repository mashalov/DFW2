#include "stdafx.h"
#include "Automatic.h"
#include "DynaModel.h"

using namespace DFW2;

std::string CAutomaticItem::GetVerbalName()
{
	return m_strName.empty() ? fmt::format("{}", m_nId) : fmt::format("{} - \"{}\"", m_nId, m_strName);
}



CAutomaticItem::CAutomaticItem(ptrdiff_t Type, ptrdiff_t Id, std::string_view Name) :
		m_nType(Type),
		m_nId(Id),
		m_strName(Name)
{

}

void CAutomaticStarter::AddToSource(std::ostringstream& source)
{
	source << fmt::format("S{} = starter({}{})\n", m_nId,
		m_strExpression.empty() ? "V" : m_strExpression, 
		CAutoModelLink::empty() ? "" : fmt::format(",{}", CAutoModelLink::String()));
}

void CAutomaticLogic::AddToSource(std::ostringstream& source)
{
	source << fmt::format("L{} = {}\n", m_nId, m_strExpression);
	// выражение логики вводим через реле с уставкой 0 и выдержкой времени
	source << fmt::format("LT{} = relaydelay(L{}, 0.0, {})\n", m_nId, 
		m_nId,
		m_strDelayExpression.empty() ? "0" : m_strDelayExpression);
}

void CAutomaticAction::AddToSource(std::ostringstream& source)
{
	source << fmt::format("A{} = action({}, {})\n", m_nId,
		m_strExpression.empty() ? "V" : m_strExpression,
		CAutoModelLink::String());
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

const std::filesystem::path& CAutomatic::PathRoot() const
{
	return m_pDynaModel->Platform().Automatic();
}

const std::filesystem::path& CAutomatic::BuildPath() const
{
	return m_pDynaModel->Platform().AutomaticBuild();
}

const std::filesystem::path& CAutomatic::ModulesPath() const
{
	return m_pDynaModel->Platform().AutomaticModules();
}

const std::string_view CAutomatic::ModuleName() const
{
	return m_pDynaModel->Platform().AutomaticModuleName();
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
	std::array<const AUTOITEMS*, 3> automaticObjects { &m_lstStarters, &m_lstLogics, &m_lstActions };
	for (const auto& automaticObject : automaticObjects)
		for (const auto& automaticItem : *automaticObject)
			automaticItem->AddToSource(source);

	for(const auto& logic : m_lstLogics)
		m_mapLogics.insert(std::make_pair(logic->GetId(), logic.get()));

	for (const auto& action : m_lstActions)
		m_AutoActionGroups[static_cast<CAutomaticAction*>(action.get())->m_nActionGroup].push_back(action.get());

	std::ofstream src;
	std::filesystem::path autoFile(PathRoot());
	autoFile.append("auto.cmp");

	src.open(autoFile, std::fstream::out);
	if (src.is_open())
	{
		src << "main\n{\n" << source.str() << "}\n";
		src.close();
#ifdef _MSC_VER
		auto pCompiler{ std::make_shared<CCompilerDLL>("UMC.dll", "CompilerFactory") };
#else
		auto pCompiler{ std::make_shared<CCompilerDLL>("./Build/libUMC.so", "CompilerFactory") };
#endif
		
		std::stringstream Sourceutf8stream;
		CDLLInstanceWrapper<CCompilerDLL> Compiler(pCompiler);

		const auto vc{ Compiler->Version() };
		if (!CDynaModel::VersionsEquals(vc, m_pDynaModel->version))
			throw dfw2error(fmt::format(DFW2::CDFW2Messages::m_cszCompilerAndRaidenVersionMismatch, vc, m_pDynaModel->version));


		CDynaModel* pDynaModel{ m_pDynaModel };

		Compiler->SetMessageCallBacks(
				MessageCallBacks
				{
					[&pDynaModel](std::string_view message)
					{
						pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, message);
						return true;
					},
					[&pDynaModel](std::string_view message)
					{
						pDynaModel->Log(DFW2MessageStatus::DFW2LOG_WARNING, message);
						return true;
					},
					[&pDynaModel](std::string_view message)
					{
						pDynaModel->Log(DFW2MessageStatus::DFW2LOG_MESSAGE, message);
						return true;
					},
					[&pDynaModel](std::string_view message)
					{
						pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, message);
						return true;
					},
					[&pDynaModel](std::string_view message)
					{
						pDynaModel->Log(DFW2MessageStatus::DFW2LOG_DEBUG, message);
						return true;
					}
				}
		);

		Sourceutf8stream <<"main\n{\n" << source.str() << "}\n";

		Compiler->SetProperty("Platform", m_pDynaModel->Platform().Platform());
		Compiler->SetProperty("Configuration", m_pDynaModel->Platform().Configuration());

		Compiler->SetProperty("OutputPath", BuildPath().string());
		Compiler->SetProperty("DllLibraryPath", ModulesPath().string());
		Compiler->SetProperty("ProjectName", ModuleName());
		Compiler->SetProperty("ReferenceSources", m_pDynaModel->Platform().SourceReference().string());
		Compiler->SetProperty("DeviceType", DeviceType());
		Compiler->SetProperty("DeviceTypeNameSystem", "Auomatic");
		Compiler->SetProperty("DeviceTypeNameVerbal", "Automatic");
		Compiler->SetProperty("LinkDeviceType", "DEVTYPE_MODEL");
		Compiler->SetProperty("LinkDeviceMode", "DLM_SINGLE");
		Compiler->SetProperty("LinkDeviceDependency", "DPD_MASTER");

		if (!Compiler->Compile(Sourceutf8stream))
			throw dfw2error(DFW2::CDFW2Messages::m_cszWrongSourceData);

		pathAutomaticModule = Compiler->CompiledModulePath();
	}
}


#define COMSTR(a) std::string((a)).c_str()

bool CAutomatic::AddStarter(ptrdiff_t Type,
						    ptrdiff_t Id,
							std::string_view Name,
							std::string_view Expression,
							ptrdiff_t LinkType,
							std::string_view ObjectClass,
							std::string_view ObjectKey,
						    std::string_view ObjectProp)
{
	m_lstStarters.push_back(std::make_unique<CAutomaticStarter>(Type, Id, Name, Expression, ObjectClass, ObjectKey, ObjectProp));
	return true;
}

bool CAutomatic::AddLogic(ptrdiff_t Type,
						  ptrdiff_t Id,
						  std::string_view Name,
						  std::string_view Expression,
						  std::string_view Actions,
						  std::string_view DelayExpression,
						  ptrdiff_t OutputMode)
{
	m_lstLogics.push_back(std::make_unique<CAutomaticLogic>(Type, Id, Name, Expression, DelayExpression, Actions, OutputMode));
	return true;
}

bool CAutomatic::AddAction(ptrdiff_t Type,
						   ptrdiff_t Id,
						   std::string_view Name,
						   std::string_view Expression,
						   ptrdiff_t LinkType,
						   std::string_view ObjectClass,
						   std::string_view ObjectKey,
						   std::string_view ObjectProp,
						   ptrdiff_t ActionGroup,
						   ptrdiff_t OutputMode,
						   ptrdiff_t RunsCount)
{
	m_lstActions.push_back(std::make_unique<CAutomaticAction>(Type, Id, Name, Expression, LinkType, ObjectClass, ObjectKey, ObjectProp, ActionGroup, OutputMode, RunsCount));
	return true;
}

bool CAutomatic::ParseActionId(std::string& Action, ptrdiff_t& Id)
{
	bool Res{ false };
	stringutils::trim(Action);
	Id = 0;
	if (!Action.empty())
	{
#ifdef _MSC_VER
		if (sscanf_s(Action.c_str(), "A%td", &Id) == 1)
#else
		if (sscanf(Action.c_str(), "A%td", &Id) == 1)
#endif
			Res = true;
	}
	return Res;
}

void CAutomatic::Init()
{
	CDeviceContainer* pAutomaticContainer{ m_pDynaModel->GetDeviceContainer(Type()) };
	if(!pAutomaticContainer)
		throw dfw2error("CAutomatic::Init AutomaticContainer not available");
	CCustomDevice* pCustomDevice{ static_cast<CCustomDevice*>(pAutomaticContainer->GetDeviceByIndex(0)) };
	if (!pCustomDevice)
		throw dfw2error("CAutomatic::Init CustomDevice for automatic not available");

	bool bRes{ true };
	STRINGLIST ActionList;

	for (auto&& it : m_lstLogics)
	{
		CAutomaticItem *pLogic= it.get();
		std::string strVarName = fmt::format("LT{}", pLogic->GetId());

		// находим в автоматике выходное реле элемента логики по имени выхода
		CRelayDelay* pActionRelay{ static_cast<CRelayDelay*>(pCustomDevice->GetPrimitiveForNamedOutput(strVarName)) };
		if (!pActionRelay)
			throw dfw2error(fmt::format(CDFW2Messages::m_cszLogicNotFoundInDLL, strVarName));

		pActionRelay->SetDiscontinuityId(pLogic->GetId());
		CAutomaticLogic* pLogicItem{ static_cast<CAutomaticLogic*>(pLogic) };
		const std::string strActions{ pLogicItem->GetActions() };
		stringutils::split(strActions, ActionList);

		ptrdiff_t Id{ 0 };
		for (auto&& strAction : ActionList)
		{
			if (CAutomatic::ParseActionId(strAction, Id))
			{
				const auto mit{ m_AutoActionGroups.find(Id) };
				if (mit != m_AutoActionGroups.end())
				{
					if (!pLogicItem->AddActionGroupId(Id))
					{
						m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszDuplicateActionGroupInLogic,
																						Id, 
																						strActions, 
																						pLogicItem->GetId()));
					}
				}
				else
				{
					m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszNoActionGroupFoundInLogic,
																				Id, 
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
			
	for (auto&& it : m_lstActions)
	{
		CAutomaticAction *pAction = static_cast<CAutomaticAction*>(it.get());
		if (!pAction->Init(m_pDynaModel, pCustomDevice))
		{
			m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszActionNotInitialized,
				pAction->GetVerbalName(),
				pAction->String()));
			bRes = false;
		}
	}

	if (!bRes)
		throw dfw2error(fmt::format(CDFW2Messages::m_cszAutomaticOrScenarioFailedToInitialize));
}


bool CAutomatic::NotifyRelayDelay(const CRelayDelayLogic* pRelay)
{
	bool bRes{ false };
	_ASSERTE(pRelay);

	if (!pRelay)
		return bRes;

	if (pRelay->GetDiscontinuityId() <= 0)
		return true;

	if (pRelay == 0)
		return true;


	auto mit{ m_mapLogics.find(pRelay->GetDiscontinuityId()) };

	if (mit != m_mapLogics.end())
	{
		CAutomaticLogic* pLogic{ static_cast<CAutomaticLogic*>(mit->second) };
		for (auto git : pLogic->GetGroupIds())
		{
			auto autoGroupIt{ m_AutoActionGroups.find(git) };

			_ASSERTE(autoGroupIt != m_AutoActionGroups.end());

			if (autoGroupIt != m_AutoActionGroups.end())
			{
				auto& lstActions{ autoGroupIt->second };
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

				// получаем устройство по классу и ключу
				CDevice* pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, CAutoModelLink::String());
				if (pDev)
				{
					// если устройство найдено, проверяем его тип по наследованию
					// до известного автоматике типа
					if(pDev->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_NODE))
					{
						m_strObjectClass = CDeviceContainerProperties::m_cszAliasNode;
						if (pDev)
						{

						}
					} else if(pDev->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_BRANCH))
					{
						if (m_strObjectProp == CDevice::m_cszSta)
						{
							m_pAction = std::make_unique<CModelActionChangeBranchState>(static_cast<CDynaBranch*>(pDev), CDynaBranch::BranchState::BRANCH_OFF);
							bRes = true;
						}
						else if (m_strObjectProp == "r")
						{
							m_pAction = std::make_unique<CModelActionChangeBranchR>(static_cast<CDynaBranch*>(pDev), *m_pValue);
							bRes = true;
						}
						else if (m_strObjectProp == "x")
						{
							m_pAction = std::make_unique<CModelActionChangeBranchX>(static_cast<CDynaBranch*>(pDev), *m_pValue);
							bRes = true;
						}
						else if (m_strObjectProp == "b")
						{
							m_pAction = std::make_unique<CModelActionChangeBranchB>(static_cast<CDynaBranch*>(pDev), *m_pValue);
							bRes = true;
						}
					} else if(pDev->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_INFPOWER))
					{
						// для генератора отдельное состояние, но вообще можно состояния
						// всех устройств с обычными состояниями eDEVICESTATE обрабатывать одинаков
						if (m_strObjectProp == CDevice::m_cszSta)
						{
							m_pAction = std::make_unique<CModelActionChangeDeviceState>(pDev, eDEVICESTATE::DS_OFF);
							bRes = true;
						}
					}
				}
				break;
			}
			case 3:	// состояние ветви
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasBranch;
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, CAutoModelLink::String());
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
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, CAutoModelLink::String());
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
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, CAutoModelLink::String());
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
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, CAutoModelLink::String());
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
				CDevice *pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, CAutoModelLink::String());
				if (pDev)
				{
					m_pAction = std::make_unique<CModelActionChangeNodeShuntX>(static_cast<CDynaNode*>(pDev), *m_pValue);
					bRes = true;
				}
				break;
			}
			case 13: // PnQn0 - узел
			{
				m_strObjectClass = CDeviceContainerProperties::m_cszAliasNode;
				CDevice* pDev = pDynaModel->GetDeviceBySymbolicLink(m_strObjectClass, m_strObjectKey, CAutoModelLink::String());
				if (pDev)
				{
					m_pAction = std::make_unique<CModelActionChangeNodePQLoad>(static_cast<CDynaNode*>(pDev), *m_pValue);
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

void CAutomaticItem::UpdateSerializer(CSerializerBase* pSerializer)
{
	pSerializer->AddProperty(CDeviceId::m_cszid, m_nId);
	pSerializer->AddProperty("Type", m_nType);
	pSerializer->AddProperty(CDevice::m_cszName, m_strName);
}

class CSerializerAction : public CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>
{
	using CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::CSerializerDataSourceList;
public:
	void UpdateSerializer(CSerializerBase* pSerializer) override
	{
		std::unique_ptr<CAutomaticItem>& DataItem(CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::GetItem());
		if (!DataItem)
			DataItem = std::make_unique<CAutomaticAction>();

		CAutomaticAction* pAction(static_cast<CAutomaticAction*>(DataItem.get()));

		CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::UpdateSerializer(pSerializer);
		pSerializer->AddProperty("class", pAction->m_strObjectClass);
		pSerializer->AddProperty("expression", pAction->m_strExpression);
		pSerializer->AddProperty("key", pAction->m_strObjectKey);
		pSerializer->AddProperty("prop", pAction->m_strObjectProp);
		pSerializer->AddProperty("linkType", pAction->m_nLinkType);
		pSerializer->AddProperty("actionGroup", pAction->m_nActionGroup);
		pSerializer->AddProperty("outputMode", pAction->m_nOutputMode);
		pSerializer->AddProperty("runsCount", pAction->m_nRunsCount);
		pAction->UpdateSerializer(pSerializer);
	}
};

class CSerializerStarter : public CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>
{
	using CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::CSerializerDataSourceList;
public:
	void UpdateSerializer(CSerializerBase* pSerializer) override
	{
		std::unique_ptr<CAutomaticItem>& DataItem(CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::GetItem());
		if (!DataItem)
			DataItem = std::make_unique<CAutomaticStarter>();

		CAutomaticStarter* pStarter(static_cast<CAutomaticStarter*>(DataItem.get()));

		CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::UpdateSerializer(pSerializer);
		pSerializer->AddProperty("class", pStarter->m_strObjectClass);
		pSerializer->AddProperty("expression", pStarter->m_strExpression);
		pSerializer->AddProperty("key", pStarter->m_strObjectKey);
		pSerializer->AddProperty("prop", pStarter->m_strObjectProp);
		pStarter->UpdateSerializer(pSerializer);
	}
};

class CSerializerLogic : public CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>
{
	using CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::CSerializerDataSourceList;
public:
	void UpdateSerializer(CSerializerBase* pSerializer) override
	{
		std::unique_ptr<CAutomaticItem>& DataItem(CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::GetItem());
		if (!DataItem)
			DataItem = std::make_unique<CAutomaticLogic>();

		CAutomaticLogic* pLogic(static_cast<CAutomaticLogic*>(DataItem.get()));
		CSerializerDataSourceList<std::unique_ptr<CAutomaticItem>>::UpdateSerializer(pSerializer);
		pSerializer->AddProperty("outputMode", pLogic->m_nOutputMode);
		pSerializer->AddProperty("actions", pLogic->m_strActions);
		pSerializer->AddProperty("expression", pLogic->m_strExpression);
		pSerializer->AddProperty("delayExpression", pLogic->m_strDelayExpression);
		pLogic->UpdateSerializer(pSerializer);
	}
};

SerializerPtr CAutomatic::GetSerializer()
{
	SerializerPtr Serializer = std::make_unique<CSerializerBase>(new CSerializerDataSourceBase());
	Serializer->SetClassName("Automatic");
	Serializer->AddSerializer("Action", new CSerializerBase(new CSerializerAction(m_lstActions)));
	Serializer->AddSerializer("Logic", new CSerializerBase(new CSerializerLogic(m_lstLogics)));
	Serializer->AddSerializer("Starter", new CSerializerBase(new CSerializerStarter(m_lstStarters)));
	return Serializer;
}


const std::filesystem::path& CScenario::PathRoot() const
{
	return m_pDynaModel->Platform().Scenario();
}

const std::filesystem::path& CScenario::BuildPath() const
{
	return m_pDynaModel->Platform().ScenarioBuild();
}

const std::filesystem::path& CScenario::ModulesPath() const 
{
	return m_pDynaModel->Platform().ScenarioModules();
}

const std::string_view CScenario::ModuleName() const 
{
	return m_pDynaModel->Platform().ScenarioModuleName();
}

const char* CAutomaticAction::cszActionTemplate = "A{}";
