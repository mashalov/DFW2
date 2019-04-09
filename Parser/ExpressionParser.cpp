#include "stdafx.h"
#include "ExpressionParser.h"
#include "ExpressionDictionary.h"

CExpressionParserRules::CExpressionParserRules() : CExpressionParser()
{
	m_pRules = &DefaultRules;
}

CExpressionParserRules::CExpressionParserRules(CParserVariables& CommonVariables) : CExpressionParser(CommonVariables)
{
	m_pRules = &DefaultRules;
}

CExpressionParser::CExpressionParser() : m_szExpression(NULL),
										 m_szTokenText(NULL),
										 m_pComplexToken(NULL),
										 m_Variables(m_InternalVariables),
										 m_bUsingInternalVars(true),
										 m_pDictionary(&DefaultDictionary)
{
	
}

CExpressionParser::CExpressionParser(CParserVariables& CommonVariables) : m_szExpression(NULL),
																   m_szTokenText(NULL),
																   m_pComplexToken(NULL),
																   m_Variables(CommonVariables),
																   m_bUsingInternalVars(false),
																   m_pDictionary(&DefaultDictionary)
{

}

CExpressionParser::CExpressionParser(const CExpressionParser* pParser) : m_szExpression(NULL),
																		 m_szTokenText(NULL),
																		 m_pComplexToken(NULL),
																		 m_pDictionary(pParser->m_pDictionary),
																		 m_Variables(m_InternalVariables),
																		 m_bUsingInternalVars(true),
																		 m_pRules(NULL)
{

}

CExpressionParser::~CExpressionParser()
{
	CleanUp();
}

void CExpressionParser::CleanUp()
{
	m_nHeadPosition = 0;
	m_nAdvanceNextSymbol = 0;
	if (m_szExpression)
	{
		delete m_szExpression;
		m_szExpression = NULL;
	}
	if (m_szTokenText)
	{
		delete m_szTokenText;
		m_szTokenText = NULL;
	}

	m_ResultStack = PARSERSTACK();
	m_ParserStack = PARSERSTACK();
	m_ArityStack = stack<int>();

	for (TOKENSET::iterator it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
		delete *it;

	if (m_pComplexToken)
	{
		delete m_pComplexToken;
		m_pComplexToken = NULL;
	}

	m_TokenPool.clear();

	if (m_bUsingInternalVars)
		m_Variables.Clear();
}

CExpressionToken* CExpressionParser::GetToken()
{
	ResetToken();
	
	if (!ProcessOperator())
	{
		while (m_bContinueToken)
		{
			if (m_eNextSymbolType == EST_ERROR)
			{
				Advance();
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGSYMBOL);
				break;
			}

			if (m_pCurrentToken && m_pCurrentToken->IsError())
				break;

			switch (m_eAwaitTokenType)
			{
			case ETT_UNDEFINED:
				ProcessUndefined();
				break;
			case ETT_NUMERIC_CONSTANT:
				ProcessNumericConstant();
				break;
			case ETT_VARIABLE:
				ProcessVariable();
				break;
			case ETT_MODELLINK:
				ProcessModelLink();
				break;
			}

			CurrentSymbolType();
		}
	}

	if (m_pCurrentToken->IsOperator() && !m_pCurrentToken->IsUnary() && m_pCurrentToken->GetType() != ETT_RB)
		m_bUnaryMinus = true;
	else
		m_bUnaryMinus = false;

	return m_pCurrentToken;
}

