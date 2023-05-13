#pragma once
#include <filesystem>
#include <sstream>
#include "Discontinuities.h"
#include "Relay.h"
#include "DLLWrapper.h"

#include "../UMC/UMC/ICompiler.h"

namespace DFW2
{
	class CDynaModel;
	class CCustomDevice;

	using CCompilerDLL = CDLLInstanceFactory<ICompiler>;

	struct CAutoModelLink
	{
		std::string m_strObjectClass;
		std::string m_strObjectKey;
		std::string m_strObjectProp;
		CAutoModelLink() {}

		CAutoModelLink(std::string_view ObjectClass, std::string_view ObjectKey, std::string_view ObjectProp) :
			m_strObjectClass(ObjectClass),
			m_strObjectKey(ObjectKey),
			m_strObjectProp(ObjectProp) { }

		CAutoModelLink(const CAutoModelLink&& other) :
			m_strObjectClass(std::move(other.m_strObjectClass)),
			m_strObjectKey(std::move(other.m_strObjectKey)),
			m_strObjectProp(std::move(other.m_strObjectProp)) {}

		bool empty() const
		{
			return m_strObjectClass.empty() && m_strObjectKey.empty() && m_strObjectProp.empty();
		}

		std::string String() const
		{
			return fmt::format("{}[{}].{}",
				m_strObjectClass,
				m_strObjectKey,
				m_strObjectProp);
		}
	};
		
	class CAutomaticItem
	{
	protected:
		ptrdiff_t m_nType = 0;
		ptrdiff_t m_nId = 0;
		std::string m_strName;
	public:
		inline ptrdiff_t GetId() { return m_nId; }
		CAutomaticItem() {}

		CAutomaticItem(CAutomaticItem&& other) noexcept :
			m_nType(std::exchange(other.m_nType, 0)),
			m_nId(std::exchange(other.m_nId, 0)),
			m_strName(std::move(m_strName)) 
		{

		}

		virtual void AddToSource(std::ostringstream& source) {}

		CAutomaticItem(ptrdiff_t Type, ptrdiff_t Id, std::string_view Name);
		virtual ~CAutomaticItem() = default;
		std::string GetVerbalName();

		void UpdateSerializer(CSerializerBase* pSerializer);
	};

	class CAutomaticAction : public CAutomaticItem, public CAutoModelLink
	{
	protected:
		std::unique_ptr<CModelAction> m_pAction;
		const double* m_pValue = nullptr;
	public:

		ptrdiff_t m_nLinkType = 0;
		ptrdiff_t m_nActionGroup = 0;
		ptrdiff_t m_nOutputMode = 0;
		ptrdiff_t m_nRunsCount = 1;
	
		std::string m_strExpression;

		CAutomaticAction() {}

		CAutomaticAction(ptrdiff_t Type, 
			ptrdiff_t Id, 
			std::string_view Name,
			std::string_view Expression,
			ptrdiff_t LinkType,
			std::string_view ObjectClass,
			std::string_view ObjectKey,
			std::string_view ObjectProp,
			ptrdiff_t ActionGroup,
			ptrdiff_t OutputMode,
			ptrdiff_t RunsCount) :

			CAutomaticItem(Type, Id, Name),
			m_strExpression(Expression),
			m_nLinkType(LinkType),
			CAutoModelLink(ObjectClass, ObjectKey, ObjectProp),
			m_nActionGroup(ActionGroup),
			m_nOutputMode(OutputMode),
			m_nRunsCount(RunsCount) {}


		CAutomaticAction(CAutomaticAction&& other) noexcept :
			CAutomaticItem(std::move(other)),
			m_pAction(std::move(other.m_pAction)),
			m_strExpression(std::move(other.m_strExpression)),
			CAutoModelLink(std::move(other)),
			m_nLinkType(std::exchange(other.m_nLinkType, 0)),
			m_nActionGroup(std::exchange(other.m_nActionGroup, 0)),
			m_nOutputMode(std::exchange(other.m_nOutputMode, 0)),
			m_nRunsCount(std::exchange(other.m_nRunsCount, 0)),
			m_pValue(std::exchange(other.m_pValue, nullptr))
		{
			
		}

		bool Do(CDynaModel *pDynaModel);
		bool Init(CDynaModel* pDynaModel, CCustomDevice *pCustomDevice);
		virtual ~CAutomaticAction() = default;
		void AddToSource(std::ostringstream& source) override;
		static const char* cszActionTemplate;
	};

	using AUTOITEMS = std::list<std::unique_ptr<CAutomaticItem>>;
	using AUTOITEMGROUP = std::map<ptrdiff_t, std::list<CAutomaticItem*> > ;
	using INTLIST = std::list<ptrdiff_t> ;
	using AUTOITEMSMAP = std::map<ptrdiff_t, CAutomaticItem*> ;

