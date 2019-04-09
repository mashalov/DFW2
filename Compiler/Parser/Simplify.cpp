#include "..\stdafx.h"
#include "ExpressionParser.h"

void RemoveChildren(CExpressionParser* pParser, CExpressionToken *pToken)
{
	PARSERSTACK ParserStack;
	TOKENITR it = pToken->ChildrenBegin();

	while (it != pToken->ChildrenEnd())
	{
		ParserStack.push(*it);
		(*it)->Unparent(pToken);
		it = pToken->RemoveChild(*it);
	}

	while (!ParserStack.empty())
	{
		CExpressionToken *pToken = ParserStack.top();
		ParserStack.pop();
		it = pToken->ChildrenBegin();
		while (it != pToken->ChildrenEnd())
		{
			ParserStack.push(*it);
			(*it)->Unparent(pToken);
			it = pToken->RemoveChild(*it);
		}
		pParser->DeleteToken(pToken);
	}
}

bool SimplifyPlus(CExpressionParser* pParser, CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_PLUS);
	bool bChanged = false;

	TOKENITR it = pToken->ChildrenBegin();
	CExpressionToken *pFirstConstant = NULL;
	double sum = 0.0;
	while (it != pToken->ChildrenEnd())
	{
		CExpressionToken *pSumToken = *it;
		if (pSumToken->IsNumeric())
		{
			sum += pSumToken->GetNumericValue();
			if (pFirstConstant)
			{
				it = pToken->RemoveChild(pSumToken);
				pParser->DeleteToken(pSumToken);
				bChanged = true;
			}
			else
			{
				pFirstConstant = pSumToken;
				it++;
			}
		}
		else if (pSumToken->GetType() == ETT_PLUS)
		{
			pSumToken->Eliminate();
			pParser->DeleteToken(pSumToken);
			bChanged = true;
			break;
		}
		else
			it++;
	}

	if (pFirstConstant)
	{
		if (sum != 0.0)
			pFirstConstant->SetNumericConstant(sum);
		else
		{
			pToken->RemoveChild(pFirstConstant);
			pParser->DeleteToken(pFirstConstant);
			if (!pToken->ChildrenCount())
				pToken->SetNumericConstant(sum);

			bChanged = true;
		}
	}

	if (pToken->ChildrenCount() == 1)
	{
		pToken->Eliminate();
		pParser->DeleteToken(pToken);
		bChanged = true;
	}

	return bChanged;
}

bool SimplifyMinus(CExpressionParser* pParser, CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_MINUS);
	_ASSERTE(pToken->ChildrenCount() == 2);

	CExpressionToken *pOp1, *pOp2;

	if (pToken->GetChildren(pOp1, pOp2))
	{
		pParser->ChangeTokenType(pToken, ETT_PLUS);
		CExpressionToken *pMul = pParser->NewExpressionToken(ETT_MUL);
		CExpressionToken *pMinus = pParser->NewExpressionToken(ETT_NUMERIC_CONSTANT);
		pMinus->SetNumericConstant(-1.0);
		pToken->ReplaceChild(pOp1, pMul);
		pMul->AddChild(pOp1);
		pMul->AddChild(pMinus);
	}

	return true;
}


bool SimplifyMul(CExpressionParser* pParser, CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_MUL);

	bool bChanged = false;
	
	TOKENITR it = pToken->ChildrenBegin();
	double mul = 1.0;
	CExpressionToken *pFirstConstant = NULL;
	while (it != pToken->ChildrenEnd())
	{
		CExpressionToken *pMulToken = *it;
		if (pMulToken->IsNumeric())
		{
			mul *= pMulToken->GetNumericValue();
			if (pFirstConstant)
			{
				it = pToken->RemoveChild(it);
				pParser->DeleteToken(pMulToken);
				bChanged = true;
			}
			else
			{
				pFirstConstant = pMulToken;
				it++;
			}
		} 
		else if (pMulToken->GetType() == ETT_MUL)
		{
			pMulToken->Eliminate();
			pParser->DeleteToken(pMulToken);
			bChanged = true;
			break;
		}
		else
			it++;
	}

	if (pFirstConstant)
	{
		if (mul == 1.0)
		{
			pToken->RemoveChild(pFirstConstant);
			pParser->DeleteToken(pToken);
			bChanged = true;
		}
		else if (mul == 0.0)
		{
			RemoveChildren(pParser, pToken);
			pToken->SetNumericConstant(mul);
			bChanged = true;
		}
		else
			pFirstConstant->SetNumericConstant(mul);
	}

	if (pToken->ChildrenCount() == 1)
	{
		pToken->Eliminate();
		pParser->DeleteToken(pToken);
		bChanged = true;
	}

	if (!bChanged)
	{
		TOKENITR it = pToken->ChildrenBegin();
		while (it != pToken->ChildrenEnd())
		{
			it++;
		}
	}

	return bChanged;
}


bool SimplifySin(CExpressionParser* pParser, CExpressionToken* pToken)
{
	bool bChanged = false;
	_ASSERTE(pToken->GetType() == ETT_SIN);
	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		if (pArg->IsNumeric())
		{
			pToken->SetNumericConstant(sin(pArg->GetNumericValue()));
			RemoveChildren(pParser, pToken);
			bChanged = true;
		}
	}

	return bChanged;
}

bool SimplifyCos(CExpressionParser* pParser, CExpressionToken* pToken)
{
	bool bChanged = false;
	_ASSERTE(pToken->GetType() == ETT_COS);
	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		if (pArg->IsNumeric())
		{
			pToken->SetNumericConstant(cos(pArg->GetNumericValue()));
			RemoveChildren(pParser, pToken);
			bChanged = true;
		}
	}

	return bChanged;
}

bool SimplifyUminus(CExpressionParser* pParser, CExpressionToken* pToken)
{
	bool bChanged = false;
	_ASSERTE(pToken->GetType() == ETT_UMINUS);
	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		pParser->ChangeTokenType(pToken, ETT_MUL);
		CExpressionToken *pMinus = pParser->NewExpressionToken(ETT_NUMERIC_CONSTANT);
		pMinus->SetNumericConstant(-1.0);
		pToken->AddChild(pMinus);
	}
	return true;
}

bool SimplifyPow(CExpressionParser* pParser, CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_POW);
	_ASSERTE(pToken->ChildrenCount() == 2);

	CExpressionToken *pOp1, *pOp2;

	bool bChanged = false;
	if (pToken->GetChildren(pOp1, pOp2))
	{
		if (pOp1->IsNumeric())
		{
			if (pOp2->IsNumeric())
			{
				pToken->SetNumericConstant(pow(pOp2->GetNumericValue(),pOp1->GetNumericValue()));
				RemoveChildren(pParser, pToken);
				bChanged = true;
			}
		}
	}

	return bChanged;
}

bool SimplifyDiv(CExpressionParser* pParser, CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_DIV);
	_ASSERTE(pToken->ChildrenCount() == 2);

	CExpressionToken *pOp1, *pOp2;

	bool bChanged = false;
	if (pToken->GetChildren(pOp1, pOp2))
	{
		if (pOp1->IsNumeric())
		{
			if (pOp2->IsNumeric())
			{
				pToken->SetNumericConstant(pOp2->GetNumericValue() / pOp1->GetNumericValue());
				RemoveChildren(pParser, pToken);
				bChanged = true;
			}
		}
	}

	return bChanged;
}


