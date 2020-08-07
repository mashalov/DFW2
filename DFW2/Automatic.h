#pragma once
#include <filesystem>
#include "Discontinuities.h"
#include "Relay.h"
#include "DLLWrapper.h"


#ifdef _DEBUG
#ifdef _WIN64
#include "x64\debug\Compiler.tlh"
#else
#include "debug\Compiler.tlh"
#endif
#else
#ifdef _WIN64
#include "x64\release\Compiler.tlh"
#else
#include "release\Compiler.tlh"
#endif
#endif

#include "..\..\AntlrCPP\AntlrCPP\ICompiler.h"

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
				throw dfw2error(fmt::format(_T("Функция создания компилятора недоступна {}"), DLLFilePath));
		}

		ICompiler* CreateCompiler()
		{
			ICompiler* pCompiler(nullptr);
			if (!m_hDLL || !m_pfnFactory)
				throw dfw2error(_T("CreateCompiler - no dll loaded"));
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
		std::wstring m_strName;
	public:
		inline ptrdiff_t GetId() { return m_nId; }
		CAutomaticItem(long Type, ptrdiff_t Id, std::wstring_view Name);
		virtual ~CAutomaticItem() {}
	};

	class CAutomaticAction : public CAutomaticItem
	{
	protected:
		long m_nLinkType;
		std::wstring m_strObjectClass;
		std::wstring m_strObjectKey;
		std::wstring m_strObjectProp;
		ptrdiff_t m_nActionGroup;
		long m_nOutputMode;
		long m_nRunsCount;
		std::unique_ptr<CModelAction> m_pAction;
		const double *m_pValue;
	public:

		CAutomaticAction(long Type, 
						 ptrdiff_t Id,
					     std::wstring_view Name,
						 long LinkType,
			             std::wstring_view  ObjectClass,
			             std::wstring_view ObjectKey,
			             std::wstring_view ObjectProp,
						 long ActionGroup,
						 long OutputMode,
						 long RunsCount);

		bool Do(CDynaModel *pDynaModel);
		bool Init(CDynaModel* pDynaModel, CCustomDevice *pCustomDevice);
		virtual ~CAutomaticAction();

		static const _TCHAR* cszActionTemplate;
	};

	using AUTOITEMS = std::list<std::unique_ptr<CAutomaticItem>>;
	using AUTOITEMGROUP = std::map<ptrdiff_t, std::list<CAutomaticItem*> > ;
	using INTLIST = std::list<ptrdiff_t> ;
	using AUTOITEMSMAP = std::map<ptrdiff_t, CAutomaticItem*> ;

	class CAutomaticLogic : public CAutomaticItem
	{
	protected:
		long m_nOutputMode;
		std::wstring m_strActions;
		INTLIST m_ActionGroupIds;
	public:

		CAutomaticLogic(long Type,
			ptrdiff_t Id,
			std::wstring_view cszName,
			std::wstring_view cszActions,
			long OutputMode);

		const std::wstring& GetActions() { return m_strActions; }
		bool AddActionGroupId(ptrdiff_t nActionId);
		const INTLIST& GetGroupIds() const { return m_ActionGroupIds;  }
	};

	bool AddLogic(long Type,
		ptrdiff_t Id,
		std::wstring_view Name,
		std::wstring_view Expression,
		std::wstring_view Actions,
		std::wstring_view DelayExpression,
		long OutputMode);

	class CAutomatic
	{
	protected:
		IAutomaticCompilerPtr m_spAutomaticCompiler;
		CDynaModel *m_pDynaModel;
		AUTOITEMS m_lstActions;
		AUTOITEMS m_lstLogics;
		AUTOITEMSMAP m_mapLogics;
		AUTOITEMGROUP m_AutoActionGroups;
		std::wostringstream source;
		std::filesystem::path pathAutomaticDLL;
	public:
		bool PrepareCompiler();

		bool AddStarter(long Type, 
						long Id, 
						std::wstring_view cszName,
						std::wstring_view cszExpression,
						long LinkType, 
						std::wstring_view ObjectClass,
						std::wstring_view ObjectKey,
						std::wstring_view ObjectProp);

		bool AddLogic(long Type, 
					  long Id, 
					  std::wstring_view Name, 
					  std::wstring_view Expression,
					  std::wstring_view Actions,
					  std::wstring_view DelayExpression,
					  long OutputMode);

		bool AddAction(long Type, 
					   long Id, 
					   std::wstring_view Name,
					   std::wstring_view Expression,
					   long LinkType, 
					   std::wstring_view ObjectClass,
					   std::wstring_view ObjectKey,
					   std::wstring_view ObjectProp,
					   long ActionGroup, 
					   long OutputMode, 
					   long RunsCount);

		const std::filesystem::path& GetDLLPath() { return pathAutomaticDLL; }
		bool CompileModels();
		CAutomatic(CDynaModel *pDynaModel);
		virtual ~CAutomatic();
		void Init();
		void Clean();
		bool NotifyRelayDelay(const CRelayDelayLogic* pRelay);
	};
}

