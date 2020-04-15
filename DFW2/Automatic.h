#pragma once
#include "Discontinuities.h"
#include "Relay.h"


#ifdef _DEBUG
#import "..\Compiler\debug\Compiler.tlb" no_namespace, named_guids
#else
#import "..\Compiler\release\Compiler.tlb" no_namespace, named_guids
#endif 

namespace DFW2
{
	class CDynaModel;
	class CCustomDevice;
		
	class CAutomaticItem
	{
	protected:
		ptrdiff_t m_nType;
		ptrdiff_t m_nId;
		std::wstring m_strName;
	public:
		inline ptrdiff_t GetId() { return m_nId; }
		CAutomaticItem(long Type, ptrdiff_t Id, const _TCHAR* cszName);
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
		CModelAction *m_pAction;
		const double *m_pValue;
	public:

		CAutomaticAction(long Type, 
						 ptrdiff_t Id,
						 const _TCHAR* cszName,
						 long LinkType,
			  			 const _TCHAR* cszObjectClass,
						 const _TCHAR* cszObjectKey,
						 const _TCHAR *cszObjectProp,
						 long ActionGroup,
						 long OutputMode,
						 long RunsCount);

		bool Do(CDynaModel *pDynaModel);
		bool Init(CDynaModel* pDynaModel, CCustomDevice *pCustomDevice);
		virtual ~CAutomaticAction();

		static const _TCHAR* cszActionTemplate;
	};

	typedef std::list<CAutomaticItem*> AUTOITEMS;
	typedef AUTOITEMS::iterator AUTOITEMSITR;

	typedef std::map<ptrdiff_t, AUTOITEMS> AUTOITEMGROUP;
	typedef AUTOITEMGROUP::iterator AUTOITEMGROUPITR;

	typedef std::list<ptrdiff_t> INTLIST;
	typedef INTLIST::iterator INTLISTITR;
	typedef INTLIST::const_iterator CONSTINTLISTITR;

	typedef std::map<ptrdiff_t, CAutomaticItem*> AUTOITEMSMAP;
	typedef AUTOITEMSMAP::iterator AUTOITEMSMAPITR;

	class CAutomaticLogic : public CAutomaticItem
	{
	protected:
		long m_nOutputMode;
		std::wstring m_strActions;
		INTLIST m_ActionGroupIds;
	public:

		CAutomaticLogic(long Type,
			ptrdiff_t Id,
			const _TCHAR* cszName,
			const _TCHAR* cszActions,
			long OutputMode);

		const std::wstring& GetActions() { return m_strActions; }
		bool AddActionGroupId(ptrdiff_t nActionId);
		const INTLIST& GetGroupIds() const { return m_ActionGroupIds;  }
	};

	bool AddLogic(long Type,
		ptrdiff_t Id,
		const _TCHAR* cszName,
		const _TCHAR* cszExpression,
		const _TCHAR* cszActions,
		const _TCHAR* cszDelayExpression,
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
	public:
		bool PrepareCompiler();

		bool AddStarter(long Type, 
						long Id, 
						const _TCHAR* cszName, 
						const _TCHAR* cszExpression, 
						long LinkType, 
						const _TCHAR* cszObjectClass, 
						const _TCHAR* cszObjectKey, 
						const _TCHAR* cszObjectProp);

		bool AddLogic(long Type, 
					  long Id, 
					  const _TCHAR* cszName, 
					  const _TCHAR* cszExpression,
					  const _TCHAR* cszActions, 
					  const _TCHAR* cszDelayExpression, 
					  long OutputMode);

		bool AddAction(long Type, 
					   long Id, 
					   const _TCHAR* cszName, 
					   const _TCHAR* cszExpression, 
					   long LinkType, 
					   const _TCHAR* cszObjectClass, 
					   const _TCHAR* cszObjectKey, 
					   const _TCHAR *cszObjectProp, 
					   long ActionGroup, 
					   long OutputMode, 
					   long RunsCount);

		bool CompileModels();
		CAutomatic(CDynaModel *pDynaModel);
		virtual ~CAutomatic();
		void Init();
		void Clean();
		bool NotifyRelayDelay(const CRelayDelayLogic* pRelay);
	};
}