bool CExpressionParser::Parse(const _TCHAR* cszExpression)
{
	bool bRes = false;

	eExpressionTokenType eError = ETT_UNDEFINED;
	m_nExpressionLength = _tcslen(cszExpression);
	m_szExpression = new _TCHAR[m_nExpressionLength + 1];
	_tcscpy_s(m_szExpression, m_nExpressionLength + 1, cszExpression);

	m_bUnaryMinus = true;

	CExpressionToken *pToken = GetToken();

	while (pToken)
	{
		if (pToken->IsEOF())	break;
		if (pToken->IsError())	break;

		if (pToken->IsLeaf()) 
		{
			m_ResultStack.push(pToken);
		} 
		else if (pToken->IsFunction())
		{
			m_ParserStack.push(pToken);
			m_ArityStack.push(1);
		}
		else if (pToken->IsOperator())
		{
			if (pToken->GetType() == ETT_COMMA)
			{
				bool bMatchParent = false;
				bool bAtLeastOneOperatorinParenthesis = false;

				while (!m_ParserStack.empty() && !bMatchParent)
				{
					if (m_ParserStack.top()->GetType() == ETT_LB)
						bMatchParent = true;
					else
					{
						eError = BuildOperatorTree();
						bAtLeastOneOperatorinParenthesis = true;

						if (eError != ETT_UNDEFINED)
							break;
					}
				}

				if (eError == ETT_UNDEFINED)
				{
					if (!bMatchParent)
						pToken->SetError(ETT_ERROR_NOLEFTPARENTHESIS);
					else
						if (!m_ArityStack.empty())
							m_ArityStack.top()++;
						else
							pToken->SetError(ETT_ERROR_STACKERROR);
				}
				else
					pToken->SetError(eError);
			}
			else if (pToken->GetType() == ETT_RB)
			{
				bool bMatchParent = false;

				while (!m_ParserStack.empty() && !bMatchParent)
				{
					if (m_ParserStack.top()->GetType() == ETT_LB)
					{
						bMatchParent = true;
						m_ParserStack.pop();
					}
					else
					{
						eError = BuildOperatorTree();
						if (eError != ETT_UNDEFINED)
							break;
					}
				}

				if (eError == ETT_UNDEFINED)
				{
					if (!bMatchParent)
						pToken->SetError(ETT_ERROR_NOLEFTPARENTHESIS);

					if (!m_ParserStack.empty() && eError == ETT_UNDEFINED)
					{
						if (m_ParserStack.top()->IsFunction())
						{
							eError = BuildOperatorTree();
							if (eError == ETT_UNDEFINED)
							{
								if (!m_ArityStack.empty())
									m_ArityStack.pop();
								else
									pToken->SetError(ETT_ERROR_STACKERROR);
							}
							else
							{
								pToken = m_ParserStack.top();
								pToken->SetError(eError);
							}
						}
					}
				}
				else
					pToken->SetError(eError);
			}
			else if (pToken->GetType() == ETT_LB)
			{
				m_ParserStack.push(pToken);
			}
			else
			{
				if (!pToken->IsError())
				{
					while (!m_ParserStack.empty())
					{
						CExpressionToken *pStackToken = m_ParserStack.top();

						if (pStackToken->RightAssociativity())
						{
							if (pStackToken->Precedence() > pToken->Precedence() && !pToken->IsUnary())
							{
								eError = BuildOperatorTree();
								if (eError != ETT_UNDEFINED)
								{
									pToken->SetError(eError);
									break;
								}
							}
							else
								break;
						}
						else
						{
							if (pStackToken->Precedence() >= pToken->Precedence() && !pToken->IsUnary())
							{
								eError = BuildOperatorTree();
								if (eError != ETT_UNDEFINED)
								{
									pToken->SetError(eError);
									break;
								}
							}
							else
								break;
						}
					}
					m_ParserStack.push(pToken);
				}
			}
		}

		if (pToken->IsError())
			break;
		else
			pToken = GetToken();
	}

	if (pToken && !pToken->IsError())
	{
		while (!m_ParserStack.empty())
		{
			if (m_ParserStack.top()->GetType() == ETT_LB)
			{
				pToken->SetError(ETT_ERROR_NORIGHTPARENTHESIS);
				break;
			}
			eError = BuildOperatorTree();
			if (eError != ETT_UNDEFINED)
			{
				pToken->SetError(eError);
				break;
			}
		}

		m_ParserStack = PARSERSTACK();

		if (!pToken->IsError())
			if (m_ResultStack.size() != 1)
				pToken->SetError(ETT_ERROR_STACKERROR);
	}

	if (pToken && !pToken->IsError())
	{
		CExpressionToken *pRoot = NewExpressionToken(ETT_ROOT);
		pRoot->AddChild(m_ResultStack.top());
		m_ResultStack.pop();
		m_ResultStack.push(pRoot);
		CleanTree();
		bRes = true;
	}

	return bRes;
}

void CExpressionParser::SkipSpaces()
{
	while (m_nHeadPosition < m_nExpressionLength)
	{
		if (!_istspace(*(m_szExpression + m_nHeadPosition))) 
			break;
		m_nHeadPosition++;
	}
}

void CExpressionParser::CurrentSymbolType()
{
	m_eNextSymbolType = EST_ERROR;
	m_nAdvanceNextSymbol = 1;

	_TCHAR *pCurrentChar = m_szExpression + m_nHeadPosition;
	if (m_nHeadPosition >= m_nExpressionLength) m_eNextSymbolType = EST_EOF;
	else if (_istspace(*pCurrentChar)) m_eNextSymbolType = EST_EOF;
	else if (_istdigit(*pCurrentChar)) m_eNextSymbolType = EST_NUMBER;
	else if (_istalpha(*pCurrentChar)) m_eNextSymbolType = EST_ALPHA;
	else if (*pCurrentChar == _T('.')) m_eNextSymbolType = EST_DOT;
	else if (_istoperator(pCurrentChar) != NULL) m_eNextSymbolType = EST_OPERATOR;
}

int CExpressionParser::_isdecimalexponent()
{
	_ASSERTE(m_nHeadPosition < m_nExpressionLength);

	_TCHAR pCurrentChar = *(m_szExpression + m_nHeadPosition);
	if (pCurrentChar == _T('E') || pCurrentChar == _T('e')) 
		return 1; 
	return 0;
}

const OperatorEnum* CExpressionParser::_istoperator(const _TCHAR* pChar)
{
	m_pCurrentOperator = NULL;

	size_t nMaxMatchLength = 0;

	for (OPERATORITR opit = m_pDictionary->OperatorsEnum.begin(); opit != m_pDictionary->OperatorsEnum.end(); opit++)
	{
		OperatorEnum *pOpEnum = *opit;
		size_t nOpLength = pOpEnum->m_strOperatorText.length();

		if (nOpLength <= m_nExpressionLength - m_nHeadPosition)
		{
			if (!_tcsncmp(pChar, pOpEnum->m_strOperatorText.c_str(), nOpLength))
			{
				if (pOpEnum->m_eOperatorType == ETT_MINUS)
					if (m_bUnaryMinus)
						pOpEnum = GetOperatorEnum(ETT_UMINUS);

				if (nOpLength > nMaxMatchLength)
				{
					m_pCurrentOperator = pOpEnum;
					m_nAdvanceNextSymbol = nOpLength;
					nMaxMatchLength = nOpLength;
				}
			}
		}
	}
	return m_pCurrentOperator;
}


