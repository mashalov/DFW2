#pragma once
#include "ExpressionToken.h"
#include "ExpressionDictionary.h"

class CExpressionParser
{

public:
	eExpressionSymbolType m_eNextSymbolType;
	eExpressionTokenType m_eAwaitTokenType;
	OperatorEnum *m_pCurrentOperator;
	bool m_bContinueToken;
	size_t m_nTokenBegin;
	size_t m_nTokenLength;
	CExpressionToken *m_pCurrentToken;

	bool m_bDecimalSeparator;
	bool m_bDecimalExponent;
	bool m_bDecimalExponentSign;
	bool m_bDecimalExponentValue;
	bool m_bDecimalFraction;
	bool m_bDecimalInteger;
	bool m_bUnaryMinus;

	bool m_bChanged;

	bool m_bUsingInternalVars;
	unique_ptr<_TCHAR[]> m_szExpression;			// временный буфер для выражения, предназначен для "разрушающих" операций типа удаления пробелов
	size_t m_nExpressionLength;
	size_t m_nHeadPosition;			// индекс текущего символа в выражении
	size_t m_nAdvanceNextSymbol;

	unique_ptr<_TCHAR[]> m_szTokenText;
	size_t m_nTokenTextLength;

	wstring m_szErrorDescription;

	PARSERSTACK m_ParserStack;		// временный стек парсера
	PARSERSTACK m_ResultStack;		// стек результата
	stack<ptrdiff_t> m_ArityStack;	// стек порядка аргументов для функций с переменным числом аргументов

	CExpressionToken *NewChildExpressionToken(eExpressionTokenType eType);
	CExpressionToken *NewExpressionToken(const OperatorEnum *pOperator);
	CExpressionToken *NewExpressionTokenFunction(const _TCHAR* cszExpression);
	CExpressionToken* NewExpressionToken(CExpressionToken *pCloneToken);


	CExpressionToken *GetToken();
	void CleanUp();
	const OperatorEnum* _istoperator(const _TCHAR* pChar);
	int _isdecimalexponent();
	void CurrentSymbolType();
	void SkipSpaces();

	bool ProcessOperator();
	void ProcessUndefined();
	void ProcessNumericConstant();
	void ProcessVariable();
	void ProcessModelLink();
	void Advance();
	void ResetToken();
	void SetChanged();

	eExpressionTokenType BuildOperatorTree();
		
	TOKENSET m_TokenPool;
	CParserVariables& m_Variables;
	CParserVariables m_InternalVariables;
	OperatorEnum* GetOperatorEnum(eExpressionTokenType eType);
	FunctionEnum* GetFunctionEnum(eExpressionTokenType eType);
	CExpressionToken *NewExpressionToken(eExpressionTokenType eType);
	void ChangeTokenType(CExpressionToken *pToken, eExpressionTokenType eType);
	CExpressionToken* CloneToken(CExpressionToken* pToken);
	const _TCHAR* GetText(const CExpressionToken *pToken);
	const _TCHAR* GetText(size_t nTokenBegin, size_t nTokenLenght);
	const _TCHAR* GetErrorDescription(CExpressionToken *pToken);
	void DeleteToken(CExpressionToken *pToken);
	void CleanTree();
	void RemoveRoot();
	void GetExpressionConstants();
	const EXPRESSIONRULES *m_pRules;
	VARIABLES m_SubstituteVars;
	const CParserDictionary *m_pDictionary;
	static const CParserDefaultDictionary DefaultDictionary;
	bool BuildModelLink(CExpressionTokenModelLink *pModelLink);


	CExpressionParser(const CExpressionParser *pParser);
	CExpressionParser();
	CExpressionParser(CParserVariables& CommonVariables);


	bool Parse(const _TCHAR *cszExpression);
	bool Simplify();
	bool Expand();
	void CheckLinks() const;
	bool Process(const _TCHAR *cszExpression);
	bool GetChanged() const;
	// формирует строку из разобранного AST в инфиксной записи
	bool Infix(wstring& strInfix);
	bool InsertEquations(CCompilerEquations& Eqs);
	const _TCHAR* GetErrorDescription();
	bool AddRule(const _TCHAR* cszSource, const _TCHAR* cszDestination);
	VariableEnum* CreateInternalVariable(const _TCHAR* cszVarName);
	virtual bool IsRuleParser() const { return false;  }
	virtual ~CExpressionParser();
};

class CExpressionParserRule : public CExpressionParser
{
public:
	CExpressionParserRule() : CExpressionParser() {}
	CExpressionParserRule(CParserVariables& CommonVariables) : CExpressionParser(CommonVariables) {}
	CExpressionParserRule(const CExpressionParser *pParser) : CExpressionParser(pParser) {}
	virtual ~CExpressionParserRule() {}
	virtual bool IsRuleParser() const { return true; }
};

class CExpressionParserRules : public CExpressionParser
{
protected:
	static const CExpressionRulesDefault DefaultRules;
public:
	CExpressionParserRules();
	CExpressionParserRules(CParserVariables& CommonVariables);
};


// класс позволяет определить существует ли токен
// в пуле токенов парсера
class CValidToken
{
protected:
	CExpressionToken *m_pCheckToken;
	const CExpressionParser *m_pParser;
public:
	CValidToken(CExpressionToken *pToken) : m_pCheckToken(pToken), m_pParser(pToken->GetParser()) {}
	bool IsValidToken() const
	{
		return m_pParser->m_TokenPool.find(m_pCheckToken) != m_pParser->m_TokenPool.end();
	}
};
