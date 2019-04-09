#include "..\stdafx.h"
#include "ExpressionToken.h"
#include "ExpressionParser.h"


CExpressionToken::CExpressionToken(CExpressionParser *pParser, eExpressionTokenType eType, size_t nTokenBegin, size_t nTokenLength) : 
									m_pParser(pParser),
									m_eType(eType), 
									m_nTokenBegin(nTokenBegin), 
									m_nTokenLength(nTokenLength),
									m_pFunctionInfo(NULL),
									m_pEquation(NULL)
{
}

CExpressionToken::CExpressionToken(CExpressionParser *pParser, const FunctionEnum *pFunction, size_t nTokenBegin, size_t nTokenLength) :
									m_pParser(pParser),
									m_pFunctionInfo(pFunction),
									m_eType(pFunction->m_eOperatorType), 
									m_nTokenBegin(nTokenBegin), 
									m_nTokenLength(nTokenLength),
									m_pEquation(NULL)
{
}

int CExpressionToken::Precedence() const
{
	if (m_pFunctionInfo)
		return static_cast<const OperatorEnum*>(m_pFunctionInfo)->m_nPrecedence;
	else
	{
		_ASSERTE(0);
		return -1;
	}
}

int CExpressionToken::ArgsCount() const
{
	if (m_pFunctionInfo)
		return m_pFunctionInfo->m_nArgsCount;
	else
	{
		_ASSERTE(0);
		return -1;
	}
}


bool CExpressionToken::RightAssociativity() const
{
	if (m_pFunctionInfo)
		return static_cast<const OperatorEnum*>(m_pFunctionInfo)->m_nRightAssoc == 1;
	else
	{
		_ASSERTE(0);
		return false;
	}
}


void CExpressionToken::AddChild(CExpressionToken* pChild)
{
	m_Children.push_back(pChild);
	pChild->AddParent(this);
}


void CExpressionToken::ReplaceChild(CExpressionToken* pOldChild, CExpressionToken* pNewChild, int nChildNumber)
{
	bool bReplaced = false;
	int nCount = 0;
	for (TOKENITR it = m_Children.begin(); it != m_Children.end(); it++, nCount++)
	{
		if (*it == pOldChild)
		{
			if (nChildNumber < 0 || nChildNumber == nCount)
			{
				*it = pNewChild;
				pOldChild->Unparent(this);
				bReplaced = true;
				// break; change all occurencies of pOldChild
			}
		}
	}

	if (!bReplaced)
		m_Children.push_back(pNewChild);

	pNewChild->AddParent(this);
}

void CExpressionToken::AddParent(CExpressionToken *pParent)
{
	m_Parents.insert(pParent);

	if (IsVariable())
	{
		VariableEnum *pEnum = m_pParser->m_Variables.Find(GetTextValue());
		pEnum->m_nRefCount++;
		_ASSERTE(m_Parents.size() <= pEnum->m_nRefCount);
	}
	else
		_ASSERTE(m_Parents.size() < 2);

	m_pParser->SetChanged();
}

void CExpressionToken::Eliminate()
{
	_ASSERTE(m_Parents.size() == 1);
	CExpressionToken *pParent = *m_Parents.begin();

	for (TOKENITR cit = m_Children.begin(); cit != m_Children.end(); cit++)
	{
		CExpressionToken *pChild = *cit;
		pChild->Unparent(this);
		pParent->ReplaceChild(this,pChild);
	}

	Unparent(pParent);
	m_Children.clear();
}

// функция добавляет к текстовому представлению дочернего токена заданное количество символов из 
// разбираемой строки
void CExpressionToken::AdvanceChild(size_t nAdvance)
{
	if (m_Children.size())
	{
		CExpressionToken *ptk = m_Children.back();
		m_Children.back()->m_nTokenLength += nAdvance;
	}
	else
		SetError(ETT_ERROR_WRONGNUMBEROFARGS);
}


