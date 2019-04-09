#include "..\stdafx.h"
#include "ExpressionParser.h"

wstring CCompilerEquation::Generate(bool bInitEquation) const
{
	wstring result;
	wstring right;

	_ASSERTE(m_pToken);

	if (m_pToken->IsFunction())
	{
		result = CExpressionToken::GenerateFunction(m_pToken, true);
		if (bInitEquation && m_pToken->IsHostBlock())
			result.insert(0, _T("init_"));
	}
	else
	if (m_pToken->IsOperator())
	{
		if (m_pToken->GetType() == ETT_PLUS || m_pToken->GetType() == ETT_MUL)
		{
			for (TOKENITR it = m_pToken->ChildrenBegin(); it != m_pToken->ChildrenEnd(); it++)
			{
				(*it)->GetEquationOperand(this, right);
				result.insert(0, right);
				result.insert(0, _T(" "));
				result.insert(0, m_pToken->m_pFunctionInfo->m_strOperatorText);
				result.insert(0, _T(" "));
			}
			if (!result.empty())
				result.erase(0, 3);
		}
		else
		{
			switch (m_pToken->ArgsCount())
			{
			case 1:
			{
				m_pToken->GetChild()->GetEquationOperand(this, right);
				if (bInitEquation && m_pToken->RequireSpecialInit())
					result = Cex(_T("init_%s(%s)"), m_pToken->SpecialInitFunctionName(), right.c_str());
				else
					result = m_pToken->m_pFunctionInfo->m_strOperatorText + right;
			}
			break;

			case 2:
			{
				TOKENITR it1 = m_pToken->ChildrenBegin();
				CExpressionToken *ptk1 = *it1;
				it1++;
				CExpressionToken *ptk2 = *it1;

				if (bInitEquation && m_pToken->RequireSpecialInit())
				{
					ptk2->GetEquationOperand(this, result);
					ptk1->GetEquationOperand(this, right);
					result = Cex(_T("init_%s(%s,%s)"), m_pToken->SpecialInitFunctionName(), result.c_str(), right.c_str());
				}
				else
				{
					ptk2->GetEquationOperand(this, right);
					result = right + m_pToken->m_pFunctionInfo->m_strOperatorText;
					ptk1->GetEquationOperand(this, right);
					result += right;
				}
			}
			break;

			default:
				_ASSERTE(0);
			}
		}
	}
	else if (m_pToken->IsNumeric())
	{
		result = m_pToken->GetTextValue();
	}
	else if (m_pToken->IsVariable())
	{
		m_pToken->GetEquationOperand(this, result);
	}

	return result.c_str();
}

bool CCompilerEquation::IsParentNeedEquation(CExpressionToken *pToken)
{
	_ASSERTE(pToken);

	for (TOKENSETITR it = pToken->m_Parents.begin(); it != pToken->m_Parents.end(); it++)
	{
		CExpressionToken *pParent = *it;
		if (pParent && (pParent->IsHostBlock() || pParent->IsVariable() || pParent->GetType() == ETT_ROOT)) return true;
	}

	return  false;
}

const _TCHAR *CCompilerEquation::m_cszInternalVar = _T("pArgs->pEquations[");
const _TCHAR *CCompilerEquation::m_cszExternalVar = _T("pArgs->pExternals[");
const _TCHAR *CCompilerEquation::m_cszConstVar = _T("pArgs->pConsts[");
const _TCHAR *CCompilerEquation::m_cszSetpointVar = _T("pArgs->pSetPoints[");
const _TCHAR *CCompilerEquation::m_cszHostVar = _T("pArgs->");
