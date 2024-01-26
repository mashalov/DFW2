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
	class CCustomDeviceCPP;

	using CCompilerDLL = CDLLInstanceFactory<ICompiler>;

	struct CAutoModelLink
	{
		std::string ObjectClass_;
		std::string ObjectKey_;
		std::string ObjectProp_;
		CAutoModelLink() {}

		CAutoModelLink(std::string_view ObjectClass, std::string_view ObjectKey, std::string_view ObjectProp) :
			ObjectClass_(ObjectClass),
			ObjectKey_(ObjectKey),
			ObjectProp_(ObjectProp) { }

		CAutoModelLink(CAutoModelLink&& other) :
			ObjectClass_(std::move(other.ObjectClass_)),
			ObjectKey_(std::move(other.ObjectKey_)),
			ObjectProp_(std::move(other.ObjectProp_)) {}

		bool empty() const
		{
			return ObjectClass_.empty() && ObjectKey_.empty() && ObjectProp_.empty();
		}

		std::string String() const
		{
			return fmt::format("{}[{}].{}",
				ObjectClass_,
				ObjectKey_,
				ObjectProp_);
		}
	};
		
	class CAutomaticItem
	{
	protected:
		ptrdiff_t Type_ = 0;
		ptrdiff_t Id_ = 0;
		std::string Name_;
	public:
		inline ptrdiff_t GetId() const { return Id_; }
		CAutomaticItem() {}

		CAutomaticItem(CAutomaticItem&& other) noexcept :
			Type_(std::exchange(other.Type_, 0)),
			Id_(std::exchange(other.Id_, 0)),
			Name_(std::move(other.Name_))
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
		ModelActionT Action_;
		const double* pValue_ = nullptr;
	public:

		ptrdiff_t LinkType_ = 0;
		ptrdiff_t ActionGroup_ = 0;
		ptrdiff_t OutputMode_ = 0;
		ptrdiff_t RunsCount_ = 1;
	
		std::string Expression_;

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
			Expression_(Expression),
			LinkType_(LinkType),
			CAutoModelLink(ObjectClass, ObjectKey, ObjectProp),
			ActionGroup_(ActionGroup),
			OutputMode_(OutputMode),
			RunsCount_(RunsCount) {}


		CAutomaticAction(CAutomaticAction&& other) noexcept :
			CAutomaticItem(std::move(other)),
			Action_(std::move(other.Action_)),
			Expression_(std::move(other.Expression_)),
			CAutoModelLink(std::move(other)),
			LinkType_(std::exchange(other.LinkType_, 0)),
			ActionGroup_(std::exchange(other.ActionGroup_, 0)),
			OutputMode_(std::exchange(other.OutputMode_, 0)),
			RunsCount_(std::exchange(other.RunsCount_, 0)),
			pValue_(std::exchange(other.pValue_, nullptr))
		{
			
		}

		bool Do(CDynaModel *pDynaModel);
		bool Init(CDynaModel* pDynaModel, CCustomDeviceCPP *pCustomDevice);
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
		INTLIST ActionGroupIds_;
		CRelayDelay* pRelay_ = nullptr;
	public:
		ptrdiff_t OutputMode_ = 0;
		std::string Actions_;
		std::string Expression_;
  	    std::string DelayExpression_;

		CAutomaticLogic() {}

		CAutomaticLogic(ptrdiff_t Type, ptrdiff_t Id, std::string_view Name,
			std::string_view Expression,
			std::string_view DelayExpression,
			std::string_view Actions,
			ptrdiff_t OutputMode) :

			CAutomaticItem(Type, Id, Name),
			OutputMode_(OutputMode),
			Actions_(Actions),
			Expression_(Expression),
			DelayExpression_(DelayExpression) {}


		const std::string& Actions() const { return Actions_; }
		bool AddActionGroupId(ptrdiff_t nActionId);
		const INTLIST& GroupIds() const { return ActionGroupIds_;  }
		void AddToSource(std::ostringstream& source) override;
		void LinkRelay(CRelayDelay* pRelay) { pRelay_ = pRelay; }
	};


	class CAutomaticStarter : public CAutomaticItem, public CAutoModelLink
	{
	public:

		std::string Expression_;
		CAutomaticStarter() {}

		CAutomaticStarter(ptrdiff_t Type,
			ptrdiff_t Id,
			std::string_view Name,
			std::string_view Expression,
			std::string_view ObjectClass,
			std::string_view ObjectKey,
			std::string_view ObjectProp) :
			CAutomaticItem(Type, Id, Name),
			Expression_(Expression),
			CAutoModelLink(ObjectClass, ObjectKey, ObjectProp) {}

		void AddToSource(std::ostringstream& source) override;
	};

	class CAutomatic
	{
	protected:
		CDynaModel *pDynaModel_;
		AUTOITEMS Actions_;
		AUTOITEMS Logics_;
		AUTOITEMS Starters_;
		AUTOITEMSMAP LogicsMap_;
		AUTOITEMGROUP AutoActionGroups_;
		std::ostringstream source;
		std::filesystem::path pathAutomaticModule_;

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

		const std::filesystem::path& ModulePath() const { return pathAutomaticModule_; }
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

