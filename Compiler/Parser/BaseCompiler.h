#pragma once
#include "AutoCompilerMessages.h"
#include "ExpressionParser.h"
#include "CompilerLogger.h"
#include "DLLOutput.h"

class CCompilerModelLink
{
protected:
	ptrdiff_t m_eType;
	wstring m_strObjClass;
	wstring m_strObjKey;
	wstring m_strObjProp;
public:
	CCompilerModelLink(ptrdiff_t eType, const _TCHAR *cszObjClass, const _TCHAR *cszObjKey, const _TCHAR *cszObjProp) :
		m_eType(eType),
		m_strObjClass(cszObjClass),
		m_strObjKey(cszObjKey),
		m_strObjProp(cszObjProp) 
	{
		stringutils::trim(m_strObjClass);
		stringutils::trim(m_strObjKey);
		stringutils::trim(m_strObjProp);
	}

	wstring ToString() const
	{
		return wstring(Cex(_T("%s[%s].%s"), m_strObjClass.c_str(), m_strObjKey.c_str(), m_strObjProp.c_str()));
	}
};

typedef list<CCompilerEquation*> EQUATIONSLIST;
typedef EQUATIONSLIST::iterator EQUATIONSITR;
typedef EQUATIONSLIST::const_iterator EQUATIONSCONSTITR;

typedef map<wstring, VariableEnum*> COMPILERVARS;
typedef COMPILERVARS::iterator COMPILERVARSITR;
typedef COMPILERVARS::const_iterator COMPILERVARSCONSTITR;

class CCompiler;

class CCompilerEquations
{
protected:
	EQUATIONSLIST m_Equations;
	COMPILERVARS m_VarExternal;
	COMPILERVARS m_VarSetpoint;
	COMPILERVARS m_VarConst;
	COMPILERVARS m_Output;
	TOKENLIST m_HostBlocks;
	CCompiler& m_Compiler;

	void FlushStack(EQUATIONSLIST& Stack, EQUATIONSLIST& NewList);
	bool IsTokenInPlace(EQUATIONSLIST& NewList, CExpressionToken *pToken);
	bool SortEquations();

public:
	CCompilerEquations(CCompiler& Compiler) : m_Compiler(Compiler) {}
	bool AddEquation(CExpressionToken *pToken);
	bool GenerateEquations();
	bool GenerateMatrix();
	bool AddOutputVariable(const _TCHAR* cszVarName, VariableEnum *pVarEnum);
	bool AddExternalVariable(const _TCHAR* cszVarName, VariableEnum *pVarEnum);
	bool AddSetPoint(const _TCHAR* cszVarName, VariableEnum *pVarEnum);
	bool AddConst(const _TCHAR* cszVarName, VariableEnum *pVarEnum);
	bool GetDerivative(CExpressionToken *pToken, CExpressionToken *pTokenBy, wstring& Diff, wstring& StrCol, bool& bExternalGuard) const;
	bool FindEquationInList(EQUATIONSLIST& NewList, const CCompilerEquation *pEquation);
	long GetBlockIndex(const CExpressionToken *pToken) const;
	virtual ~CCompilerEquations();

	const COMPILERVARS& GetExternalVars() const { return m_VarExternal; }
	const COMPILERVARS& GetSetpointVars() const { return m_VarSetpoint; }
	const COMPILERVARS& GetConstVars()    const { return m_VarConst; }
	const COMPILERVARS& GetOutputVars()	const   { return m_Output; }
	const TOKENLIST& GetBlocks() const			{ return m_HostBlocks; }
	EQUATIONSCONSTITR EquationsBegin() const    { return m_Equations.begin(); }
	EQUATIONSCONSTITR EquationsEnd() const      { return m_Equations.end(); }


};

class CCompiler
{
protected:
	CCompilerEquations m_Equations;
	CDLLOutput m_DLLOutput;
public:
	CParserVariables m_Variables;
	CCompiler(CCompilerLogger& Logger) : m_Equations(*this), m_Logger(Logger), m_DLLOutput(*this) {}
	CCompilerLogger& m_Logger;
	const CCompilerEquations& GetEquations() const { return m_Equations;  }
	bool AddOutputVariable(const _TCHAR *cszVarName, VariableEnum* pVarEnum);
	bool AddExternalVariable(const _TCHAR *cszVarName, VariableEnum* pVarEnum);
	bool AddSetPoint(const _TCHAR *cszVarName, VariableEnum* pVarEnum);
	bool AddConst(const _TCHAR *cszVarName, VariableEnum* pVarEnum);
};

class CCompilerItem
{
protected:
	CCompiler& m_Compiler;
public:
	CCompilerItem(CCompiler& Compiler) : m_Compiler(Compiler), m_pParser(NULL) {}
	virtual ~CCompilerItem();
	CExpressionParserRules *m_pParser;
	VariableEnum* CreateInternalVariable(const _TCHAR* cszVarName);
	VariableEnum* CreateOutputVariable(const _TCHAR* cszVarName);
	bool AddSpecialVariables();
};

class CBaseToExternalLink : public CVariableExtendedInfoBase
{
public:
	VariableEnum *m_pEnum;
	CBaseToExternalLink(VariableEnum *pEnum) : m_pEnum(pEnum) {}
};