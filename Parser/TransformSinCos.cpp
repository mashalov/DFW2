#include "stdafx.h"
#include "ExpressionTransform.h"


bool CTransformSin::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformCos::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}


bool CTransformSin::Simplify(CExpressionToken* pToken)
{
	bool bRes = false;

	if (pToken->GetType() != ETT_SIN)
		return bRes;

	bRes = true;

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
	bool bRes = false;
	if (pToken->GetType() != ETT_COS)
		return bRes;

	bRes = true;

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