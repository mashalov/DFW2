#include "..\stdafx.h"
#include "ExpressionParser.h"

bool CTransformPlus::Expand(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_PLUS);

	bool bRes = true;

	const CExpressionParser *pParser = pToken->GetParser();

	bRes = bRes && SimplifyPlusConstants(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifySum(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyPlusDiminished(pToken);

	return bRes;
}

bool CTransformMinus::Expand(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_MINUS);

	bool bRes = true;

	CExpressionToken *pOp1, *pOp2;

	if (pToken->GetChildren(pOp1, pOp2))
	{
		pToken->ChangeType(ETT_PLUS);
		CExpressionToken *pMul = pToken->NewExpressionToken(ETT_MUL);
		CExpressionToken *pMinus = pToken->NewExpressionToken(ETT_NUMERIC_CONSTANT);
		pMinus->SetNumericConstant(-1.0);
		pToken->ReplaceChild(pOp1, pMul,0);
		pMul->AddChild(pOp1);
		pMul->AddChild(pMinus);
	}

	return bRes;
}


bool CTransformUminus::Expand(CExpressionToken* pToken)
{
	return Simplify(pToken);
}

bool CTransformUminus::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_UMINUS);
	bool bRes = true;

	CExpressionToken *pArg = pToken->GetChild();
	if (pArg)
	{
		pToken->ChangeType(ETT_MUL);
		CExpressionToken *pMinus = pToken->NewNumericConstant(-1.0);
		pToken->AddChild(pMinus);
	}
	return bRes;
}

bool CTransformMinus::Simplify(CExpressionToken* pToken)
{
	return Expand(pToken);
}

bool CTransformMinus::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_MINUS);
	_ASSERTE(pToken->ChildrenCount() == 2);

	CExpressionToken *pFirst = NULL, *pSecond = NULL;

	if (pToken->GetChildren(pSecond, pFirst))
	{
		if (pSecond == pChildToken)
		{
			Result = _T("-1");
			bRes = true;
		}
		else if (pFirst == pChildToken)
		{
			Result = _T("1");
			bRes = true;
		}
	}

	return bRes;
}

bool CTransformPlus::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_PLUS);

	bool bRes = true;

	const CExpressionParser *pParser = pToken->GetParser();

	bRes = bRes && SimplifyPlusConstants(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyPlusDiminished(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifySumCombineBraces(pToken);

	return bRes;
}


bool CTransformPlus::SimplifySum(CExpressionToken* pSum)
{
	bool bRes = true;

	TOKENITR it1 = pSum->ChildrenBegin();
	TOKENLIST MatchList1, MatchList2, DiffList1, DiffList2;

	while (it1 != pSum->ChildrenEnd())
	{
		TOKENITR it2 = it1;
		it2++;
		if (it2 == pSum->ChildrenEnd())
			break;

		while (it2 != pSum->ChildrenEnd())
		{
			_ASSERTE(it1 != it2);
			CompareTokens(*it1, *it2, MatchList1, MatchList2, DiffList1, DiffList2);
			if (MatchList1.size() > 0)
			{
				double coe1 = 1.0, coe2 = 1.0;
				if (ConstantCoefficient(DiffList1, coe1) && ConstantCoefficient(DiffList2, coe2))
				{
					CExpressionToken *pToRemove = *it1;
					pSum->RemoveChild(pToRemove);
					pToRemove->Delete();
					if (DiffList2.size() == 1)
						DiffList2.front()->SetNumericConstant(coe1 + coe2);
					else
					{
						CExpressionToken *pCoe = pSum->NewNumericConstant(coe1 + coe2);
						if ((*it2)->GetType() == ETT_MUL)
							(*it2)->AddChild(pCoe);
						else
						{
							CExpressionToken *pToMul = *it2;
							CExpressionToken *pMul = pSum->NewExpressionToken(ETT_MUL);
							pMul->AddChild(pCoe);
							pSum->ReplaceChild(pToMul, pMul);
							pMul->AddChild(pToMul);
						}
					}
				}
			}

			if (pSum->ParserChanged())
				break;

			it2++;
		}

		if (pSum->ParserChanged())
			break;

		it1++;
	}
	return bRes;
}

bool CTransformPlus::SimplifyPlusConstants(CExpressionToken *pToken)
{
	bool bRes = true;

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
				pSumToken->Delete();
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
			pSumToken->Delete();
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
			pFirstConstant->Delete();
			if (!pToken->ChildrenCount())
				pToken->SetNumericConstant(sum);
		}
	}

	return bRes;
}