void CExpressionToken::SetTextValue(const _TCHAR *cszValue)
{
	if (cszValue)
	{
		if (m_strStringValue != cszValue)
		{
			m_pParser->SetChanged();
			m_strStringValue = cszValue;
		}

		if (m_eType == ETT_NUMERIC_CONSTANT)
		{
			if (m_strStringValue.empty())
			{
				m_pParser->SetChanged();
				m_strStringValue = _T("0");
			}
		}
	}



#ifdef _DEBUG
	else
		_ASSERTE(!IsVariable() && !IsNumeric());
#endif
}

const _TCHAR* CExpressionToken::GetTextValue() const
{
	if (IsError())
		return m_pParser->GetErrorDescription();

	return m_strStringValue.empty() ? NULL : m_strStringValue.c_str();
}

void CExpressionToken::GetOperand(CExpressionToken *pToken, wstring& Result, bool bRight) const
{
	Result = pToken->GetTextValue();

	bool bNeedParenthesis = true;

	if (!pToken->IsLeaf())
	{
		if (pToken->Precedence() > Precedence())
			bNeedParenthesis = false;
		if (pToken->RightAssociativity() == RightAssociativity() && pToken->Precedence() == Precedence())
		{
			if (bRight && RightAssociativity())
				bNeedParenthesis = false;
			if (!bRight && !RightAssociativity())
				bNeedParenthesis = false;
		}
	}
	else
		bNeedParenthesis = false;

	if (bNeedParenthesis)
	{
		Result.insert(Result.begin(), _T('('));
		Result.push_back(_T(')'));
	}
}

// true если переменная внешняя eCVT_EXTERNAL

bool CExpressionToken::GetEquationOperandIndex(const CCompilerEquation *pEquation, wstring& Result) const
{
	bool bRes = false; 

	VariableEnum *pEnum;
	GetEquationOperandType(pEquation, Result, pEnum);
	if (pEnum)
	{
		switch (pEnum->m_eVarType)
		{
		case eCVT_INTERNAL:
			_ASSERTE(m_pEquation);
			Result = Cex(_T("%d"), m_pEquation->m_nIndex);
			break;
		case eCVT_EXTERNAL:
			Result += _T(".nIndex");
			bRes = true;
			break;

		}
	}

	return bRes;
}

void CExpressionToken::GetEquationOperand(const CCompilerEquation *pEquation, wstring& Result) const
{
	VariableEnum *pEnum;
	GetEquationOperandType(pEquation, Result, pEnum);
	if (pEnum)
	{
		if (pEnum->m_eVarType == eCVT_EXTERNAL)
		{
			Result += _T(".pValue");
			Result.insert(Result.begin(), _T('*'));
		}
	}
}

void CExpressionToken::GetEquationVariableType(const CCompilerEquation *pEquation, VariableEnum* pVarEnum, wstring& Result) const
{
	_ASSERTE(pVarEnum);

	switch (pVarEnum->m_eVarType)
	{
	case eCVT_INTERNAL:
		_ASSERTE(m_pEquation);
		Result = Cex(_T("%s%d]"), CCompilerEquation::m_cszInternalVar, m_pEquation->m_nIndex);
		break;
	case eCVT_EXTERNAL:
		Result = Cex(_T("%s%d]"), CCompilerEquation::m_cszExternalVar, pVarEnum->m_nIndex);
		break;
	case eCVT_EXTERNALSETPOINT:
		Result = Cex(_T("%s%d]"), CCompilerEquation::m_cszSetpointVar, pVarEnum->m_nIndex);
		break;
	case eCVT_CONST:
		Result = Cex(_T("%s%d]"), CCompilerEquation::m_cszConstVar, pVarEnum->m_nIndex);
	case eCVT_HOST:
		Result = Cex(_T("%s%s"), CCompilerEquation::m_cszHostVar, GetTextValue());
		break;
	}
}