	class CAutomaticLogic : public CAutomaticItem
	{
	protected:
		INTLIST m_ActionGroupIds;
		CRelayDelay* pRelay_ = nullptr;
	public:
		ptrdiff_t m_nOutputMode = 0;
		std::string m_strActions;
		std::string m_strExpression;
  	    std::string m_strDelayExpression;

		CAutomaticLogic() {}

		CAutomaticLogic(ptrdiff_t Type, ptrdiff_t Id, std::string_view Name,
			std::string_view Expression,
			std::string_view DelayExpression,
			std::string_view Actions,
			ptrdiff_t OutputMode) :

			CAutomaticItem(Type, Id, Name),
			m_nOutputMode(OutputMode),
			m_strActions(Actions),
			m_strExpression(Expression),
			m_strDelayExpression(DelayExpression) {}


		const std::string& GetActions() { return m_strActions; }
		bool AddActionGroupId(ptrdiff_t nActionId);
		const INTLIST& GetGroupIds() const { return m_ActionGroupIds;  }
		void AddToSource(std::ostringstream& source) override;
		void LinkRelay(CRelayDelay* pRelay) { pRelay_ = pRelay; }
	};


	class CAutomaticStarter : public CAutomaticItem, public CAutoModelLink
	{
	public:

		std::string m_strExpression;
		CAutomaticStarter() {}

		CAutomaticStarter(ptrdiff_t Type,
			ptrdiff_t Id,
			std::string_view Name,
			std::string_view Expression,
			std::string_view ObjectClass,
			std::string_view ObjectKey,
			std::string_view ObjectProp) :
			CAutomaticItem(Type, Id, Name),
			m_strExpression(Expression),
			CAutoModelLink(ObjectClass, ObjectKey, ObjectProp) {}

		void AddToSource(std::ostringstream& source) override;
	};

	class CAutomatic
	{
	protected:
		CDynaModel *pDynaModel_;
		AUTOITEMS m_lstActions;
		AUTOITEMS m_lstLogics;
		AUTOITEMS m_lstStarters;
		AUTOITEMSMAP m_mapLogics;
		AUTOITEMGROUP m_AutoActionGroups;
		std::ostringstream source;
		std::filesystem::path pathAutomaticModule;

	public:

		bool AddStarter(ptrdiff_t Type, 
						ptrdiff_t Id,
						std::string_view Name,
						std::string_view Expression,
						ptrdiff_t LinkType,
						std::string_view ObjectClass,
						std::string_view ObjectKey,
						std::string_view ObjectProp);

		bool AddLogic(ptrdiff_t Type, 
					  ptrdiff_t Id,
					  std::string_view Name, 
					  std::string_view Expression,
					  std::string_view Actions,
					  std::string_view DelayExpression,
					  ptrdiff_t OutputMode);

		bool AddAction(ptrdiff_t Type,
					   ptrdiff_t Id,
					   std::string_view Name,
					   std::string_view Expression,
					   ptrdiff_t LinkType,
					   std::string_view ObjectClass,
					   std::string_view ObjectKey,
					   std::string_view ObjectProp,
					   ptrdiff_t ActionGroup,
					   ptrdiff_t OutputMode,
					   ptrdiff_t RunsCount);

		const std::filesystem::path& GetModulePath() const { return pathAutomaticModule; }
		void CompileModels();
		CAutomatic(CDynaModel *pDynaModel);
		virtual ~CAutomatic();
		void Init();
		void Clean();
		bool NotifyRelayDelay(const CRelayDelayLogic* pRelay);
		SerializerPtr GetSerializer();
		static bool ParseActionId(std::string& Action, ptrdiff_t& Id);
		virtual const std::filesystem::path& PathRoot() const;
		virtual const std::filesystem::path& BuildPath() const;
		virtual const std::filesystem::path& ModulesPath() const;
		virtual const std::string_view ModuleName() const;
		virtual const std::string_view DeviceType() const { return "DEVTYPE_AUTOMATIC"; }
		virtual const std::string_view DeviceTypeSystemName() const { return "Automatic"; }
		virtual const std::string_view DeviceTypeVerbalName() const { return "Automatic"; }
		virtual const eDFW2DEVICETYPE Type() const { return eDFW2DEVICETYPE::DEVTYPE_AUTOMATIC; }
	};

	class CScenario : public CAutomatic
	{
	public:
		using CAutomatic::CAutomatic;

		const std::filesystem::path& PathRoot() const override;
		const std::filesystem::path& BuildPath() const override;
		const std::filesystem::path& ModulesPath() const override;
		const std::string_view ModuleName() const override;
		const std::string_view DeviceType() const override { return "DEVTYPE_SCENARIO"; } 
		const std::string_view DeviceTypeSystemName() const override { return "Scenario"; }
		const std::string_view DeviceTypeVerbalName() const override { return "Scenario"; }
		const eDFW2DEVICETYPE Type() const override { return eDFW2DEVICETYPE::DEVTYPE_SCENARIO; }
	};
}

