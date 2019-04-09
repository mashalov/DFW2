#include "..\stdafx.h"
#include "ExpressionParser.h"

bool CTransformRelayDelay::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformRelayDelay::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_RELAY || pToken->GetType() == ETT_RELAYD || pToken->GetType() == ETT_RELAYAL);

	bool bRes = true;

	TOKENITR it = pToken->ChildrenBegin();

	if (pToken->ChildrenCount() <= 3 && pToken->ChildrenCount() >= 2)
	{
		CExpressionToken *pCoe = NULL;
		CExpressionToken *pTol = NULL;
		CExpressionToken *pVal = NULL;
	}
	else
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}

	return bRes;
}

bool CTransformExpand::Simplify(CExpressionToken *pToken)
{
	_ASSERTE(pToken->GetType() == ETT_EXPAND);

	bool bRes = true;

	CExpressionToken *pInput = NULL;
	CExpressionToken *pTime = NULL;
	if (pToken->GetChildren(pTime, pInput))
	{
		if (pInput->IsNumeric() && pInput->GetNumericValue() <= 0.0)
			pToken->SetNumericConstant(0.0);
	}
	else
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}

	return bRes;
}

bool CTransformExpand::Expand(CExpressionToken *pToken)
{
	return Simplify(pToken);
}

bool CTransformShrink::Simplify(CExpressionToken *pToken)
{
	_ASSERTE(pToken->GetType() == ETT_SHRINK);

	bool bRes = true;

	CExpressionToken *pInput = NULL;
	CExpressionToken *pTime = NULL;
	if (pToken->GetChildren(pTime, pInput))
	{
		if (pInput->IsNumeric() && pInput->GetNumericValue() <= 0.0)
			pToken->SetNumericConstant(0.0);
	}
	else
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}

	return bRes;
}

bool CTransformShrink::Expand(CExpressionToken *pToken)
{
	return Simplify(pToken);
}

bool CTransformLimit::Expand(CExpressionToken *pToken)
{
	return Simplify(pToken);
}

bool CTransformLimit::Simplify(CExpressionToken *pToken)
{
	_ASSERTE(pToken->GetType() == ETT_LIMIT);

	bool bRes = true;

	CExpressionToken *pMax = NULL;
	CExpressionToken *pMin = NULL;
	CExpressionToken *pVal = NULL;

	if (pToken->ChildrenCount() != 3)
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}
	else
	{
		TOKENITR it = pToken->ChildrenBegin();

		pMax = *it;
		it++;
		pMin = *it;
		it++;
		pVal = *it;

		if (pMax->IsNumeric() && pMin->IsNumeric())
		{
			double dMax = pMax->GetNumericValue();
			double dMin = pMin->GetNumericValue();
			
			if (dMax >= dMin)
			{
				if (pVal->IsNumeric())
				{
					double dVal = pVal->GetNumericValue();
					if (dVal > dMax) dVal = dMax;
					else
					if (dVal < dMin) dVal = dMin;
					pToken->SetNumericConstant(dVal);
				}
			}
			else
			{
				bRes = false;
				pToken->SetError(ETT_ERROR_WRONGRANGE);
			}
		}
	}
	return bRes;
}


bool CTransformHigher::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_HIGHER);

	bool bRes = true;

	CExpressionToken *pInput1	= NULL;
	CExpressionToken *pInput2	= NULL;

	if (pToken->ChildrenCount() != 2)
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}
	else
	{
		TOKENITR it = pToken->ChildrenBegin();

		pInput1 = *it;
		it++;
		pInput2 = *it;

		if (pInput1->IsNumeric() && pInput2->IsNumeric())
		{
			double dInput1 = pInput1->GetNumericValue();
			double dInput2 = pInput2->GetNumericValue();

			if (dInput1 > dInput2)
				pToken->SetNumericConstant(1.0);
			else
				pToken->SetNumericConstant(0.0);
		}
	}
	return bRes;
}

bool CTransformHigher::Expand(CExpressionToken* pToken)
{
	return CTransformHigher::Simplify(pToken);
}


bool CTransformLower::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_LOWER);

	bool bRes = true;

	CExpressionToken *pInput1 = NULL;
	CExpressionToken *pInput2 = NULL;

	if (pToken->ChildrenCount() != 2)
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}
	else
	{
		TOKENITR it = pToken->ChildrenBegin();

		pInput1 = *it;
		it++;
		pInput2 = *it;

		if (pInput1->IsNumeric() && pInput2->IsNumeric())
		{
			double dInput1 = pInput1->GetNumericValue();
			double dInput2 = pInput2->GetNumericValue();

			if (dInput1 < dInput2)
				pToken->SetNumericConstant(1.0);
			else
				pToken->SetNumericConstant(0.0);
		}
	}
	return bRes;
}

