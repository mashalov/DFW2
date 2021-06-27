#pragma once
#include <filesystem>
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
		ptrdiff_t m_nType;
		ptrdiff_t m_nId;
		std::string m_strName;
	public:
		inline ptrdiff_t GetId() { return m_nId; }
		CAutomaticItem(long Type, ptrdiff_t Id, std::string_view Name);
		virtual ~CAutomaticItem() = default;
		std::string GetVerbalName();
	};

	class CAutomaticAction : public CAutomaticItem
	{
	protected:
		long m_nLinkType;
		std::string m_strObjectClass;
		std::string m_strObjectKey;
		std::string m_strObjectProp;
		ptrdiff_t m_nActionGroup;
		long m_nOutputMode;
		long m_nRunsCount;
		std::unique_ptr<CModelAction> m_pAction;
		const double *m_pValue;
	public:

		CAutomaticAction(long Type, 
						 ptrdiff_t Id,
					     std::string_view Name,
						 long LinkType,
			             std::string_view  ObjectClass,
			             std::string_view ObjectKey,
			             std::string_view ObjectProp,
						 long ActionGroup,
						 long OutputMode,
						 long RunsCount);

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
		long m_nOutputMode;
		std::string m_strActions;
		INTLIST m_ActionGroupIds;
	public:

		CAutomaticLogic(long Type,
			ptrdiff_t Id,
			std::string_view cszName,
			std::string_view cszActions,
			long OutputMode);

		const std::string& GetActions() { return m_strActions; }
		bool AddActionGroupId(ptrdiff_t nActionId);
		const INTLIST& GetGroupIds() const { return m_ActionGroupIds;  }
	};

	bool AddLogic(long Type,
		ptrdiff_t Id,
		std::string_view Name,
		std::string_view Expression,
		std::string_view Actions,
		std::string_view DelayExpression,
		long OutputMode);

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

		bool AddLogic(long Type, 
					  long Id, 
					  std::string_view Name, 
					  std::string_view Expression,
					  std::string_view Actions,
					  std::string_view DelayExpression,
					  long OutputMode);

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
	};
}