void CExpressionToken::GetEquationOperandType(const CCompilerEquation *pEquation, wstring& Result, VariableEnum*& pVarEnum) const
{
	pVarEnum = NULL;
	Result = GetTextValue();

	if (m_pEquation)
	{
		if (m_pEquation != pEquation)
		{
			Result = Cex(_T("%s%d]"), CCompilerEquation::m_cszInternalVar, m_pEquation->m_nIndex);
		}
		else
		{
			if (!IsNumeric())
			{
				pVarEnum = m_pParser->m_Variables.Find(Result);
				if (pVarEnum)
				{
					GetEquationVariableType(pEquation, pVarEnum, Result);
				}
			}
		}

	}
	else
	{
		if (!IsNumeric())
		{
			pVarEnum = m_pParser->m_Variables.Find(Result);
			if (pVarEnum)
			{
				GetEquationVariableType(pEquation, pVarEnum, Result);
			}
		}
	}
}

const _TCHAR* CExpressionToken::EvaluateText()
{
	const _TCHAR *pValue = GetTextValue();

	/*
	https://stackoverflow.com/questions/14175177/how-to-walk-binary-abstract-syntax-tree-to-generate-infix-notation-with-minimall
	1. You never need parentheses around the operator at the root of the AST.
	2. If operator A is the child of operator B, and A has a higher precedence than B, the parentheses around A can be omitted.
	3. If a left-associative operator A is the left child of a left-associative operator B with the same precedence, the parentheses around A can be omitted. A left-associative operator is one for which x A y A z is parsed as (x A y) A z.
	4. If a right-associative operator A is the right child of a right-associative operator B with the same precedence, the parentheses around A can be omitted. A right-associative operator is one for which x A y A z is parsed as x A (y A z).
	5. If you can assume that an operator A is associative, i.e. that (x A y) A z = x A (y A z) for all x,y,z, and A is the child of the same operator A, you can choose to omit parentheses around the child A. In this case, reparsing the expression will yield a different AST that gives the same result when evaluated.
	*/

	if (IsOperator())
	{
		if (GetType() == ETT_PLUS || GetType() == ETT_MUL)
		{
			wstring result;
			wstring right;
			for (CONSTTOKENITR it = m_Children.begin(); it != m_Children.end(); it++)
			{
				GetOperand(*it, right, false);
				result.insert(0, right);
				result.insert(0, _T(" "));
				result.insert(0, m_pFunctionInfo->m_strOperatorText);
				result.insert(0, _T(" "));
			}

			if (!result.empty())
				result.erase(0, 3);
			SetTextValue(result.c_str());
		}
		else
		{
			switch (ArgsCount())
			{
			case 1:
			{
				wstring right;
				GetOperand(*m_Children.begin(), right, false);
				wstring str = m_pFunctionInfo->m_strOperatorText + right;
				SetTextValue(str.c_str());
			}
			break;

			case 2:
			{
				CONSTTOKENITR it1 = m_Children.begin();
				CExpressionToken *ptk1 = *it1;
				it1++;
				CExpressionToken *ptk2 = *it1;

				wstring left, right;
				GetOperand(ptk2, left, false);
				GetOperand(ptk1, right, true);

				wstring str = left + m_pFunctionInfo->m_strOperatorText + right;
				SetTextValue(str.c_str());
			}
			break;

			default:
				_ASSERTE(0);
			}
		}
	}
	else if (IsFunction())
	{
		wstring str = CExpressionToken::GenerateFunction(this);
		SetTextValue(str.c_str());
	}
	return pValue;
}

// собирает текстовое представление ссылки на модель по 
// разобранным элементам ссылки - имени таблицы, ключа и свойства
const _TCHAR* CExpressionTokenModelLink::EvaluateText()
{
	if (nWait != MLS_END)
	{
		nWait = MLS_END;

		wstring str = GetTextValue();
		str.push_back(_T('['));
		size_t nCount = 0;
		for (TOKENITR it = m_Children.begin(); it != m_Children.end(); it++, nCount++)
		{
			str += (*it)->GetTextValue();
			if (nCount < m_Children.size() - 2)
				str.push_back(_T(','));
			else
				if (nCount < m_Children.size() - 1)
				{
					str.push_back(_T(']'));
					str.push_back(_T('.'));
				}
		}

		m_ChildrenSave = m_Children;
		m_Children.clear();

		SetTextValue(str.c_str());
	}
	return GetTextValue();
}

