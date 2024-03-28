#include "stdafx.h"
#include "Automatic.h"
#include "DynaModel.h"

using namespace DFW2;

std::string CAutomaticItem::GetVerbalName()
{
	return Name_.empty() ? fmt::format("{}", Id_) : fmt::format("{} - \"{}\"", Id_, Name_);
}



CAutomaticItem::CAutomaticItem(ptrdiff_t Type, ptrdiff_t Id, std::string_view Name) :
		Type_(Type),
		Id_(Id),
		Name_(Name)
{

}

void CAutomaticStarter::AddToSource(std::ostringstream& source)
{
	source << fmt::format("S{} = starter({}{})\n", Id_,
		Expression_.empty() ? "V" : Expression_, 
		CAutoModelLink::empty() ? "" : fmt::format(",{}", CAutoModelLink::String()));
}

void CAutomaticLogic::AddToSource(std::ostringstream& source)
{
	source << fmt::format("L{} = {}\n", Id_, Expression_);
	// выражение логики вводим через реле с уставкой 0 и выдержкой времени
	source << fmt::format("LT{} = relaydelay(L{}, 0.0, {})\n", Id_, 
		Id_,
		DelayExpression_.empty() ? "0" : DelayExpression_);
}

void CAutomaticAction::AddToSource(std::ostringstream& source)
{
	source << fmt::format("A{} = action({}, {})\n", Id_,
		Expression_.empty() ? "V" : Expression_,
		CAutoModelLink::String());
}


bool CAutomaticLogic::AddActionGroupId(ptrdiff_t nActionId)
{
	bool bRes = find(ActionGroupIds_.begin(), ActionGroupIds_.end(), nActionId) == ActionGroupIds_.end();
	if (bRes)
		ActionGroupIds_.push_back(nActionId);
	return bRes;
}


CAutomatic::CAutomatic(CDynaModel* pDynaModel) : pDynaModel_(pDynaModel)
{
}

const std::filesystem::path& CAutomatic::PathRoot() const
{
	return pDynaModel_->Platform().Automatic();
}

const std::filesystem::path& CAutomatic::BuildPath() const
{
	return pDynaModel_->Platform().AutomaticBuild();
}

const std::filesystem::path& CAutomatic::ModulesPath() const
{
	return pDynaModel_->Platform().AutomaticModules();
}

const std::string_view CAutomatic::ModuleName() const
{
	return pDynaModel_->Platform().AutomaticModuleName();
}

