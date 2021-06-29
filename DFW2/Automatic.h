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

	/*
	class CCompilerDLL : public CDLLInstance
	{
	protected:
		fnCompilerFactory m_pfnFactory;
	public:
		void Init(std::wstring_view DLLFilePath) override
		{
			CDLLInstance::Init(DLLFilePath);
			m_pfnFactory = reinterpret_cast<fnCompilerFactory>(::GetProcAddress(m_hDLL, "CompilerFactory"));
			if (!m_pfnFactory)
				throw dfw2error(fmt::format("Функция создания компилятора недоступна {}", DLLFilePath));
		}

		ICompiler* CreateCompiler()
		{
			ICompiler* pCompiler(nullptr);
			if (!m_hDLL || !m_pfnFactory)
				throw dfw2error("CreateCompiler - no dll loaded");
			return m_pfnFactory();
		}
	};
	*/

	using CCompilerDLL = CDLLInstanceFactory<ICompiler>;
		
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

		CAutomaticItem(ptrdiff_t Type, ptrdiff_t Id, std::string_view Name);
		virtual ~CAutomaticItem() = default;
		std::string GetVerbalName();

		void UpdateSerializer(CSerializerBase* pSerializer);
	};

	class CAutomaticAction : public CAutomaticItem
	{
	protected:
		std::unique_ptr<CModelAction> m_pAction;
		const double* m_pValue = nullptr;
	public:

		ptrdiff_t m_nLinkType = 0;
		ptrdiff_t m_nActionGroup = 0;
		ptrdiff_t m_nOutputMode = 0;
		ptrdiff_t m_nRunsCount = 1;
		std::string m_strObjectClass;
		std::string m_strObjectKey;
		std::string m_strObjectProp;

		CAutomaticAction() {}

		CAutomaticAction(long Type, 
						 ptrdiff_t Id,
					     std::string_view Name,
						 ptrdiff_t LinkType,
			             std::string_view  ObjectClass,
			             std::string_view ObjectKey,
			             std::string_view ObjectProp,
						 ptrdiff_t ActionGroup,
						 ptrdiff_t OutputMode,
						 ptrdiff_t RunsCount);

		CAutomaticAction(CAutomaticAction&& other) noexcept :
			CAutomaticItem(std::move(other)),
			m_pAction(std::move(other.m_pAction)),
			m_strObjectClass(std::move(other.m_strObjectClass)),
			m_strObjectKey(std::move(other.m_strObjectKey)),
			m_strObjectProp(std::move(other.m_strObjectProp)),
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
	public:
		ptrdiff_t m_nOutputMode = 0;
		std::string m_strActions;

		CAutomaticLogic() {}

		CAutomaticLogic(ptrdiff_t Type,
			ptrdiff_t Id,
			std::string_view cszName,
			std::string_view cszActions,
			ptrdiff_t OutputMode);

		const std::string& GetActions() { return m_strActions; }
		bool AddActionGroupId(ptrdiff_t nActionId);
		const INTLIST& GetGroupIds() const { return m_ActionGroupIds;  }
	};

	class CAutomatic
	{
	protected:
		CDynaModel *m_pDynaModel;
		AUTOITEMS m_lstActions;
		AUTOITEMS m_lstLogics;
		AUTOITEMSMAP m_mapLogics;
		AUTOITEMGROUP m_AutoActionGroups;
		std::ostringstream source;
		std::filesystem::path pathAutomaticModule;

	public:

		bool AddStarter(long Type, 
						long Id, 
						std::string_view cszName,
						std::string_view cszExpression,
						long LinkType, 
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

		bool AddAction(long Type, 
					   long Id, 
					   std::string_view Name,
					   std::string_view Expression,
					   long LinkType, 
					   std::string_view ObjectClass,
					   std::string_view ObjectKey,
					   std::string_view ObjectProp,
					   long ActionGroup, 
					   long OutputMode, 
					   long RunsCount);

		const std::filesystem::path& GetModulePath() const { return pathAutomaticModule; }
		void CompileModels();
		CAutomatic(CDynaModel *pDynaModel);
		virtual ~CAutomatic();
		void Init();
		void Clean();
		bool NotifyRelayDelay(const CRelayDelayLogic* pRelay);
		SerializerPtr GetSerializer();
	};
}