TOKENITR CExpressionToken::RemoveChild(TOKENITR& rit)
{
	(*rit)->Unparent(this);
	return m_Children.erase(rit);
}


TOKENITR CExpressionToken::RemoveChild(CExpressionToken* pChild)
{
	for (TOKENITR it = m_Children.begin(); it != m_Children.end(); it++)
		if (*it == pChild)
		{
			pChild->Unparent(this);
			m_pParser->SetChanged();
			return m_Children.erase(it);
		}

	return m_Children.end();
}

void CExpressionToken::Unparent(CExpressionToken* pParent)
{
	if (IsVariable())
	{
		VariableEnum *vEnum = m_pParser->m_Variables.Find(GetTextValue());
		if (vEnum)
		{
			if (vEnum->m_nRefCount > 0)
				vEnum->m_nRefCount--;
			m_Parents.erase(pParent);

			_ASSERTE(vEnum->m_nRefCount >= m_Parents.size());
		}
		m_pParser->SetChanged();
	}
	else
	{
		m_Parents.erase(pParent);
		m_pParser->SetChanged();
	}
}


void CExpressionToken::SetNumericConstant(double Constant)
{
	wstring TextValue;
	CExpressionToken::GetNumericText(Constant, TextValue);
	SetNumericConstant(TextValue.c_str());
}

void CExpressionToken::SetNumericConstant(const _TCHAR *cszNumericConstant)
{
	_ASSERTE(!IsVariable());

	if (m_eType != ETT_NUMERIC_CONSTANT)
	{
		RemoveChildren();
		m_pParser->SetChanged();
	}

	m_eType = ETT_NUMERIC_CONSTANT;
	m_pFunctionInfo = NULL;
	SetTextValue(cszNumericConstant);
}

long CExpressionToken::GetPinsCount() const
{
	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->GetPinsCount();
	else
		return 2;
}

size_t CExpressionToken::EquationsCount() const
{
	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->EquationsCount();
	else
		return 1;
}

bool CExpressionToken::IsHostBlock() const
{
	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->IsHostBlock();
	else
		return false;
}

const _TCHAR *CExpressionToken::GetBlockType() const
{
	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->GetBlockType();
	else
		return NULL;
}

bool CExpressionToken::RequireSpecialInit()
{
	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->SpecialInitFunctionName() != NULL;
	else
		return false;
}

const _TCHAR* CExpressionToken::SpecialInitFunctionName()
{
	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->SpecialInitFunctionName();
	else
		return NULL;
}

bool CExpressionToken::Simplify()
{
	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->Simplify(this);
	else
		return true;
}

bool CExpressionToken::Derivative(CExpressionToken *pChildToken, wstring& Result)
{
	if (pChildToken->IsConst())
		return false;

	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->Derivative(this, pChildToken, Result);
	else
		return false;
}

bool CExpressionToken::Expand()
{
	if (m_pFunctionInfo && m_pFunctionInfo->m_pTransform)
		return m_pFunctionInfo->m_pTransform->Expand(this);
	else
		return true;
}

void CExpressionToken::SetOperator(FunctionEnum* pOpEnum)
{
	m_eType = pOpEnum->m_eOperatorType;
	m_pFunctionInfo = pOpEnum;
}

bool ChildrenSortPredicate(const CExpressionToken* first, const CExpressionToken* second)
{
	wstring str1 = first->GetTextValue();
	wstring str2 = second->GetTextValue();
	return str1 > str2;
}

CExpressionToken* CExpressionToken::GetChild()
{
	if (m_Children.size() > 0)
		return *m_Children.begin();
	else
		return NULL;
}