bool CExpressionParser::ProcessOperator()
{
	bool bRes = false;
	CurrentSymbolType();
	switch (m_eNextSymbolType)
	{
	case EST_OPERATOR:
		Advance();
		m_pCurrentToken = NewExpressionToken(m_pCurrentOperator);
		bRes = true;
		break;
	}
	return bRes;
}

void CExpressionParser::ProcessUndefined()
{
	switch (m_eNextSymbolType)
	{
	case EST_NUMBER:
		m_bDecimalInteger = true;
		m_eAwaitTokenType = ETT_NUMERIC_CONSTANT;
		Advance();
		break;
	case EST_DOT:
		m_eAwaitTokenType = ETT_NUMERIC_CONSTANT;
		Advance();
		break;
	case EST_ALPHA:
		m_eAwaitTokenType = ETT_VARIABLE;
		Advance();
		break;
	case EST_OPERATOR:
		Advance();
		m_eAwaitTokenType = m_pCurrentOperator->m_eOperatorType;
		break;
	case EST_EOF:
		m_pCurrentToken = NewExpressionToken(ETT_EOF);
		break;
	default:
		m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGSYMBOL);
		break;
	}
}

void CExpressionParser::ProcessNumericConstant()
{
	switch (m_eNextSymbolType)
	{
	case EST_NUMBER:

		if (m_bDecimalExponent)
			m_bDecimalExponentValue = true;

		if (m_bDecimalSeparator)
			m_bDecimalFraction = true;
		else
			m_bDecimalInteger = true;

		Advance();

		break;
	case EST_DOT:

		if (m_bDecimalSeparator || m_bDecimalExponent)
			m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		else
		{
			m_bDecimalSeparator = true;
			Advance();
		}

		break;

	case EST_ALPHA:

		if (m_bDecimalExponent)
			m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		else
		{
			if (_isdecimalexponent())
			{
				m_bDecimalExponent = true;
				Advance();
			}
			else
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		}

		break;

	case EST_OPERATOR:

		if (m_bDecimalExponentValue || !m_bDecimalExponent)
		{
			if (m_bDecimalInteger || m_bDecimalFraction)
				m_pCurrentToken = NewExpressionToken(ETT_NUMERIC_CONSTANT);
			else
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		}
		else
			if (m_bDecimalExponentSign)
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
			else
				if (m_pCurrentOperator && 
						(m_pCurrentOperator->m_eOperatorType == ETT_PLUS  || 
						 m_pCurrentOperator->m_eOperatorType == ETT_MINUS ||
						 m_pCurrentOperator->m_eOperatorType == ETT_UMINUS ))
				{
					Advance();
					m_bDecimalExponentSign = true;
				}
				else
					m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);

		break;

	case EST_EOF:
		if (m_bDecimalExponentValue || !m_bDecimalExponent)
		{
			if (m_bDecimalInteger || m_bDecimalFraction)
				m_pCurrentToken = NewExpressionToken(ETT_NUMERIC_CONSTANT);
			else
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);

		}
		else
			m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		break;
	default:
		m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		break;
	}
}

void CExpressionParser::ProcessVariable()
{
	switch (m_eNextSymbolType)
	{
	case EST_ALPHA:
	case EST_NUMBER:
		Advance();
		break;
	case EST_OPERATOR:
		if (m_pCurrentOperator->m_eOperatorType == ETT_LB)
			m_pCurrentToken = NewExpressionTokenFunction(m_szExpression);
		else if (m_pCurrentOperator->m_eOperatorType == ETT_LBS)
		{
			m_pComplexToken = new CExpressionTokenModelLink(this, m_nTokenBegin, m_nTokenLength);
			m_pCurrentToken = m_pComplexToken;

			m_pCurrentToken->SetTextValue(GetText(m_nTokenBegin, m_nTokenLength));

			m_eAwaitTokenType = ETT_MODELLINK;
			Advance();
		}
		else 
			m_pCurrentToken = NewExpressionToken(ETT_VARIABLE);
		break;
	case EST_EOF:
		m_pCurrentToken = NewExpressionToken(ETT_VARIABLE);
		break;
	default:
		m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGSYMBOL);
	}
}