void CAutomatic::Clean()
{
	Actions_.clear();
	Logics_.clear();
	Logics_.clear();
	AutoActionGroups_.clear();
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
	std::array<const AUTOITEMS*, 3> automaticObjects { &Starters_, &Logics_, &Actions_ };
	for (const auto& automaticObject : automaticObjects)
		for (const auto& automaticItem : *automaticObject)
			automaticItem->AddToSource(source);

	for(const auto& logic : Logics_)
		LogicsMap_.insert(std::make_pair(logic->GetId(), logic.get()));

	for (const auto& action : Actions_)
		AutoActionGroups_[static_cast<CAutomaticAction*>(action.get())->ActionGroup_].push_back(action.get());

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
		auto pCompiler{ std::make_shared<CCompilerDLL>("./umc/libUMC.so", "CompilerFactory") };
#endif
		
		std::stringstream Sourceutf8stream;
		CDLLInstanceWrapper<CCompilerDLL> Compiler(pCompiler);

		const auto vc{ Compiler->Version() };
		if (!CDynaModel::VersionsEquals(vc, pDynaModel_->version))
			throw dfw2error(fmt::format(DFW2::CDFW2Messages::m_cszCompilerAndRaidenVersionMismatch, vc, pDynaModel_->version));


		CDynaModel* pDynaModel{ pDynaModel_ };

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

		Compiler->SetProperty("Platform", pDynaModel_->Platform().Platform());
		Compiler->SetProperty("Configuration", pDynaModel_->Platform().Configuration());

		Compiler->SetProperty("OutputPath", BuildPath().string());
		Compiler->SetProperty("DllLibraryPath", ModulesPath().string());
		Compiler->SetProperty("ProjectName", ModuleName());
		Compiler->SetProperty("ReferenceSources", pDynaModel_->Platform().SourceReference().string());
		Compiler->SetProperty("DeviceType", DeviceType());
		Compiler->SetProperty("DeviceTypeNameSystem", "Auomatic");
		Compiler->SetProperty("DeviceTypeNameVerbal", "Automatic");
		Compiler->SetProperty("LinkDeviceType", "DEVTYPE_MODEL");
		Compiler->SetProperty("LinkDeviceMode", "DLM_SINGLE");
		Compiler->SetProperty("LinkDeviceDependency", "DPD_MASTER");

		if (!Compiler->Compile(Sourceutf8stream))
			throw dfw2error(DFW2::CDFW2Messages::m_cszWrongSourceData);

		pathAutomaticModule_ = Compiler->CompiledModulePath();
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
	Starters_.push_back(std::make_unique<CAutomaticStarter>(Type, Id, Name, Expression, ObjectClass, ObjectKey, ObjectProp));
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
	Logics_.push_back(std::make_unique<CAutomaticLogic>(Type, Id, Name, Expression, DelayExpression, Actions, OutputMode));
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
	Actions_.push_back(std::make_unique<CAutomaticAction>(Type, Id, Name, Expression, LinkType, ObjectClass, ObjectKey, ObjectProp, ActionGroup, OutputMode, RunsCount));
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
	CDeviceContainer* pAutomaticContainer{ pDynaModel_->GetDeviceContainer(Type()) };
	if(!pAutomaticContainer)
		throw dfw2error("CAutomatic::Init AutomaticContainer not available");
	auto *pCustomDevice{ static_cast<CCustomDeviceCPP*>(pAutomaticContainer->GetDeviceByIndex(0)) };
	if (!pCustomDevice)
		throw dfw2error("CAutomatic::Init CustomDevice for automatic not available");

	bool bRes{ true };
	STRINGLIST ActionList;

	for (auto&& it : Logics_)
	{
		CAutomaticItem* pLogic{ it.get() };
		const std::string strVarName{ fmt::format("LT{}", pLogic->GetId()) };

		// находим в автоматике выходное реле элемента логики по имени выхода
		CRelayDelay* pActionRelay{ static_cast<CRelayDelay*>(pCustomDevice->GetPrimitiveForNamedOutput(strVarName)) };
		if (!pActionRelay)
			throw dfw2error(fmt::format(CDFW2Messages::m_cszLogicNotFoundInDLL, strVarName));

		pActionRelay->SetDiscontinuityId(pLogic->GetId());
		CAutomaticLogic* pLogicItem{ static_cast<CAutomaticLogic*>(pLogic) };
		pLogicItem->LinkRelay(pActionRelay);
		const std::string strActions{ pLogicItem->Actions() };
		stringutils::split(strActions, ActionList);

		ptrdiff_t Id{ 0 };
		for (auto&& strAction : ActionList)
		{
			if (CAutomatic::ParseActionId(strAction, Id))
			{
				if (const auto mit{ AutoActionGroups_.find(Id) }; mit != AutoActionGroups_.end())
				{
					if (!pLogicItem->AddActionGroupId(Id))
					{
						pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszDuplicateActionGroupInLogic,
																						Id, 
																						strActions, 
																						pLogicItem->GetId()));
					}
				}
				else
				{
					pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszNoActionGroupFoundInLogic,
																				Id, 
																				strActions, 
																				pLogicItem->GetId()));
					bRes = false;
				}
			}
			else
			{
				pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongActionInLogicList,
																strAction, 
																strActions, 
																pLogic->GetId()));
				bRes = false;
			}
		}
	}
			
	for (auto&& it : Actions_)
	{
		CAutomaticAction* pAction{ static_cast<CAutomaticAction*>(it.get()) };
		if (!pAction->Init(pDynaModel_, pCustomDevice))
		{
			pDynaModel_->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszActionNotInitialized,
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


	auto mit{ LogicsMap_.find(pRelay->GetDiscontinuityId()) };

	if (mit != LogicsMap_.end())
	{
		CAutomaticLogic* pLogic{ static_cast<CAutomaticLogic*>(mit->second) };
		pDynaModel_->LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Сработало реле автоматики {}",
			pLogic->GetVerbalName()
			));
		for (auto git : pLogic->GroupIds())
		{
			auto autoGroupIt{ AutoActionGroups_.find(git) };

			_ASSERTE(autoGroupIt != AutoActionGroups_.end());

			if (autoGroupIt != AutoActionGroups_.end())
			{
				auto& lstActions{ autoGroupIt->second };
				for (auto & item : lstActions)
				{
					static_cast<CAutomaticAction*>(item)->Do(pDynaModel_);
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

	if (Action_ && pValue_ && RunsCount_)
	{
		pDynaModel->LogTime(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszRunningAction, 
			GetVerbalName()));

		bRes = (Action_->Do(pDynaModel, *pValue_) != eDFW2_ACTION_STATE::AS_ERROR);
		RunsCount_--;
	}

	return bRes;
}

bool CAutomaticAction::Init(CDynaModel* pDynaModel, CCustomDeviceCPP *pCustomDevice)
{
	_ASSERTE(!Action_);
	_ASSERTE(!pValue_);
	const std::string strVarName{ fmt::format(cszActionTemplate, Id_) };
	pValue_ = pCustomDevice->GetVariableConstPtr(strVarName);
	if (pValue_)
	{
		switch (Type_)
		{
			case 1: // объект
			{
				// здесь нужно определить что за устройство и что за действие по классу и полю. 
				// Нужно привести заданные параметры для объекта к известным Raiden действиям
				// получаем устройство по классу и ключу

				if (CDevice* pDev{ pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()) }; pDev)
				{
					// если устройство найдено, проверяем его тип по наследованию
					// до известного автоматике типа
					if(pDev->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_NODE))
					{
						ObjectClass_ = CDynaNodeBase::cszAliasNode_;
						if (ObjectProp_ == CDevice::cszSta_)
							Action_ = std::make_unique<CModelActionChangeDeviceState>(static_cast<CDynaNode*>(pDev), eDEVICESTATE::DS_OFF);
						else if (ObjectProp_ == CDynaNodeBase::cszPload0_)
							// искусственное поле для множителя СХН нагрузки активной нагрузки
							Action_ = std::make_unique<CModelActionChangeNodePload>(static_cast<CDynaNode*>(pDev), *pValue_);
						else if (ObjectProp_ == CDynaNodeBase::cszQload0_)
							// искусственное поле для множителя СХН нагрузки реактивной нагрузки
							Action_ = std::make_unique<CModelActionChangeNodeQload>(static_cast<CDynaNode*>(pDev), *pValue_);

					} else if(pDev->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_BRANCH))
					{
						if (ObjectProp_ == CDevice::cszSta_)
							Action_ = std::make_unique<CModelActionChangeBranchState>(static_cast<CDynaBranch*>(pDev), CDynaBranch::BranchState::BRANCH_OFF);
						else if (ObjectProp_ == CDynaNodeBase::cszr_)
							Action_ = std::make_unique<CModelActionChangeBranchR>(static_cast<CDynaBranch*>(pDev), *pValue_);
						else if (ObjectProp_ == CDynaNodeBase::cszx_)
							Action_ = std::make_unique<CModelActionChangeBranchX>(static_cast<CDynaBranch*>(pDev), *pValue_);
						else if (ObjectProp_ == CDynaNodeBase::cszb_)
							Action_ = std::make_unique<CModelActionChangeBranchB>(static_cast<CDynaBranch*>(pDev), *pValue_);
					} else if(pDev->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_INFPOWER))
					{
						// для генератора отдельное состояние, но вообще можно состояния
						// всех устройств с обычными состояниями eDEVICESTATE обрабатывать одинаково
						if (ObjectProp_ == CDevice::cszSta_)
							Action_ = std::make_unique<CModelActionChangeDeviceState>(pDev, eDEVICESTATE::DS_OFF);
					}
					else
						if(auto CheckAction{ std::make_unique<CModelActionChangeDeviceVariable>(pDev, CAutoModelLink::ObjectProp_) } ; CheckAction->Initialized())
							Action_ = std::move(CheckAction);
				}
				break;
			}
			case 3:	// состояние ветви
			{
				ObjectClass_ = CDynaBranch::cszAliasBranch_;
				if (CDevice* pDev = pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()); pDev)
					Action_ = std::make_unique<CModelActionChangeBranchState>(static_cast<CDynaBranch*>(pDev), CDynaBranch::BranchState::BRANCH_OFF);
				break;
			}
			case 4:	// g-шунт узла
			{
				ObjectClass_ = CDynaNodeBase::cszAliasNode_;
				if (CDevice* pDev{ pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()) }; pDev)
					Action_ = std::make_unique<CModelActionChangeNodeShuntG>(static_cast<CDynaNode*>(pDev), *pValue_);
				break;
			}
			case 5: // b-шунт узла
			{
				ObjectClass_ = CDynaNodeBase::cszAliasNode_;
				if (CDevice* pDev{ pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()) }; pDev)
					Action_ = std::make_unique<CModelActionChangeNodeShuntB>(static_cast<CDynaNode*>(pDev), *pValue_);
				break;
			}
			case 6:	// r-шунт узла
			{
				ObjectClass_ = CDynaNodeBase::cszAliasNode_;
				if (CDevice* pDev{ pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()) }; pDev)
					Action_ = std::make_unique<CModelActionChangeNodeShuntR>(static_cast<CDynaNode*>(pDev), *pValue_);
				break;
			}
			case 7:	// x-шунт узла
			{
				ObjectClass_ = CDynaNodeBase::cszAliasNode_;
				if (CDevice* pDev{ pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()) }; pDev)
					Action_ = std::make_unique<CModelActionChangeNodeShuntX>(static_cast<CDynaNode*>(pDev), *pValue_);
				break;
			}
			case 13: // PnQn0 - узел
			{
				ObjectClass_ = CDynaNodeBase::cszAliasNode_;
				if (CDevice* pDev{ pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()) }; pDev)
					Action_ = std::make_unique<CModelActionChangeNodePQload>(static_cast<CDynaNode*>(pDev), *pValue_);
				break;
			}
			case 19: // Шунт по остаточному напряжению - Uост
			{
				ObjectClass_ = CDynaNodeBase::cszAliasNode_;
				if (CDevice* pDev{ pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()) }; pDev)
					Action_ = std::make_unique<CModelActionChangeNodeShuntToUscUref>(static_cast<CDynaNode*>(pDev), *pValue_);
				break;
			}
			case 20: // Шунт по остаточному напряжению  - R/X
			{
				ObjectClass_ = CDynaNodeBase::cszAliasNode_;
				if (CDevice* pDev{ pDynaModel->GetDeviceBySymbolicLink(ObjectClass_, ObjectKey_, CAutoModelLink::String()) }; pDev)
					Action_ = std::make_unique<CModelActionChangeNodeShuntToUscRX>(static_cast<CDynaNode*>(pDev), *pValue_);
				break;
			}
			default:
				pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszActionTypeNotFound, GetVerbalName(), Type_));
				break;
		}
	}
	else
		pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszActionNotFoundInDLL, strVarName));

	return Action_ != nullptr;
}

