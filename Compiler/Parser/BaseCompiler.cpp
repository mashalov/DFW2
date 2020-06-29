#include "..\stdafx.h"
#include "BaseCompiler.h"

bool CCompilerEquations::AddEquation(CExpressionToken *pToken)
{
	bool bRes = true;

	if (pToken->GetType() == ETT_ROOT) return bRes;

	if (pToken->IsOperator() || pToken->IsFunction())
	{
		_ASSERTE(!pToken->m_pEquation);

		size_t nEquations = pToken->EquationsCount();

		if (pToken->IsHostBlock())
			m_HostBlocks.push_back(pToken);

		while (nEquations > 0)
		{
			CCompilerEquation *pEquation = new CCompilerEquation(pToken, m_Equations.size());

			if (!pToken->m_pEquation)
				pToken->m_pEquation = pEquation;

			m_Equations.push_back(pEquation);
			nEquations--;
		}
	}
	else
	{
		
		if (CCompilerEquation::IsParentNeedEquation(pToken) && !pToken->m_pEquation)
		{
			bool bAddEquation = true;
			if (pToken->IsVariable() /*&& *pToken->GetTextValue() == _T('S')*/)
			{
				VariableEnum *pVarEnum = m_Compiler.m_Variables.Find(pToken->GetTextValue());
				if (pVarEnum)
					if (pVarEnum->m_eVarType == eCVT_INTERNAL)
						bAddEquation = false;
			}

			if (bAddEquation)
			{
				CCompilerEquation *pEquation = new CCompilerEquation(pToken, m_Equations.size());
				pToken->m_pEquation = pEquation;
				m_Equations.push_back(pEquation);
			}
		}
	}

	return bRes;
}

CCompilerEquations::~CCompilerEquations()
{
	for (auto&& it : m_Equations)
		delete it;
}

long CCompilerEquations::GetBlockIndex(const CExpressionToken *pToken) const
{
	long nCount = 0;
	const TOKENLIST& Blocks = m_Compiler.GetEquations().GetBlocks();
	for (CONSTTOKENITR bit = Blocks.begin(); bit != Blocks.end(); bit++, nCount++)
	{
		if (*bit == pToken)
			break;
	}

	_ASSERTE(nCount < static_cast<long>(Blocks.size()));

	return nCount;
}


bool CCompilerEquations::GenerateEquations()
{
	bool bRes = SortEquations();

	if (bRes)
	{
		const TOKENLIST& Blocks = m_Compiler.GetEquations().GetBlocks();

		for (CONSTTOKENITR bit = Blocks.begin(); bit != Blocks.end() ;bit++)
		{
			const CExpressionToken *pToken = *bit;

			size_t nPinCount = 0;
			size_t nPins = pToken->GetPinsCount() - 1;

			for (CONSTRTOKENITR pit = pToken->FirstChild(); pit != pToken->LastChild(); pit++, nPinCount++)
			{
				const CExpressionToken *pChildToken = *pit;
				if (nPinCount >= nPins)
				{
					CCompilerEquation *pEquation = pChildToken->m_pEquation;
					size_t nParameterIndex = nPinCount - nPins;
					if (!CExpressionToken::CheckTokenIsConstant(pChildToken))
					{
						m_Compiler.m_Logger.Log(fmt::format(CAutoCompilerMessages::cszNotConstParameter,
															pToken->GetTextValue(),
															nPinCount + 1,
															pChildToken->GetTextValue()));
						bRes = false;
					}
				}
			}
		}

		if (bRes)
		{
			for (EQUATIONSITR it = m_Equations.begin(); it != m_Equations.end(); it++)
			{
				CCompilerEquation *pEquation = *it;
				if (!pEquation->m_pToken->IsHostBlock())
				{
					std::wstring str = pEquation->Generate();
					_tcprintf(_T("\n%3d %s"), pEquation->m_nIndex, str.c_str());
				}
				else
				{
					std::wstring str = pEquation->Generate();
					_tcprintf(_T("\n // %3d %s"), pEquation->m_nIndex, str.c_str());
				}
			}
		}
	}

	return bRes;
}


bool CCompilerEquations::GetDerivative(CExpressionToken *pToken, 
									   CExpressionToken *pTokenBy, 
									   std::wstring& Diff,
									   std::wstring& strCol,
									   bool& bExternalGuard) const
{
	bExternalGuard = false;

	bool bRes = pToken->Derivative(pTokenBy,Diff);
	if (bRes)
	{
		if (pTokenBy->m_pEquation)
			strCol = fmt::format(_T("{}"), pTokenBy->m_pEquation->m_nIndex);
		else if (pTokenBy->IsVariable())
			bExternalGuard = pTokenBy->GetEquationOperandIndex(pToken->m_pEquation, strCol);
		bRes = true;
	}
	
	return bRes;
}