void CExpressionParser::ProcessModelLink()
{
	CExpressionTokenModelLink *pModelLink = static_cast<CExpressionTokenModelLink*>(m_pCurrentToken);
	switch (m_eNextSymbolType)
	{
	case EST_ALPHA:
		switch (pModelLink->nWait)
		{
			case CExpressionTokenModelLink::MLS_ALPHA_CONTINUE:
			case CExpressionTokenModelLink::MLS_NUMBER_CONTINUE:
				pModelLink->AdvanceChild(m_nAdvanceNextSymbol);
				Advance();
			break;
			case CExpressionTokenModelLink::MLS_ALPHA:
				pModelLink->nWait = CExpressionTokenModelLink:: MLS_ALPHA_CONTINUE;
				pModelLink->AddChild(NewChildExpressionToken(ETT_PROPERTY));
				m_bContinueToken = true;
				Advance();
				break;
			default:
				pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
		}
		break;
	case EST_NUMBER:
		switch (pModelLink->nWait)
		{
			case CExpressionTokenModelLink::MLS_ALPHA_CONTINUE:
			case CExpressionTokenModelLink::MLS_NUMBER_CONTINUE:
				pModelLink->AdvanceChild(m_nAdvanceNextSymbol);
				Advance();
			break;
			case CExpressionTokenModelLink::MLS_NUMBER:
				pModelLink->nWait = CExpressionTokenModelLink::MLS_NUMBER_CONTINUE;
				pModelLink->AddChild(NewChildExpressionToken(ETT_NUMERIC_CONSTANT));
				m_bContinueToken = true;
				Advance();
				break;
			default:
				pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
		}
		break;

	case EST_OPERATOR:
		if (pModelLink->nWait == CExpressionTokenModelLink::MLS_ALPHA_CONTINUE)
		{
			BuildModelLink(pModelLink);
		}
		else
		if (pModelLink->nWait == CExpressionTokenModelLink::MLS_NUMBER_CONTINUE)
		{
			if (m_pCurrentOperator->m_eOperatorType == ETT_RBS)
			{
				Advance();
				pModelLink->AdvanceChild(m_nAdvanceNextSymbol);
				pModelLink->nWait = CExpressionTokenModelLink::MLS_DOT;
			}
			else if (m_pCurrentOperator->m_eOperatorType == ETT_COMMA)
			{
				Advance();
				pModelLink->AdvanceChild(m_nAdvanceNextSymbol);
				pModelLink->nWait = CExpressionTokenModelLink::MLS_NUMBER;
			}
			else
				pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
		}
		else
			pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);

		break;

	case EST_DOT:
		if (pModelLink->nWait == CExpressionTokenModelLink::MLS_DOT)
		{
			Advance();
			pModelLink->nWait = CExpressionTokenModelLink::MLS_ALPHA;
		}
		else
			pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);

		break;

	case EST_EOF:
		if (pModelLink->nWait == CExpressionTokenModelLink::MLS_ALPHA_CONTINUE)
			BuildModelLink(pModelLink);
		else
			pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
		break;

	default:
		pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
	}
}

void CExpressionParser::Advance()
{
	m_nTokenLength += m_nAdvanceNextSymbol;
	m_nHeadPosition += m_nAdvanceNextSymbol;
}

CExpressionToken* CExpressionParser::NewExpressionToken(CExpressionToken *pCloneToken)
{
	CExpressionToken *pNewToken = NULL;
	if (pCloneToken->IsVariable())
	{

		if (pCloneToken->GetParser()->IsRuleParser())
		{
			VARIABLEITR vit = m_SubstituteVars.find(pCloneToken->GetTextValue());
			if (vit != m_SubstituteVars.end())
			{
				pNewToken = CloneToken(vit->second.m_pToken);
			}
			else
				_ASSERTE(0);
		}
		else
		{
			VariableEnum *pEnum = m_Variables.Find(pCloneToken->GetTextValue());
			_ASSERTE(pEnum);
			pNewToken = pEnum->m_pToken;
		}
	}
	else
	{
		pNewToken = NewExpressionToken(pCloneToken->GetType());
		pNewToken->SetTextValue(pCloneToken->GetTextValue());
	}
	return pNewToken;
}

CExpressionToken* CExpressionParser::NewExpressionToken(eExpressionTokenType eType)
{
	CExpressionToken* pToken = NULL;
	if (eType == ETT_VARIABLE || eType == ETT_MODELLINK)
	{
		wstring VarName = GetText(m_nTokenBegin, m_nTokenLength);
		VariableEnum *pEnum = m_Variables.Find(VarName);
		if (pEnum)
			pToken = pEnum->m_pToken;
		else
		{
			pToken = new CExpressionToken(this, eType, m_nTokenBegin, m_nTokenLength);
			pToken->SetTextValue(VarName.c_str());
			m_TokenPool.insert(pToken);
			m_Variables.Add(VarName, pToken);
		}
	}

	if (!pToken)
	{
		OperatorEnum *pOpEnum = GetOperatorEnum(eType);
		if (pOpEnum)
			pToken = new CExpressionToken(this, pOpEnum, m_nTokenBegin, m_nTokenLength);
		else
		{
			FunctionEnum *pOpFunction = GetFunctionEnum(eType);
			if (pOpFunction)
				pToken = new CExpressionToken(this, pOpFunction, m_nTokenBegin, m_nTokenLength);
			else
				pToken = new CExpressionToken(this, eType, m_nTokenBegin, m_nTokenLength);
		}
		
		m_TokenPool.insert(pToken);
	}
	m_bContinueToken = false;
	return pToken;
}

CExpressionToken* CExpressionParser::NewChildExpressionToken(eExpressionTokenType eType)
{
	m_nTokenLength = 0;
	m_nTokenBegin = m_nHeadPosition;
	CExpressionToken* pToken = new CExpressionToken(this, eType, m_nHeadPosition, m_nTokenLength);
	m_TokenPool.insert(pToken);
	m_bContinueToken = false;
	return pToken;
}

