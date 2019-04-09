#include "..\stdafx.h"
#include "ExpressionParser.h"

bool CTransformPow::Expand(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_POW || pToken->GetType() == ETT_POWF);

	bool bRes = true;

	const CExpressionParser *pParser = pToken->GetParser();

	bRes = bRes && SimplifyPowConstant(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyPowNested(pToken);

	return bRes;
}


bool CTransformPow::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_POW || pToken->GetType() == ETT_POWF);

	bool bRes = true;

	const CExpressionParser *pParser = pToken->GetParser();

	bRes = bRes && SimplifyPowConstant(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyPowChain(pToken);

	return bRes;
}


bool CTransformExp::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_EXP);

	bool bRes = true;

	CExpressionToken *pExp = pToken->GetChild();
	if (pExp && pExp->IsNumeric())
	{
		pToken->SetNumericConstant(exp(pExp->GetNumericValue()));
	}

	return bRes;
}


bool CTransformExp::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformExp::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;
	
	_ASSERTE(pToken->GetType() == ETT_EXP);

	CExpressionToken *pExp = pToken->GetChild();

	if (pExp)
	{
		wstring Operand;
		pExp->GetEquationOperand(pToken->m_pEquation, Operand);
		Result = Cex(_T("exp(%s)"), Operand.c_str());
		bRes = true;
	}

	return bRes;
}


bool CTransformPow::SimplifyPowConstant(CExpressionToken* pToken)
{
	bool bRes = true;

	pToken->ChangeType(ETT_POWF);

	CExpressionToken *pExp, *pBase;

	if (pToken->GetChildren(pExp, pBase))
	{
		if (pExp->IsNumeric())
		{
			if (pBase->IsNumeric())
			{
				double dExp	 = pExp->GetNumericValue();
				double dBase = pBase->GetNumericValue();
				double dInt = 0.0;
				if (dExp < 0.0 && dBase == 0.0)
				{
					bRes = false;
					pToken->SetError(ETT_ERROR_DIVISIONBYZERO);
				}
				else if (dBase < 0.0 && modf(dExp, &dInt) != 0.0)
				{
					bRes = false;
					pToken->SetError(ETT_ERROR_NEGATIVEROOT);
				}
				else
					pToken->SetNumericConstant(pow(pBase->GetNumericValue(), pExp->GetNumericValue()));
			}
			else
			{
				if (pExp->GetNumericValue() == 0.0)
					pToken->SetNumericConstant(1.0);
				else
				if (pExp->GetNumericValue() == 1.0)
				{
					CExpressionToken *pExp = pToken->GetChild();
					pToken->RemoveChild(pExp);
					pExp->Delete();
					pToken->Eliminate();
					pToken->Delete();
				}
			}
		}
	}

	return bRes;
}

bool CTransformPow::SimplifyPowChain(CExpressionToken* pToken)
{
	bool bRes = true;

	CExpressionToken *pExp, *pBase;

	if (pToken->GetChildren(pExp, pBase))
	{
		if (pBase->GetType() == ETT_POWF)
		{
			CExpressionToken *pExp1 = NULL, *pBase1 = NULL;
			if (pBase->GetChildren(pExp1, pBase1))
			{
				CExpressionToken *pNewMul = pToken->NewExpressionToken(ETT_MUL);
				pBase->RemoveChild(pExp1);
				pBase->RemoveChild(pBase1);
				pToken->ReplaceChild(pExp, pNewMul, 0);
				pToken->ReplaceChild(pBase, pBase1, 1);
				pNewMul->AddChild(pExp1);
				pNewMul->AddChild(pExp);
				pBase->Delete();
			}
		}
	}

	return bRes;
}