CExpressionToken* CExpressionToken::GetParent()
{
	_ASSERTE(m_Parents.size()== 1);

	if (m_Parents.size() > 0)
		return *m_Parents.begin();
	else
		return NULL;
}


bool CExpressionToken::GetChildren(CExpressionToken*& pChild1, CExpressionToken*& pChild2)
{
	if (m_Children.size() > 1)
	{
		TOKENITR itBegin = m_Children.begin();
		pChild1 = *itBegin;
		itBegin++;
		pChild2 = *itBegin;
		return true;
	}
	return false;
}

// если данный токен является числом, то вовзращаем текстовое
// представление числа
double CExpressionToken::GetNumericValue() const
{
	if (IsNumeric())
	{
		return _tstof(GetTextValue());
	}
	else
	{
		_ASSERTE(IsNumeric());
		return 0.0;
	}
}


ptrdiff_t CExpressionToken::Compare(const CExpressionToken *pToCompare) const
{
	ptrdiff_t nIdentical = 0;

	CONSTPARSERSTACK Stack;

	Stack.push(this);
	Stack.push(pToCompare);
	

	while (!Stack.empty() && nIdentical == 0)
	{
		const CExpressionToken *pRight = Stack.top();
		Stack.pop();
		const CExpressionToken *pLeft  = Stack.top();
		Stack.pop();

		nIdentical = pRight->GetType() - pLeft->GetType();

		if (nIdentical == 0)
		{
			switch (pLeft->GetType())
			{
			case ETT_VARIABLE:
			case ETT_MODELLINK:
				nIdentical = _tcscmp(pRight->GetTextValue(), pLeft->GetTextValue());
				break;
			case ETT_NUMERIC_CONSTANT:
				{
					double diff = pRight->GetNumericValue() - pLeft->GetNumericValue();
					nIdentical = (diff > 0) ? 1 : (diff < 0) ? -1 : 0;
				}
				break;
			default:
				nIdentical = pLeft->ChildrenCount() - pRight->ChildrenCount();
				if ( nIdentical == 0)
				{
					CONSTTOKENITR itLeft  = pLeft->ChildrenBegin();
					CONSTTOKENITR itRight = pRight->ChildrenBegin();
					while (itLeft != pLeft->ChildrenEnd())
					{
						Stack.push(*itLeft);
						Stack.push(*itRight);
						itLeft++;
						itRight++;
					}
				}
				break;
			}
		}
	}

	return nIdentical;
}


void CExpressionToken::RemoveChildren()
{
	PARSERSTACK ParserStack;
	TOKENITR it = ChildrenBegin();

	while (it != ChildrenEnd())
	{
		ParserStack.push(*it);
		it = RemoveChild(*it);
	}

	while (!ParserStack.empty())
	{
		CExpressionToken *pToken = ParserStack.top();
		ParserStack.pop();

		if (pToken->GetType() != ETT_MODELLINK)
		{
			it = pToken->ChildrenBegin();
			while (it != pToken->ChildrenEnd())
			{
				ParserStack.push(*it);
				//(*it)->Unparent(pToken);
				it = pToken->RemoveChild(*it);
			}
			m_pParser->DeleteToken(pToken);
		}
	}
}

void CExpressionToken::Delete()
{
	RemoveChildren();
	m_pParser->DeleteToken(this);
}


CExpressionToken* CExpressionToken::NewNumericConstant(double dConstant)
{
	CExpressionToken *pNewToken = m_pParser->NewExpressionToken(ETT_NUMERIC_CONSTANT);
	pNewToken->SetNumericConstant(dConstant);
	return pNewToken;
}

CExpressionToken* CExpressionToken::NewExpressionToken(eExpressionTokenType eType)
{
	return m_pParser->NewExpressionToken(eType);
}


bool CExpressionToken::ChangeType(eExpressionTokenType eNewType)
{
	bool bRes = true;
	m_pParser->ChangeTokenType(this, eNewType);
	return bRes;
}