CExpressionToken* CExpressionParser::NewExpressionTokenFunction(const _TCHAR *cszExpression)
{
	CExpressionToken* pToken = NULL;
	for (FUNCTIONITR fit = m_pDictionary->FunctionsEnum.begin(); fit != m_pDictionary->FunctionsEnum.end(); fit++)
	{
		FunctionEnum *pFunc = *fit;
		if (!_tcsncmp(pFunc->m_strOperatorText.c_str(), cszExpression + m_nTokenBegin, m_nTokenLength))
		{
			pToken = new CExpressionToken(this, pFunc, m_nTokenBegin, m_nTokenLength);
			break;
		}
	}

	if (!pToken)
		pToken = new CExpressionToken(this, ETT_ERROR_UNKNOWNFUNCTION, m_nTokenBegin, m_nTokenLength);

	m_TokenPool.insert(pToken);

	m_bContinueToken = false;
	return pToken;
}


CExpressionToken* CExpressionParser::NewExpressionToken(const OperatorEnum *pOperator)
{
	CExpressionToken* pToken = new CExpressionToken(this, pOperator, m_nTokenBegin, m_nTokenLength);
	m_bContinueToken = false;
	m_TokenPool.insert(pToken);
	return pToken;
}

void CExpressionParser::ResetToken()
{
	SkipSpaces();
	m_eAwaitTokenType = ETT_UNDEFINED;
	m_nTokenBegin = m_nHeadPosition;
	m_pCurrentOperator = NULL;
	m_nTokenLength = 0;
	m_bContinueToken = true;
	m_pCurrentToken = NULL;
	m_bDecimalSeparator = false;
	m_bDecimalExponent = false;
	m_bDecimalExponentSign = false;
	m_bDecimalExponentValue = false;
	m_bDecimalFraction = false;
	m_bDecimalInteger = false;
}

const _TCHAR* CExpressionParser::GetText(const CExpressionToken *pToken)
{
	return GetText(pToken->TextBegin(), pToken->TextLength());
}

const _TCHAR* CExpressionParser::GetText(size_t nTokenBegin, size_t nTokenLenght)
{
	if (m_szTokenText) delete m_szTokenText;
	m_szTokenText = new _TCHAR[nTokenLenght + 1];
	_tcsncpy_s(m_szTokenText, nTokenLenght + 1, m_szExpression + nTokenBegin, nTokenLenght);
	return m_szTokenText;
}

eExpressionTokenType CExpressionParser::BuildOperatorTree()
{
	eExpressionTokenType eError = ETT_UNDEFINED;
	if (m_ParserStack.empty())
		return ETT_ERROR_STACKERROR;

	CExpressionToken *pStackToken = m_ParserStack.top();
	int nArgsCount = pStackToken->ArgsCount();

	if (pStackToken->IsFunction())
	{
		if (!m_ArityStack.empty())
		{
			if (nArgsCount >= 0)
			{
				if (m_ArityStack.top() != nArgsCount)
					eError = ETT_ERROR_WRONGNUMBEROFARGS;
			}
			else
				nArgsCount = m_ArityStack.top();
		}
	}

	if (eError == ETT_UNDEFINED)
	{
		if (m_ResultStack.size() >= static_cast<size_t>(nArgsCount))
		{
			while (nArgsCount--)
			{
				pStackToken->AddChild(m_ResultStack.top());
				m_ResultStack.pop();
			}
		}
		else
			eError = ETT_ERROR_STACKERROR;
	}

	if (eError == ETT_UNDEFINED)
	{
		m_ParserStack.pop();
		m_ResultStack.push(pStackToken);
	}
	return eError;

}


const _TCHAR* CExpressionParser::GetErrorDescription(CExpressionToken *pToken)
{
	const _TCHAR* cszAtPosition = _T(" в позиции ");

	if (pToken)
	{
		if (pToken->IsError())
		{
			switch (pToken->GetType())
			{
			case ETT_ERROR_WRONGSYMBOL:
				m_szErrorDescription = Cex(_T("Неверный символ %s%s%d"), GetText(pToken), cszAtPosition, pToken->TextBegin());
				break;
			case ETT_ERROR_WRONGNUMBER:
				m_szErrorDescription = Cex(_T("Неверный формат числа %s%s%d"), GetText(pToken), cszAtPosition, pToken->TextBegin());
				break;
			case ETT_ERROR_UNKNOWNFUNCTION:
				m_szErrorDescription = Cex(_T("Неизвестная функция %s%s%d"), GetText(pToken), cszAtPosition, pToken->TextBegin());
				break;
			case ETT_ERROR_STACKERROR:
				m_szErrorDescription = Cex(_T("Ошибка стека%s%d"),cszAtPosition,pToken->TextBegin());
				break;
			case ETT_ERROR_WRONGNUMBEROFARGS:
				m_szErrorDescription = Cex(_T("Неверное число аргументов %s%s%d"),GetText(pToken),cszAtPosition,pToken->TextBegin());
				break;
			case ETT_ERROR_NOLEFTPARENTHESIS:
				m_szErrorDescription = Cex(_T("Нет левой скобки %s%d"), cszAtPosition,pToken->TextBegin());
				break;
			case ETT_ERROR_NORIGHTPARENTHESIS:
				m_szErrorDescription = Cex(_T("Нет правой скобки %s%d"),cszAtPosition,pToken->TextBegin());
				break;
			case ETT_ERROR_DIVISIONBYZERO:
				m_szErrorDescription = Cex(_T("Деление на ноль"));
				break;
			case ETT_ERROR_NEGATIVEROOT:
				m_szErrorDescription = Cex(_T("Дробная степень отрицательного числа"));
				break;
			case ETT_ERROR_NEGATIVELOG:
				m_szErrorDescription = Cex(_T("Логарифм из отрицательного числа"));
				break;
			case ETT_ERROR_WRONGARG:
				m_szErrorDescription = Cex(_T("Аргумент вне допустимого диапазона"));
				break;
			case ETT_ERROR_WRONGRANGE:
				m_szErrorDescription = Cex(_T("Неправильный диапазон"));
				break;
			}
		}
		else
			_tcprintf(_T("\nOK"));
	}
	return m_szErrorDescription.c_str();
}

