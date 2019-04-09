#include "..\stdafx.h"
#include "ExpressionParser.h"

bool CTransformAbs::Simplify(class CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_ABS);

	bool bRes = true;

	CExpressionToken *pArg = pToken->GetChild();

	if (pArg)
	{
		if (pArg->IsNumeric())
		{
			pToken->SetNumericConstant(fabs(pArg->GetNumericValue()));
		}
	}

	return bRes;
}

bool CTransformAbs::Expand(class CExpressionToken* pToken)
{
	return Simplify(pToken);
}