bool CTransformPlus::SimplifyPlusDiminished(CExpressionToken *pToken)
{
	bool bRes = true;

	if (pToken->ChildrenCount() == 1)
	{
		pToken->Eliminate();
		pToken->Delete();
	}
	else if (pToken->ChildrenCount() == 0)
	{
		pToken->SetNumericConstant(0.0);
	}

	return bRes;
}


bool CTransformPlus::SimplifySumCombineBraces(CExpressionToken* pSum)
{
	bool bRes = true;

	TOKENITR it1 = pSum->ChildrenBegin();
	TOKENLIST MatchList1, MatchList2, DiffList1, DiffList2;

	while (it1 != pSum->ChildrenEnd())
	{
		TOKENITR it2 = it1;
		it2++;
		if (it2 == pSum->ChildrenEnd())
			break;

		while (it2 != pSum->ChildrenEnd())
		{
			_ASSERTE(it1 != it2);

			CExpressionToken *pMul1 = *it1;
			CExpressionToken *pMul2 = *it2;

			CompareTokens(pMul1, pMul2, MatchList1, MatchList2, DiffList1, DiffList2);

			if (MatchList1.size() > 0)
			{
				CExpressionToken *pNewMul = pSum->NewExpressionToken(ETT_MUL);
				CExpressionToken* pNewSum = pSum->NewExpressionToken(ETT_PLUS);

				CExpressionToken *pNewMul1 = pSum->NewExpressionToken(ETT_MUL);
				CExpressionToken *pNewMul2 = pSum->NewExpressionToken(ETT_MUL);
				
				pSum->RemoveChild(pMul1);
				pSum->RemoveChild(pMul2);

				pNewSum->AddChild(pNewMul1);
				pNewSum->AddChild(pNewMul2);

				pNewMul->AddChild(pNewSum);
				pSum->AddChild(pNewMul);

				for (TOKENITR it = MatchList1.begin(); it != MatchList1.end(); it++)
				{
					if (*it != pMul1)
						pMul1->RemoveChild(*it);

					pNewMul->AddChild(*it);
				}

				for (TOKENITR it = MatchList2.begin(); it != MatchList2.end(); it++)
				{
					if (*it != pMul2)
					{
						pMul2->RemoveChild(*it);
						(*it)->Delete();
					}
				}

				for (TOKENITR it = DiffList1.begin(); it != DiffList1.end(); it++)
				{
					if (*it != pMul1)
						pMul1->RemoveChild(*it);

					pNewMul1->AddChild(*it);
				}

				if (!pNewMul1->ChildrenCount())
					pNewMul1->AddChild(pSum->NewNumericConstant(1.0));

				for (TOKENITR it = DiffList2.begin(); it != DiffList2.end(); it++)
				{
					if (*it != pMul2)
						pMul2->RemoveChild(*it);

					pNewMul2->AddChild(*it);
				}

				if (!pNewMul2->ChildrenCount())
					pNewMul2->AddChild(pSum->NewNumericConstant(1.0));

				if(pMul1->GetType() == ETT_MUL) pMul1->Delete();
				pMul2->Delete();

			}

			if (pSum->ParserChanged())
				break;

			it2++;
		}

		if (pSum->ParserChanged())
			break;

		it1++;
	}

#ifdef _DEBUG
	pSum->GetParser()->CheckLinks();
#endif

	return bRes;
}

bool CTransformPlus::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_PLUS);

	for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
		if (pChildToken == *it)
		{
			Result = _T("1");
			bRes = true;
			break;
		}

	return bRes;
}