bool CExpressionParser::Simplify()
{
	bool bRes = true;

	CleanTree();
	m_bChanged = true;
	wstring infix;

	if (bRes)
	{
		do
		{
			m_bChanged = false;
			PARSERSTACK Stack;
			TOKENSET Visited;

			if (m_ResultStack.empty())
			{
				bRes = false;
				break;
			}

			Stack.push(m_ResultStack.top());
			
			bool bPushChanged = false;

			while (!Stack.empty() && bRes)
			{
				m_bChanged = bPushChanged;

				GetExpressionConstants();
				CExpressionToken *pToken = Stack.top();

				CValidToken ValidToken(pToken);

				pToken->SortChildren();

				m_bChanged = false;

				for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
				{
					if (Visited.find(*it) == Visited.end())
						Stack.push(*it);
				}

				if (Stack.top() == pToken)
				{
					Visited.insert(pToken);

					if (m_pRules)
					{
						for (EXPRESSIONTRULEITR rit = m_pRules->begin(); rit != m_pRules->end() && bRes; rit++)
						{
							m_bChanged = false;

							if (!(*rit)->Apply(pToken))
							{
								bRes = false;
								break;
							}

							if (GetChanged() || ! ValidToken.IsValidToken())
							{
								bPushChanged = true;
								break;
							}
						}
					}

					if (GetChanged())
						break;

					if (ValidToken.IsValidToken())
					{
						m_bChanged = false;

						if (!pToken->Simplify())
						{
							bRes = false;
							break;
						}
						else
						{
							if (GetChanged())
								bPushChanged = true;

							if(ValidToken.IsValidToken())
								pToken->SortChildren();

							m_bChanged = false;

							Stack.pop();
						}
					}
					else
						Stack.pop();
				}
			}

			m_bChanged = bPushChanged;
			//Infix(infix);
			//_tcprintf(_T("\n%s"), infix.c_str());
		} 
		while (GetChanged() && bRes);
	}

	return bRes;
}


//#define ROOT