bool CCompilerEquations::GenerateMatrix()
{
	bool bRes = true;
	std::wstring row, col;

	for (EQUATIONSITR it = m_Equations.begin(); it != m_Equations.end(); it++)
	{
		CCompilerEquation *pEquation = *it;
		CExpressionToken *pToken = pEquation->m_pToken;

		if (!pToken->IsHostBlock())
		{
			std::wstring Diagonal, Diff, strCol;

			if (pToken->IsVariable() && !pToken->IsConst())
			{
				pToken->GetEquationOperandIndex(pEquation, strCol);
				Diff = _T("1");
				_tcprintf(_T("\n[%5d, %30s] -(%s)"), pEquation->m_nIndex, strCol.c_str(), Diff.c_str());
			}
			else
			{
				for (TOKENLIST::reverse_iterator tkit = pToken->m_Children.rbegin(); tkit != pToken->m_Children.rend() && bRes; tkit++)
				{
					CExpressionToken *pChildToken = *tkit;
					bool bExternalGuard = false;
					if (GetDerivative(pToken, pChildToken, Diff, strCol, bExternalGuard))
					{

						std::wstring DiagIndex = fmt::format(_T("{}"), pEquation->m_nIndex);
						if (DiagIndex == strCol)
						{
							Diagonal = Diff;
							if (Diff == _T("1"))
							{
								m_Compiler.m_Logger.Log(fmt::format(CAutoCompilerMessages::cszSingularMatrix));
								bRes = false;
							}
						}
						else
							_tcprintf(_T("\n[%5d, %30s] -(%s)"), pEquation->m_nIndex, strCol.c_str(), Diff.c_str());
					}
				}
			}

			if (bRes)
			{
				if (Diagonal.empty())
					_tcprintf(_T("\n[%5d, %30d] 1.0"), pEquation->m_nIndex, pEquation->m_nIndex);
				else
					_tcprintf(_T("\n[%5d, %30d] 1.0 - (%s)"), pEquation->m_nIndex, pEquation->m_nIndex, Diagonal.c_str());
			}
		}
	}
	return bRes;
}

bool CExpressionParser::InsertEquations(CCompilerEquations& Eqs)
{
	bool bRes = true;

	_ASSERTE(m_ResultStack.size() > 0);

	if (m_ResultStack.size() != 1)
		return false;

	CExpressionToken *pRoot = m_ResultStack.top();
	CExpressionToken *pChild = pRoot->GetChild();

	PARSERSTACK Stack;
	Stack.push(pRoot);
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
			Eqs.AddEquation(pToken);
			Stack.pop();
		}
	}

	return bRes;
}

VariableEnum* CCompilerItem::CreateInternalVariable(const _TCHAR* cszVarName)
{
	VariableEnum *pVar = m_pParser->CreateInternalVariable(cszVarName);
	return pVar;
}

VariableEnum* CCompilerItem::CreateOutputVariable(const _TCHAR* cszVarName)
{
	VariableEnum *pVar = CreateInternalVariable(cszVarName);
	m_Compiler.AddOutputVariable(cszVarName, pVar);
	return pVar;
}

bool CCompilerItem::AddSpecialVariables()
{
	bool bRes = true;

	CParserVariables& vars = m_pParser->m_Variables;
	for (VARIABLECONSTITR vit = vars.Begin(); vit != vars.End(); vit++)
	{
		if (vit->second.m_eVarType == eCVT_EXTERNAL)
		{
			const COMPILERVARS &compVars = m_Compiler.GetEquations().GetExternalVars();
			if (compVars.find(vit->first) == compVars.end())
				bRes = bRes && m_Compiler.AddExternalVariable(vit->first.c_str(), vars.Find(vit->first.c_str()));
		}
	}

	return bRes;
}

CCompilerItem::~CCompilerItem()
{
	if (m_pParser)
		delete m_pParser;
}

bool CCompiler::AddOutputVariable(const _TCHAR *cszVarName, VariableEnum* pVarEnum)
{
	return m_Equations.AddOutputVariable(cszVarName, pVarEnum);
}

bool CCompiler::AddExternalVariable(const _TCHAR *cszVarName, VariableEnum* pVarEnum)
{
	return m_Equations.AddExternalVariable(cszVarName, pVarEnum);
}

bool CCompiler::AddSetPoint(const _TCHAR *cszVarName, VariableEnum* pVarEnum)
{
	return m_Equations.AddSetPoint(cszVarName, pVarEnum);
}

bool CCompiler::AddConst(const _TCHAR *cszVarName, VariableEnum* pVarEnum)
{
	return m_Equations.AddConst(cszVarName, pVarEnum);
}

bool CCompilerEquations::AddOutputVariable(const _TCHAR* cszVarName, VariableEnum *pVarEnum)
{
	bool bRes = m_Output.insert(std::make_pair(cszVarName, pVarEnum)).second;
	if (bRes) pVarEnum->m_nIndex = m_Output.size() - 1;
	return bRes;
}