void CAutomaticItem::UpdateSerializer(CSerializerBase* pSerializer)
{
	pSerializer->AddProperty(CDeviceId::cszid_, Id_);
	pSerializer->AddProperty("Type", Type_);
	pSerializer->AddProperty(CDevice::cszName_, Name_);
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
		pSerializer->AddProperty("class", pAction->ObjectClass_);
		pSerializer->AddProperty("expression", pAction->Expression_);
		pSerializer->AddProperty("key", pAction->ObjectKey_);
		pSerializer->AddProperty("prop", pAction->ObjectProp_);
		pSerializer->AddProperty("linkType", pAction->LinkType_);
		pSerializer->AddProperty("actionGroup", pAction->ActionGroup_);
		pSerializer->AddProperty("outputMode", pAction->OutputMode_);
		pSerializer->AddProperty("runsCount", pAction->RunsCount_);
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
		pSerializer->AddProperty("class", pStarter->ObjectClass_);
		pSerializer->AddProperty("expression", pStarter->Expression_);
		pSerializer->AddProperty("key", pStarter->ObjectKey_);
		pSerializer->AddProperty("prop", pStarter->ObjectProp_);
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
		pSerializer->AddProperty("outputMode", pLogic->OutputMode_);
		pSerializer->AddProperty("actions", pLogic->Actions_);
		pSerializer->AddProperty("expression", pLogic->Expression_);
		pSerializer->AddProperty("delayExpression", pLogic->DelayExpression_);
		pLogic->UpdateSerializer(pSerializer);
	}
};

SerializerPtr CAutomatic::GetSerializer()
{
	SerializerPtr Serializer = std::make_unique<CSerializerBase>(new CSerializerDataSourceBase());
	Serializer->SetClassName("Automatic");
	Serializer->AddSerializer("Action", new CSerializerBase(new CSerializerAction(Actions_)));
	Serializer->AddSerializer("Logic", new CSerializerBase(new CSerializerLogic(Logics_)));
	Serializer->AddSerializer("Starter", new CSerializerBase(new CSerializerStarter(Starters_)));
	return Serializer;
}


const std::filesystem::path& CScenario::PathRoot() const
{
	return pDynaModel_->Platform().Scenario();
}

const std::filesystem::path& CScenario::BuildPath() const
{
	return pDynaModel_->Platform().ScenarioBuild();
}

const std::filesystem::path& CScenario::ModulesPath() const 
{
	return pDynaModel_->Platform().ScenarioModules();
}

const std::string_view CScenario::ModuleName() const 
{
	return pDynaModel_->Platform().ScenarioModuleName();
}

const char* CAutomaticAction::cszActionTemplate = "A{}";