CExpressionToken* CExpressionToken::Clone()
{
	return m_pParser->CloneToken(this);
}

bool CExpressionToken::ParserChanged() const
{
	return m_pParser->GetChanged();
}


bool PredicateSortChildrenPlus(const CExpressionToken* pLhs, const CExpressionToken *pRhs)
{
	return pLhs->Compare(pRhs) < 0;
};

bool CExpressionToken::SortChildren()
{
	if (GetType() != ETT_MODELLINK)
	{
		PARSERSTACK Stack;
		TOKENSET Visited;
		Stack.push(this);

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
				if (pToken->GetType() == ETT_PLUS || pToken->GetType() == ETT_MUL)
				{
					TOKENLIST lst = pToken->m_Children;
					pToken->m_Children.sort(PredicateSortChildrenPlus);
					if (lst != pToken->m_Children)
						m_pParser->SetChanged();
				}
				Visited.insert(pToken);
				Stack.pop();
			}
		}
	}
	return m_pParser->GetChanged();
}


const CExpressionParser* CExpressionToken::GetParser()
{
	return m_pParser;
}

CExpressionRule::CExpressionRule(const _TCHAR *cszSource, const _TCHAR *cszDestination, const CExpressionParser* pParser)
{
	m_pSource = new CExpressionParserRule(pParser);
	m_pDestination = new CExpressionParserRule(pParser);
	m_pSource->Parse(cszSource);
	m_pSource->GetExpressionConstants();
	m_pSource->Simplify();
	m_pSource->RemoveRoot();
	m_pDestination->Parse(cszDestination);
	m_pDestination->GetExpressionConstants();
	m_pDestination->Simplify();
	m_pDestination->RemoveRoot();
}


CExpressionRule::CExpressionRule(const _TCHAR *cszSource, const _TCHAR *cszDestination)
{
	m_pSource = new CExpressionParserRule();
	m_pDestination = new CExpressionParserRule();
	m_pSource->Parse(cszSource);
	m_pSource->GetExpressionConstants();
	m_pSource->Simplify();
	m_pSource->RemoveRoot();
	m_pDestination->Parse(cszDestination);
	m_pDestination->GetExpressionConstants();
	m_pDestination->Simplify();
	m_pDestination->RemoveRoot();
}

CExpressionRule::~CExpressionRule()
{
	delete m_pSource;
	delete m_pDestination;
}

// функция формирует текстовое представление числа с максимальной точностью
// если число не имеет дробной части, к текстовому представлению добавляем ".0"
// чтобы у C-компилятора не возникло сомнений что тип double
void CExpressionToken::GetNumericText(double dValue, wstring& TextValue)
{
	_TCHAR buf[100];
	_stprintf_s(buf, 100, _T("%.30g"), dValue);
	double fc = 0;
	TextValue = buf;
	if (modf(dValue, &fc) == 0.0)
		TextValue += _T(".0");

}

bool CExpressionToken::Join(CExpressionToken *pToken)
{
	for (TOKENSETITR it = pToken->m_Parents.begin(); it != pToken->m_Parents.end(); it++)
	{
		CExpressionToken *pParent = *it;
		pParent->ReplaceChild(pToken, this);
	}

	pToken->m_Parents.clear();

	return true;
}


// Проверяем токен на постоянство (не является функцией чего либо, кроме времени)
// Используется для того, чтобы опредилить - вставлять в матрицу производную от токена или нет

bool CExpressionToken::IsConst() const
{ 
	if (IsNumeric())
		return true;

	if (IsVariable())
	{
		VariableEnum *pEnum = m_pParser->m_Variables.Find(GetTextValue());
		if (pEnum)
		{
			if (pEnum->m_eVarType == eCVT_EXTERNALSETPOINT || pEnum->m_eVarType == eCVT_CONST || pEnum->m_eVarType == eCVT_HOST)
				return true;
		}
	}
	return false;
}

