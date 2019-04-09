#pragma once
#include "BaseCompiler.h"

class CAutoItem
{
protected:
	ptrdiff_t m_eType;
	ptrdiff_t m_nId;
	wstring m_strName;
	wstring m_strFormula;
public:
	CAutoItem(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR* cszName, const _TCHAR *cszFormula);
	ptrdiff_t GetId() const { return m_nId; }

	wstring GetVerbalName() const 
	{ 
		if (m_strName.empty())
			return wstring(Cex(_T("%d"), GetId(), m_strName.c_str())); 
		else
			return wstring(Cex(_T("%d [%s]"), GetId(), m_strName.c_str()));
	};

	const wstring& GetFormula() const
	{
		return m_strFormula;
	}

};

class CAutoCompilerItem : public CCompilerItem
{
public:
	bool InsertEquations(CCompilerEquations& Equations);
	CAutoCompilerItem(CCompiler& Compiler);
};

class CAutoStarterItem : public CAutoItem
{
protected:
	CCompilerModelLink m_ModelLink;
public:
	CAutoStarterItem(CAutoItem& AutoItem, CCompilerModelLink& ModelLink);
	const CCompilerModelLink& GetModelLink() const
	{
		return m_ModelLink;
	}
};

class CAutoLogicItem : public CAutoItem
{
protected:
	wstring m_strDelay;
	wstring m_strActions;
	int m_nOutputMode;
public:
	CAutoLogicItem(CAutoItem& AutoItem, const _TCHAR* cszActions, const _TCHAR* cszDelay, int nOutputMode);
};

class CAutoActionItem : public CAutoStarterItem
{
protected:
	int m_nActionGroup;
	int m_nOutputMode;
	int m_nRunsCount;
public:
	CAutoActionItem(CAutoItem& AutoItem, CCompilerModelLink& ModelLink, int nActionGroup, int nOutputMode, int nRunsCount);
};

class CAutoCompilerItemBaseV : public CAutoCompilerItem
{
public:
	CAutoCompilerItemBaseV(CCompiler& Compiler) : CAutoCompilerItem(Compiler) {}
	bool Process(CAutoStarterItem *pStarterItem);
	static const _TCHAR* cszV;
	static const _TCHAR* cszBase;
};


class CCompilerAutoStarterItem : public CAutoCompilerItemBaseV, public CAutoStarterItem
{
public:
	CCompilerAutoStarterItem(CAutoStarterItem& StarterItem, CCompiler& Compiler) : CAutoCompilerItemBaseV(Compiler),
																				   CAutoStarterItem(StarterItem) {}
	bool Process();
	bool InsertEquations(CCompilerEquations& Equations);
};

class CCompilerAutoLogicItem : public CAutoCompilerItem, public CAutoLogicItem
{
public:
	CCompilerAutoLogicItem(CAutoLogicItem& LogicItem, CCompiler& Compiler) : CAutoCompilerItem(Compiler),  
																			 CAutoLogicItem(LogicItem)
																			 {}
	bool Process();
	bool InsertEquations(CCompilerEquations& Equations);
};

class CCompilerAutoActionItem : public CAutoCompilerItemBaseV, public CAutoActionItem
{
public:
	CCompilerAutoActionItem(CAutoActionItem& ActionItem, CCompiler& Compiler) : CAutoCompilerItemBaseV(Compiler),
																				CAutoActionItem(ActionItem)
																				{}
	bool Process();
	bool InsertEquations(CCompilerEquations& Equations);
};

typedef map<ptrdiff_t, CCompilerAutoStarterItem> STARTERMAP;
typedef STARTERMAP::iterator STARTERMAPITR;

typedef map<ptrdiff_t, CCompilerAutoLogicItem> LOGICMAP;
typedef LOGICMAP::iterator LOGICMAPITR;

typedef map<ptrdiff_t, CCompilerAutoActionItem> ACTIONMAP;
typedef ACTIONMAP::iterator ACTIONMAPITR;

class CAutoCompiler : public CCompiler
{
	STARTERMAP m_Starters;
	LOGICMAP m_Logics;
	ACTIONMAP m_Actions;

	bool m_bStatus;

public:
	CAutoCompiler(CCompilerLogger& Logger) : CCompiler(Logger), m_bStatus(true) {}
	bool AddStarter(CAutoStarterItem& Starter);
	bool AddLogic(CAutoLogicItem& Logic);
	bool AddAction(CAutoActionItem& Action);
	bool Generate(const _TCHAR *cszPath);
};