bool CTransformPow::SimplifyPowNested(CExpressionToken* pToken)
{
	bool bRes = true;

	CExpressionToken *pExp, *pBase;

	if (pToken->GetChildren(pExp, pBase))
	{
		if (pBase->GetType() == ETT_MUL)
		{
			pToken->RemoveChild(pBase);
			pToken->RemoveChild(pExp);
			pToken->ChangeType(ETT_MUL);

			int nCount = 0;
			TOKENITR it = pBase->ChildrenBegin();

			while ( it != pBase->ChildrenEnd())
			{
				CExpressionToken *pMul = *it;
				it = pBase->RemoveChild(*it);
				CExpressionToken *pNewPow = pToken->NewExpressionToken(ETT_POWF);
				pNewPow->AddChild(pExp->Clone());
				pNewPow->AddChild(pMul);
				pToken->AddChild(pNewPow);
				nCount++;
			}
			pExp->Delete();
			pBase->Delete();
		}
	}

	return bRes;
}

bool CTransformSqrt::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}


bool CTransformSqrt::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_SQRT);

	bool bRes = true;
	CExpressionToken *pChild = pToken->GetChild();
	pToken->RemoveChild(pChild);
	pToken->ChangeType(ETT_POW);
	pToken->AddChild(pToken->NewNumericConstant(0.5));
	pToken->AddChild(pChild);
	return bRes;
}


bool CTransformLn::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_LN);

	bool bRes = true;

	CExpressionToken *pExp = pToken->GetChild();
	if (pExp && pExp->IsNumeric())
	{
		double dExp = pExp->GetNumericValue();
		if (dExp < 0.0)
		{
			bRes = false;
			pToken->SetError(ETT_ERROR_NEGATIVELOG);
		}
		else
			pToken->SetNumericConstant(log(dExp));
	}

	return bRes;
}

bool CTransformLn::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_LN);
	_ASSERTE(pToken->ChildrenCount() == 1);

	if (pToken->GetChild() == pChildToken)
	{
		pChildToken->GetEquationOperand(pToken->m_pEquation, Result);
		Result = _T("(1.0 / ") + Result + _T(")"); 
		bRes = true;
	}

	return bRes;
}

bool CTransformLn::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}


bool CTransformLog::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_LOG);

	bool bRes = true;

	CExpressionToken *pExp = pToken->GetChild();
	if (pExp && pExp->IsNumeric())
	{
		double dExp = pExp->GetNumericValue();
		if (dExp < 0.0)
		{
			bRes = false;
			pToken->SetError(ETT_ERROR_NEGATIVELOG);
		}
		else
			pToken->SetNumericConstant(log10(dExp));
	}

	return bRes;
}


bool CTransformLog::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformLog::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_LOG);
	_ASSERTE(pToken->ChildrenCount() == 1);

	if (pToken->GetChild() == pChildToken)
	{
		pChildToken->GetEquationOperand(pToken->m_pEquation, Result);
		// dlog10(x)/dx = 1 / log(10) / x;
		Result = _T("(0.43429448190325182765112891891661 / ") + Result + _T(")");
		bRes = true;
	}

	return bRes;
}

bool CTransformPow::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_POWF || pToken->GetType() == ETT_POW);
	_ASSERTE(pToken->ChildrenCount() == 2);

	CExpressionToken *pExp = NULL, *pBase = NULL;
	
	if (pToken->GetChildren(pExp, pBase))
	{
		wstring strExp, strBase;
		pExp->GetEquationOperand(pToken->m_pEquation, strExp);
		pBase->GetEquationOperand(pToken->m_pEquation, strBase);

		if (pChildToken == pBase)
		{
			if (pExp->IsNumeric())
			{
				double dExp = pExp->GetNumericValue() - 1.0;
				Result = strExp + _T(" * ");
				if (dExp != 1.0)
				{
					CExpressionToken::GetNumericText(dExp, strExp);
					Result += Cex(_T("%s%s, %s)"), cszPow, strBase.c_str(), strExp.c_str());
				}
				else
				{
					Result += strBase;
				}

			}
			else
			{
				Result = Cex(_T("%s * %s%s, %s-1)"), strExp.c_str(), cszPow, strBase.c_str(), strExp.c_str());
			}
			bRes = true;
		}
		else if (pChildToken == pExp)
		{
			Result = Cex(_T("%s%s,%s)*ln(%s)"), cszPow, strBase.c_str(), strExp.c_str(), strBase.c_str());
			bRes = true;
		}
	}

	return bRes;
}

const _TCHAR *CTransformPow::cszPow = _T("pow(");