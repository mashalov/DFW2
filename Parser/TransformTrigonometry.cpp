#include "..\stdafx.h"
#include "ExpressionParser.h"


bool CTransformSin::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformCos::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformTan::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformSin::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_SIN);

	bool bRes = true;

	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		if (pArg->IsNumeric())
		{
			pToken->SetNumericConstant(sin(pArg->GetNumericValue()));
		}
	}

	return bRes;
}

bool CTransformCos::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_COS);

	bool bRes = true;

	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		if (pArg->IsNumeric())
		{
			pToken->SetNumericConstant(cos(pArg->GetNumericValue()));
		}
	}

	return bRes;
}

bool CTransformTan::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_TAN);

	bool bRes = true;

	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		if (pArg->IsNumeric())
		{
			// check division by zero
			pToken->SetNumericConstant(tan(pArg->GetNumericValue()));
		}
	}

	return bRes;
}

bool CTransformTan::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	_ASSERTE(pToken->GetType() == ETT_TAN);
	_ASSERTE(pToken->ChildrenCount() == 1);

	bool bRes = false;

	if (pToken->GetChild() == pChildToken)
	{
		pChildToken->GetEquationOperand(pToken->m_pEquation, Result);
		Result = _T("tan(") + Result + _T(")");
		Result = Result + _T(" * ") + Result;
		Result = _T("1 + ") + Result;
		bRes = true;
	}

	return bRes;

}

bool CTransformAtan2::Simplify(CExpressionToken* pToken)
{
	return Expand(pToken);
}

bool CTransformAtan2::Expand(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_ATAN2);

	bool bRes = true;

	CExpressionToken *pDen, *pNom;

	if (pToken->GetChildren(pDen, pNom))
	{
		if (pDen->IsNumeric() && pNom->IsNumeric())
		{
			pToken->SetNumericConstant(atan2(pNom->GetNumericValue(), pDen->GetNumericValue()));
		}
	}

	return bRes;
}

bool CTransformAtan2::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_ATAN2);
	_ASSERTE(pToken->ChildrenCount() == 2);

	CExpressionToken *pDen, *pNom;

	if (pToken->GetChildren(pDen,pNom))
	{
		wstring Den, Nom;
		pDen->GetEquationOperand(pDen->m_pEquation, Den);
		pNom->GetEquationOperand(pNom->m_pEquation, Nom);
		if (pChildToken == pDen)
		{
			Result = Cex(_T("(-%s / (%s*%s + %s*%s))"), Nom.c_str(), Nom.c_str(), Nom.c_str(), Den.c_str(), Den.c_str());
			bRes = true;
		}
		else
		if (pChildToken == pNom)
		{
			Result = Cex(_T("(%s / (%s*%s + %s*%s))"), Den.c_str(), Nom.c_str(), Nom.c_str(), Den.c_str(), Den.c_str());
			bRes = true;
		}
	}

	return bRes;
}

bool CTransformAsin::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformAsin::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_ASIN);

	bool bRes = true;

	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		if (pArg->IsNumeric())
		{
			double dArg = pArg->GetNumericValue();
			if (dArg <= 1.0 && dArg >= -1.0)
				pToken->SetNumericConstant(asin(dArg));
			else
			{
				bRes = false;
				pToken->SetError(ETT_ERROR_WRONGARG);
			}
		}
	}

	return bRes;
}

bool CTransformAsin::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_ASIN);
	_ASSERTE(pToken->ChildrenCount() == 1);

	if (pToken->GetChild() == pChildToken)
	{
		pChildToken->GetEquationOperand(pToken->m_pEquation, Result);
		Result = _T("1.0 / sqrt(1.0 - ") + Result + _T(" * ") + Result + _T(")");
		bRes = true;
	}

	return bRes;
}

bool CTransformAcos::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}


bool CTransformAcos::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_ACOS);

	bool bRes = true;

	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		if (pArg->IsNumeric())
		{
			double dArg = pArg->GetNumericValue();
			if (dArg <= 1.0 && dArg >= -1.0)
				pToken->SetNumericConstant(acos(dArg));
			else
			{
				bRes = false;
				pToken->SetError(ETT_ERROR_WRONGARG);
			}
		}
	}
	return bRes;
}

bool CTransformAcos::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_ACOS);
	_ASSERTE(pToken->ChildrenCount() == 1);

	if (pToken->GetChild() == pChildToken)
	{
		pChildToken->GetEquationOperand(pToken->m_pEquation, Result);
		Result = _T("-1.0 / sqrt(1.0 - ") + Result + _T(" * ") + Result + _T(")");
		bRes = true;
	}

	return bRes;
}

bool CTransformSin::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_SIN);
	_ASSERTE(pToken->ChildrenCount() == 1);

	if (pToken->GetChild() == pChildToken)
	{
		pChildToken->GetEquationOperand(pToken->m_pEquation, Result);
		Result = _T("cos(") + Result + _T(")");
		bRes = true;
	}

	return bRes;
}

bool CTransformCos::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_COS);
	_ASSERTE(pToken->ChildrenCount() == 1);

	if (pToken->GetChild() == pChildToken)
	{
		pChildToken->GetEquationOperand(pToken->m_pEquation, Result);
		Result = _T("-sin(") + Result + _T(")");
		bRes = true;
	}

	return bRes;
}