bool CCompilerEquations::AddExternalVariable(const _TCHAR* cszVarName, VariableEnum *pVarEnum)
{
	bool bRes = m_VarExternal.insert(std::make_pair(cszVarName, pVarEnum)).second;
	if (bRes) pVarEnum->m_nIndex = m_VarExternal.size() - 1;
	return bRes;
}

bool CCompilerEquations::AddSetPoint(const _TCHAR* cszVarName, VariableEnum *pVarEnum)
{
	bool bRes = m_VarSetpoint.insert(std::make_pair(cszVarName, pVarEnum)).second;
	if (bRes) pVarEnum->m_nIndex = m_VarSetpoint.size() - 1;
	return bRes;
}

bool CCompilerEquations::AddConst(const _TCHAR* cszVarName, VariableEnum *pVarEnum)
{
	bool bRes = m_VarConst.insert(std::make_pair(cszVarName, pVarEnum)).second;
	if (bRes) pVarEnum->m_nIndex = m_VarConst.size() - 1;
	return bRes;
}

bool CCompilerEquations::FindEquationInList(EQUATIONSLIST& NewList, const CCompilerEquation *pEquation)
{
	bool bFound = true;
	EQUATIONSITR eit = NewList.begin();
	while (eit != NewList.end())
	{
		if (pEquation->m_nIndex == (*eit)->m_nIndex)
			break;
		else
			eit++;
	}

	if (eit == NewList.end())
		bFound = false;

	return bFound;
}

bool CCompilerEquations::IsTokenInPlace(EQUATIONSLIST& NewList, CExpressionToken *pToken)
{
	bool bInPlace = true;

	if (pToken->IsVariable() && !pToken->IsConst())
	{
		VariableEnum *pVarEnum = m_Compiler.m_Variables.Find(pToken->GetTextValue());
		if (pVarEnum->m_eVarType == eCVT_INTERNAL)
		{
			CCompilerEquation *pEquation = pToken->m_pEquation;
			if (pEquation)
				if (!FindEquationInList(NewList, pEquation))
					bInPlace = false;
		}
	}
	else
	{
		for (TOKENITR tkit = pToken->ChildrenBegin(); tkit != pToken->ChildrenEnd(); tkit++)
		{
			CExpressionToken *pChildToken = *tkit;

			if (pChildToken->IsVariable())
			{
				VariableEnum *pEnum = m_Compiler.m_Variables.Find(pChildToken->GetTextValue());
				if (pEnum->m_eVarType != eCVT_INTERNAL) continue;
				if (pEnum->m_pToken->m_pEquation)
					pChildToken = pEnum->m_pToken->m_pEquation->m_pToken;
				else
					_ASSERTE(0);
			}

			if (pChildToken->m_pEquation)
			{
				if (!FindEquationInList(NewList, pChildToken->m_pEquation))
				{
					bInPlace = false;
					break;
				}
			}
		}
	}

	return bInPlace;
}

void CCompilerEquations::FlushStack(EQUATIONSLIST& Stack, EQUATIONSLIST& NewList)
{
	bool bEmit = true;
	while (!Stack.empty() && bEmit)
	{
		bEmit = false;

		EQUATIONSITR it = Stack.begin();
		while (it != Stack.end())
		{
			CCompilerEquation *pStackedEquation = *it;

			if (IsTokenInPlace(NewList, pStackedEquation->m_pToken))
			{
				NewList.push_back(pStackedEquation);
				_tcprintf(_T("\n %d --> %s"), pStackedEquation->m_nIndex, pStackedEquation->Generate().c_str());
				it = Stack.erase(it);
				bEmit = true;
			}
			else
				it++;
		}
	}
}

bool CCompilerEquations::SortEquations()
{
	bool bRes = true;

	EQUATIONSLIST Stack;
	EQUATIONSLIST newList;

	for (EQUATIONSCONSTITR eit = m_Equations.begin(); eit != m_Equations.end(); eit++)
	{
		CCompilerEquation *pEquation = *eit;
		CExpressionToken *pToken = pEquation->m_pToken;

		if (IsTokenInPlace(newList, pToken))
		{
			newList.push_back(pEquation);
			_tcprintf(_T("\n %d --> %s"), pEquation->m_nIndex, pEquation->Generate().c_str());
			FlushStack(Stack, newList);
		}
		else
		{
			Stack.push_back(pEquation);
		}
	}
	
	FlushStack(Stack, newList);

	if (!Stack.empty())
	{
		m_Compiler.m_Logger.Log(CAutoCompilerMessages::cszAlgebraicLoop);
		for (EQUATIONSITR eit = Stack.begin(); eit != Stack.end(); eit++)
		{
			m_Compiler.m_Logger.Log((*eit)->m_pToken->GetTextValue());
		}
		bRes = false;
	}
	else
		m_Equations = newList;

	return bRes;
}