bool CTransformLower::Expand(CExpressionToken* pToken)
{
	return CTransformLower::Simplify(pToken);
}


bool CTransformDeadband::Expand(CExpressionToken *pToken)
{
	return Simplify(pToken);
}

bool CTransformBit::Simplify(CExpressionToken *pToken)
{
	_ASSERTE(pToken->GetType() == ETT_BIT);

	bool bRes = true;

	if (pToken->ChildrenCount() != 1)
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}
	else
	{
		CExpressionToken *pInput = pToken->GetChild();

		if (pInput->IsNumeric())
		{
			if (pInput->GetNumericValue() > 0)
				pToken->SetNumericConstant(1.0);
			else
				pToken->SetNumericConstant(0.0);
		}
	}
	return bRes;
}

bool CTransformBit::Expand(CExpressionToken *pToken)
{
	return Simplify(pToken);
}

bool CTransformDeadband::Simplify(CExpressionToken *pToken)
{
	_ASSERTE(pToken->GetType() == ETT_DEADBAND);

	bool bRes = true;

	CExpressionToken *pVal = NULL;
	CExpressionToken *pDB = NULL;

	if (pToken->GetChildren(pDB, pVal))
	{
		if (pDB->IsNumeric() && pVal->IsNumeric())
		{
			double out = 0;

			double dVal = pVal->GetNumericValue();
			double dDB = pDB->GetNumericValue();

			if (dVal > dDB)
				out = dVal - dDB;
			else 
				if (dVal < -dDB)
					out = dVal + dDB;

			pToken->SetNumericConstant(out);
		}
	}
	else
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}

	return bRes;
}


bool CTransformAnd::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_AND);

	bool bRes = true;

	CExpressionToken *pInput1 = NULL;
	CExpressionToken *pInput2 = NULL;

	if (pToken->ChildrenCount() != 2)
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}
	else
	{
		TOKENITR it = pToken->ChildrenBegin();

		pInput1 = *it;
		it++;
		pInput2 = *it;

		if ( (pInput1->IsNumeric() && pInput1->GetNumericValue() <= 0) ||
			 (pInput2->IsNumeric() && pInput2->GetNumericValue() <= 0) )
		{
				pToken->SetNumericConstant(0.0);
		}
		else
			if ((pInput1->IsNumeric() && pInput1->GetNumericValue() > 0) &&
				(pInput2->IsNumeric() && pInput2->GetNumericValue() > 0) )
			{
				pToken->SetNumericConstant(1.0);
			}
	}
	return bRes;
}

bool CTransformOr::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_OR);

	bool bRes = true;

	CExpressionToken *pInput1 = NULL;
	CExpressionToken *pInput2 = NULL;

	if (pToken->ChildrenCount() != 2)
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}
	else
	{
		TOKENITR it = pToken->ChildrenBegin();

		pInput1 = *it;
		it++;
		pInput2 = *it;

		if ((pInput1->IsNumeric() && pInput1->GetNumericValue() > 0) ||
			(pInput2->IsNumeric() && pInput2->GetNumericValue() > 0) )
		{
			pToken->SetNumericConstant(1.0);
		}
		else
			if ((pInput1->IsNumeric() && pInput1->GetNumericValue() <= 0) &&
				(pInput2->IsNumeric() && pInput2->GetNumericValue() <= 0) )
			{
				pToken->SetNumericConstant(0.0);
			}
	}
	return bRes;
}

bool CTransformNot::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_NOT);

	bool bRes = true;

	if (pToken->ChildrenCount() != 1)
	{
		bRes = false;
		pToken->SetError(ETT_ERROR_WRONGNUMBEROFARGS);
	}
	else
	{
		CExpressionToken *pInput = pToken->GetChild();

		if (pInput->IsNumeric())
		{
			if (pInput->GetNumericValue() > 0)
				pToken->SetNumericConstant(0.0);
			else
				pToken->SetNumericConstant(1.0);
		}
	}
	return bRes;
}

bool CTransformAnd::Expand(CExpressionToken* pToken)
{
	return CTransformAnd::Simplify(pToken);
}

bool CTransformOr::Expand(CExpressionToken* pToken)
{
	return CTransformOr::Simplify(pToken);
}

bool CTransformNot::Expand(CExpressionToken* pToken)
{
	return CTransformNot::Simplify(pToken);
}