bool CExpressionParser::Expand()
{
#ifdef ROOT

	bool bRes = true;

	CleanTree();

	for (TOKENSETITR it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
	{
		CExpressionToken *pToken = *it;
		if (pToken->IsLeaf() && !pToken->GetTextValue())
			pToken->SetTextValue(GetText(pToken));
	}

	m_bChanged = true;
	wstring infix;

	if (bRes)
	{
		do
		{
			m_bChanged = false;
			PARSERSTACK Stack;
			Stack.push(m_ResultStack.top());
			while (!Stack.empty())
			{
				CExpressionToken *pToken = Stack.top();
				Stack.pop();
				pToken->SortChildren();
				if (!pToken->Expand(this))
				{
					bRes = false;
					break;
				}
				if (m_TokenPool.find(pToken) != m_TokenPool.end())
				{
					pToken->SortChildren();
					for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
						Stack.push(*it);
				}
			}
			Infix(infix);
			_tcprintf(_T("\n%s"), infix.c_str());
		} 
		while (GetChanged() && bRes);
	}

	return bRes;

#else

	bool bRes = true;

	CleanTree();
	m_bChanged = true;
	wstring infix;

	if (bRes)
	{
		do
		{
			m_bChanged = false;
			PARSERSTACK Stack;
			TOKENSET Visited;
			Stack.push(m_ResultStack.top());

			bool bPushChanged = false;

			while (!Stack.empty() && bRes)
			{
				m_bChanged = bPushChanged;
				GetExpressionConstants();

				CExpressionToken *pToken = Stack.top();

				CValidToken ValidToken(pToken);

				pToken->SortChildren();

				m_bChanged = false;

				for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
				{
					if (Visited.find(*it) == Visited.end())
						Stack.push(*it);
				}

				if (Stack.top() == pToken)
				{
					Visited.insert(pToken);
					
					m_bChanged = false;

					if (!pToken->Expand())
					{
						bRes = false;
						break;
					}
					else
					{
						if (GetChanged())
							bPushChanged = true;

						if (ValidToken.IsValidToken())
							pToken->SortChildren();

						m_bChanged = false;

						Stack.pop();
					}
				}
			}

			m_bChanged = bPushChanged;
//			Infix(infix);
//			_tcprintf(_T("\n%s"), infix.c_str());
		} 
		while (GetChanged() && bRes);
	}

	return bRes;

#endif

}


bool CExpressionParser::Infix(wstring& strInfix)
{
	bool bRes = true;
	if (m_ResultStack.size() != 1)
		return false;

	PARSERSTACK Stack;
	Stack.push(m_ResultStack.top());
	TOKENSET Visited;

	while (!Stack.empty())
	{
		CExpressionToken *pToken = Stack.top();
		for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
		{
			CExpressionToken *pChild = *it;
			if (Visited.find(pChild) == Visited.end())
			Stack.push(pChild);
		}

		if (Stack.top() == pToken)
		{
			Visited.insert(pToken);
			pToken->EvaluateText();
			Stack.pop();
		}
	}

	if (m_ResultStack.top()->GetType() == ETT_ROOT)
		strInfix = (*m_ResultStack.top()->ChildrenBegin())->GetTextValue();
	else
		strInfix = m_ResultStack.top()->GetTextValue();

	return bRes;
}

void CExpressionParser::ChangeTokenType(CExpressionToken *pToken, eExpressionTokenType eType)
{
	OperatorEnum *pOpEnum = GetOperatorEnum(eType);
	if (pOpEnum)
		pToken->SetOperator(pOpEnum);
	else
	{
		FunctionEnum *pFuncEnum = GetFunctionEnum(eType);
		if (pFuncEnum)
			pToken->SetOperator(pFuncEnum);
	}
}


FunctionEnum* CExpressionParser::GetFunctionEnum(eExpressionTokenType eType)
{
	for (FUNCTIONITR it = m_pDictionary->FunctionsEnum.begin(); it != m_pDictionary->FunctionsEnum.end(); it++)
		if ((*it)->m_eOperatorType == eType)
			return *it;

	return NULL;
}

OperatorEnum* CExpressionParser::GetOperatorEnum(eExpressionTokenType eType)
{
	for (OPERATORITR it = m_pDictionary->OperatorsEnum.begin(); it != m_pDictionary->OperatorsEnum.end(); it++)
		if ((*it)->m_eOperatorType == eType)
			return *it;
	return NULL;
}

void CExpressionParser::DeleteToken(CExpressionToken *pToken)
{
	if (pToken->ParentsCount() == 0)
	{
		bool bCanDelete = false;
		if (pToken->GetType() == ETT_VARIABLE || pToken->GetType() == ETT_MODELLINK)
		{
			VariableEnum *pEnum = m_Variables.Find(pToken->GetTextValue());
			bCanDelete = false;
		}
		else
			bCanDelete = true;

		if (bCanDelete)
		{
			TOKENSETITR it = m_TokenPool.find(pToken);
			if (it != m_TokenPool.end())
			{
				_ASSERTE(pToken->ChildrenCount() == 0);
				m_TokenPool.erase(it);
				delete pToken;
				SetChanged();
			}
		}
	}
}


CExpressionToken* CExpressionParser::CloneToken(CExpressionToken* pToken)
{
	CExpressionToken *pCloneToken = NewExpressionToken(pToken);
	CExpressionToken *pRetToken = pCloneToken;


	PARSERSTACK Stack, CloneStack;
	Stack.push(pToken);
	CloneStack.push(pCloneToken);

	while (!Stack.empty())
	{
		CExpressionToken *pStackToken = Stack.top();
		pCloneToken = CloneStack.top();
		Stack.pop();
		CloneStack.pop();

		for (TOKENITR it = pStackToken->ChildrenBegin(); it != pStackToken->ChildrenEnd(); it++)
		{
			CExpressionToken *pChildToken = *it;
			CExpressionToken *pNewCloneToken = NewExpressionToken(pChildToken);
			pCloneToken->AddChild(pNewCloneToken);
			Stack.push(pChildToken);
			CloneStack.push(pNewCloneToken);
		}
	}
	
	return pRetToken;
}

void CExpressionParser::SetChanged()
{
	m_bChanged = true;
}

bool CExpressionParser::GetChanged() const
{
	return m_bChanged;
}

bool CExpressionParser::AddRule(const _TCHAR* cszSource, const _TCHAR* cszDestination)
{
	CExpressionRule *pNewRule = new CExpressionRule(cszSource, cszDestination, this);
	//m_Rules.push_back(pNewRule);
	return true;
}

void CExpressionParser::CleanTree()
{
	TOKENSETITR it = m_TokenPool.begin();

	while (it != m_TokenPool.end())
	{
		bool bDeleted = false;
		switch ((*it)->GetType())
		{
		case ETT_LB:
		case ETT_RB:
		case ETT_EOF:
		case ETT_COMMA:
			delete *it;
			it = m_TokenPool.erase(it);
			bDeleted = true;
			break;
		}
		if (!bDeleted)
			it++;
	}
}

void CExpressionParser::RemoveRoot()
{
	CExpressionToken *pToken = m_ResultStack.top();
	if (pToken->GetType() == ETT_ROOT)
	{
		m_ResultStack.pop();
		m_ResultStack.push(pToken->GetChild());
	}
}

void CExpressionParser::GetExpressionConstants()
{
	for (TOKENSETITR it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
	{
		CExpressionToken *pToken = *it;
		if (pToken->IsLeaf() && !pToken->GetTextValue())
			pToken->SetTextValue(GetText(pToken));
	}
}

void CExpressionParser::CheckLinks() const
{
	for (TOKENSETITR it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
	{
		CExpressionToken *pToken = *it;
		switch (pToken->GetType())
		{
		case ETT_ROOT:
		case ETT_VARIABLE:
		case ETT_MODELLINK:
			break;
		default:
			_ASSERTE(pToken->ParentsCount() > 0);
			break;
		}
	}
}

const _TCHAR* CExpressionParser::GetErrorDescription()
{
	for (TOKENSETITR it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
	{
		CExpressionToken *pToken = *it;
		if (pToken->IsError())
			return GetErrorDescription(pToken); 
	}

	m_szErrorDescription = _T('\0');

	return m_szErrorDescription.c_str();
}

bool CExpressionParser::BuildModelLink(CExpressionTokenModelLink *pModelLink)
{
	bool bRes = true;

	_ASSERTE(pModelLink->nWait == CExpressionTokenModelLink::MLS_ALPHA_CONTINUE);

	pModelLink->AdvanceChild(m_nAdvanceNextSymbol);
	for (TOKENITR it = pModelLink->ChildrenBegin(); it != pModelLink->ChildrenEnd(); it++)
		(*it)->SetTextValue(GetText(*it));
	pModelLink->SetTextValue(GetText(pModelLink));
	pModelLink->EvaluateText();

	VariableEnum *pEnum = m_Variables.Find(pModelLink->GetTextValue());

	if (!pEnum)
	{
		m_Variables.Add(pModelLink->GetTextValue(), pModelLink);
		m_TokenPool.insert(pModelLink);
	}
	else
	{
		m_pCurrentToken = pEnum->m_pToken;
		delete pModelLink;
	}
	m_pComplexToken = NULL;
	m_bContinueToken = false;


	return bRes;
}

bool CExpressionParser::Process(const _TCHAR *cszExpression)
{
	bool bRes = Parse(cszExpression);
	bRes = bRes && Simplify();
	bRes = bRes && Expand();
	bRes = bRes && Simplify();

#ifdef _DEBUG
	wstring str;
	bRes = bRes && Infix(str);
#endif

	return bRes;
}

VariableEnum* CExpressionParser::CreateInternalVariable(const _TCHAR* cszVarName)
{
	VariableEnum *pInternalVar = m_Variables.Find(cszVarName);
	if (pInternalVar)
	{
		pInternalVar->m_eVarType = eCVT_INTERNAL;
		return pInternalVar;
	}

	CExpressionToken *pToken = new CExpressionToken(this, ETT_VARIABLE, 0, 0);
	pToken->SetTextValue(cszVarName);
	m_TokenPool.insert(pToken);
	m_Variables.Add(cszVarName, pToken);
	pInternalVar = m_Variables.Find(cszVarName);
	pInternalVar->m_eVarType = eCVT_INTERNAL;
	return pInternalVar;
}


CParserVariables::~CParserVariables()
{
	Clear();
}

void CParserVariables::Clear()
{
	m_Variables.clear();
}

bool CParserVariables::ChangeToEquationNames()
{
	set<CExpressionParser*> Parsers;

	for (VARIABLEITR it = m_Variables.begin(); it != m_Variables.end(); it++)
	{
		wstring compname;
		it->second.m_pToken->GetEquationOperand(it->second.m_pToken->m_pEquation, compname);
		it->second.m_pToken->SetTextValue(compname.c_str());
		Parsers.insert(it->second.m_pToken->m_pParser);
	}

	for (set<CExpressionParser*>::iterator pit = Parsers.begin(); pit != Parsers.end(); pit++)
	{
		CExpressionParser *pParser = *pit;
		wstring Infix;
		pParser->Infix(Infix);
	}


	return true;
}


VariableEnum *CParserVariables::Find(const _TCHAR* cszVarName)
{
	VariableEnum *pEnum = NULL;
	VARIABLEITR it = m_Variables.find(cszVarName);
	if (it != m_Variables.end())
		pEnum = &it->second;
	return pEnum;
}

bool CParserVariables::Rename(const _TCHAR *cszVarName, const _TCHAR *cszNewVarName)
{
	bool bRes = false;
	VariableEnum *pVarEnum = Find(cszVarName);
	if (pVarEnum)
	{
		VariableEnum Tmp = *pVarEnum;
		VariableEnum *pCheckEnum = Find(cszNewVarName);
		if (pCheckEnum)
		{
			if (m_Variables.erase(cszVarName))
			{
				CExpressionToken *pCurrentToken = pCheckEnum->m_pToken;
				pCheckEnum->m_eVarType = Tmp.m_eVarType;
				bRes = pCheckEnum->m_pToken->Join(Tmp.m_pToken);
				Tmp.m_pToken->Delete();
			}
		}
		else
		{
			pVarEnum->m_pToken->SetTextValue(cszNewVarName);
			if (m_Variables.erase(cszVarName))
				bRes = m_Variables.insert(make_pair(cszNewVarName, Tmp)).second;
		}
	}
	return bRes;
}

VariableEnum *CParserVariables::Find(const wstring& strVarName)
{
	return Find(strVarName.c_str());
}

VariableEnum *CParserVariables::Find(const CCompilerEquation *pEquation)
{
	for (VARIABLEITR it = m_Variables.begin(); it != m_Variables.end(); it++)
	{
		if (it->second.m_pToken->m_pEquation == pEquation)
			return &it->second;
	}
	return NULL;
}

void CParserVariables::Add(const _TCHAR* cszVarName, CExpressionToken* pToken)
{
	m_Variables.insert(make_pair(cszVarName, VariableEnum(pToken)));

	if (pToken->GetType() == ETT_MODELLINK)
		m_Variables.find(cszVarName)->second.m_eVarType = eCVT_EXTERNAL;

}

void CParserVariables::Add(const wstring& strVarName, CExpressionToken* pToken)
{
	Add(strVarName.c_str(), pToken);
}


const CParserDefaultDictionary CExpressionParser::DefaultDictionary;
const CExpressionRulesDefault CExpressionParserRules::DefaultRules;