// Проверяем токен на постоянство (не является функцией чего либо, в том числе времени)
// Используется для того, чтобы опредилить - можно ли использовать токен для инициализации блока

bool CExpressionToken::IsConstParameter() const
{
	if (IsNumeric())
		return true;

	if (IsVariable())
	{
		VariableEnum *pEnum = m_pParser->m_Variables.Find(GetTextValue());
		if (pEnum)
		{
			if (pEnum->m_eVarType == eCVT_EXTERNALSETPOINT || pEnum->m_eVarType == eCVT_CONST)
				return true;
		}
	}
	return false;
}

const _TCHAR* CExpressionToken::GetBlockName() const
{
	return _T("");
}

wstring CExpressionToken::GenerateFunction(CExpressionToken *pToken, bool bEquationMode)
{
	_ASSERTE(pToken->IsFunction());
	_ASSERTE(pToken->m_pFunctionInfo);

	wstring result = pToken->m_pFunctionInfo->m_strOperatorText + _T("(");
	ptrdiff_t nCount = 1000;
	wstring right;

	CExpressionTransform *pTransform = pToken->m_pFunctionInfo->m_pTransform;

	//if (pTransform && pTransform->IsHostBlock())
	//	nCount = pTransform->GetPinsCount() - 1;

	for (RTOKENITR it = pToken->FirstChild(); it != pToken->LastChild(); it++)
	{
		if (bEquationMode)
		{
			if (nCount > 0)
			{
				(*it)->GetEquationOperand(pToken->m_pEquation, right);
				nCount--;
			}
			else
				right = (*it)->GetTextValue();
		}
		else
			right = (*it)->GetTextValue();

		if (it != pToken->FirstChild())
			result += _T(", ");

		result += right;
	}
	
	result += _T(")");

	return result;
}

ptrdiff_t VariableEnum::GetIndex() const
{
	if (m_eVarType == eCVT_INTERNAL)
	{
		_ASSERTE(m_pToken);
		_ASSERTE(m_pToken->m_pEquation);
		return m_pToken->m_pEquation->m_nIndex;
	}
	else
		return m_nIndex;
}

bool CExpressionToken::CheckTokenIsConstant(const CExpressionToken *pToken)
{
	bool bConstant = true;

	_ASSERTE(pToken);

	CONSTPARSERSTACK Stack;
	Stack.push(pToken);

	// проверяем, есть ли в дереве токена хотя бы одна переменная не константа
	while (!Stack.empty() && bConstant)
	{
		const CExpressionToken *pStackToken = Stack.top();
		Stack.pop();

		// проверяем параметр на постоянство, в том числе и во времени
		if (!pStackToken->IsConstParameter())
		{
			if (pStackToken->IsVariable())
			{
				// если переменная есть, и она не константа
				const CCompilerEquation *pEquation = pStackToken->m_pEquation;
				if (pEquation)
				{
					// проверяем, есть ли у нее уравнение
					const CExpressionToken *pEqToken = pEquation->m_pToken;

					// и если уравнение указывает не на обрабатываемую переменную
					// кладем ее в стек. Это нужно потому, что константные переменные не входят
					// в систему напрямую, а только через уравнение с внутренней переменной,
					// поэтому и нужно добраться от внутренней переменной ко внешней
					if (pEqToken != pStackToken)
						Stack.push(pEqToken);
					else
						bConstant = false;

					// иначе переменная не является константой. Внешние переменные уравнением
					// указывают сами на себя
				}
				else
					bConstant = false;
			}
			else
			{
				// если найдена не переменная, перебираем дочерние токены и кладем в стек
				// если они не константы
				for (CONSTTOKENITR it = pStackToken->ChildrenBegin(); it != pStackToken->ChildrenEnd(); it++)
				{
					const CExpressionToken *pChildToken = *it;
					if (!pChildToken->IsConstParameter())
						Stack.push(pChildToken);
				}
			}
		}
	}

	return bConstant;
}