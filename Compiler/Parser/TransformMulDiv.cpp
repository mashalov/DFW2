#include "..\stdafx.h"
#include "ExpressionParser.h"


bool CTransformMul::Expand(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_MUL);

	bool bRes = true;

	const CExpressionParser *pParser = pToken->GetParser();


	if (!pParser->GetChanged())
		bRes = bRes && SimplifyMulExpandBraces(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyMulConstant(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyMulDiminished(pToken);


	return bRes;

}

bool CTransformDiv::Expand(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_DIV);

	bool bRes = true;

	const CExpressionParser *pParser = pToken->GetParser();

	bRes = bRes && SimplifyDivConvertToPow(pToken);

	return bRes;
}

bool CTransformMul::Simplify(CExpressionToken* pToken)
{
	_ASSERTE(pToken->GetType() == ETT_MUL);

	bool bRes = true;

	const CExpressionParser *pParser = pToken->GetParser();

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyMulConstant(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyMulCombinePow(pToken);

	if (!pParser->GetChanged())
		bRes = bRes && SimplifyMulDiminished(pToken);

	return bRes;
}

bool CTransformDiv::Simplify(CExpressionToken* pToken)
{
	return Expand(pToken);
}

bool CTransformMul::SimplifyMulDiminished(CExpressionToken* pToken)
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

bool CTransformMul::SimplifyMulConstant(CExpressionToken* pToken)
{
	bool bRes = true;

	TOKENITR it = pToken->ChildrenBegin();
	double mul = 1.0;
	CExpressionToken *pFirstConstant(nullptr);

	while (it != pToken->ChildrenEnd())
	{
		CExpressionToken *pMulToken = *it;
		if (pMulToken->IsNumeric())
		{
			mul *= pMulToken->GetNumericValue();
			if (pFirstConstant)
			{
				it = pToken->RemoveChild(it);
				pMulToken->Delete();
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
			pMulToken->Delete();
			pFirstConstant = nullptr;;
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
			pFirstConstant->Delete();
			if (!pToken->ChildrenCount())
				pToken->SetNumericConstant(mul);
			else if (pToken->ChildrenCount() == 1)
			{
				pToken->Eliminate();
				pToken->Delete();
			}
		}
		else if (mul == 0.0)
		{
			pToken->RemoveChildren();
			pToken->SetNumericConstant(mul);
		}
		else
			pFirstConstant->SetNumericConstant(mul);
	}

	return bRes;
}

bool CTransformMul::SimplifyMulExpandBraces(CExpressionToken *pToken)
{
	bool bRes = true;

	TOKENITR it = pToken->ChildrenBegin();
	CExpressionToken *pSumToken(nullptr);
	for (it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
	{
		if ((*it)->GetType() == ETT_PLUS)
		{
			pSumToken = *it;
			break;
		}
	}

	if (pSumToken)
	{
		pToken->RemoveChild(pSumToken);
		pToken->GetParent()->ReplaceChild(pToken, pSumToken);

		for (it = pSumToken->ChildrenBegin(); it != pSumToken->ChildrenEnd(); it++)
		{
			CExpressionToken *pSumChild = *it;
			CExpressionToken *pMulClone = pToken->Clone();
			pSumToken->ReplaceChild(pSumChild, pMulClone);
			pMulClone->AddChild(pSumChild);
		}
		pToken->Delete();
	}

	return bRes;
}

bool CTransformDiv::SimplifyDivConvertToPow(CExpressionToken *pToken)
{
	CExpressionToken *pDenom, *pNom;

	bool bRes = pToken->GetChildren(pDenom, pNom);

	if (bRes)
	{
			
			CExpressionToken *pNewPow = pToken->NewExpressionToken(ETT_POWF);
			pToken->ReplaceChild(pDenom, pNewPow, 0);
			pNewPow->AddChild(pToken->NewNumericConstant(-1));
			pNewPow->AddChild(pDenom);
			pToken->ChangeType(ETT_MUL);
	}

	return bRes;
}


bool CTransformMul::SimplifyMulCombinePow(CExpressionToken *pToken)
{
	bool bRes = true;

	const CExpressionParser *pParser = pToken->GetParser();

	
	// expr * expr = expr ^ 2
	TOKENITR it1 = pToken->ChildrenBegin();
	while (it1 != pToken->ChildrenEnd())
	{
		CExpressionToken *pToken1 = *it1;
		TOKENITR it2 = pToken->ChildrenBegin();
		while (it2 != pToken->ChildrenEnd())
		{
			CExpressionToken *pToken2 = *it2;
			if (it1 != it2)
			{
				if (!pToken1->Compare(pToken2))
				{
					pToken->RemoveChild(pToken1);
					pToken->RemoveChild(pToken2);
					pToken1->Delete();
					CExpressionToken *pNewPow = pToken->NewExpressionToken(ETT_POWF);
					pToken->AddChild(pNewPow);
					pNewPow->AddChild(pToken->NewNumericConstant(2.0));
					pNewPow->AddChild(pToken2);
				}
				if (pParser->GetChanged())
					break;
			}
			it2++;
		}
		if (pParser->GetChanged())
			break;
		it1++;
	}

	// expr^x * expr^y = expr^(x+y)
	if (!pParser->GetChanged())
	{
		TOKENITR it1 = pToken->ChildrenBegin();
		while (it1 != pToken->ChildrenEnd())
		{
			CExpressionToken *pToken1 = *it1;
			if (pToken1->GetType() == ETT_POWF)
			{
				CExpressionToken *pTokenExp1(nullptr), *pTokenBase1(nullptr);
				CExpressionToken *pTokenExp2(nullptr), *pTokenBase2(nullptr);

				if (pToken1->GetChildren(pTokenExp1, pTokenBase1))
				{
					TOKENITR it2 = pToken->ChildrenBegin();
					while (it2 != pToken->ChildrenEnd())
					{
						CExpressionToken *pToken2 = *it2;
						if (it1 != it2)
						{
							if (pToken2->GetType() == ETT_POWF)
							{
								if (pToken2->GetChildren(pTokenExp2, pTokenBase2))
								{
									if (!pTokenBase2->Compare(pTokenBase1))
									{
										CExpressionToken *pNewPlus = pToken->NewExpressionToken(ETT_PLUS);
										pToken2->RemoveChild(pTokenExp2);
										pToken->RemoveChild(pToken2);
										pToken2->Delete();
										pToken1->ReplaceChild(pTokenExp1, pNewPlus, 0);
										pNewPlus->AddChild(pTokenExp1);
										pNewPlus->AddChild(pTokenExp2);
									}
								}
							}
							if (pParser->GetChanged())
								break;
						}
						it2++;
					}
				}
			}
			if (pParser->GetChanged())
				break;
			it1++;
		}
	}


	// x^expr * y^expr = (x*y)^expr
	if (!pParser->GetChanged())
	{
		TOKENITR it1 = pToken->ChildrenBegin();
		while (it1 != pToken->ChildrenEnd())
		{
			CExpressionToken *pToken1 = *it1;
			if (pToken1->GetType() == ETT_POWF)
			{
				CExpressionToken *pTokenExp1(nullptr), *pTokenBase1(nullptr);
				CExpressionToken *pTokenExp2(nullptr), *pTokenBase2(nullptr);

				if (pToken1->GetChildren(pTokenExp1, pTokenBase1))
				{
					TOKENITR it2 = pToken->ChildrenBegin();
					while (it2 != pToken->ChildrenEnd())
					{
						CExpressionToken *pToken2 = *it2;
						if (it1 != it2)
						{
							if (pToken2->GetType() == ETT_POWF)
							{
								if (pToken2->GetChildren(pTokenExp2, pTokenBase2))
								{
									if (!pTokenExp1->Compare(pTokenExp2))
									{
										pToken->RemoveChild(pToken2);
										pToken2->RemoveChild(pTokenBase2);
										pToken2->Delete();
										CExpressionToken *pNewMul = pToken->NewExpressionToken(ETT_MUL);
										pNewMul->AddChild(pTokenBase2);
										pToken1->ReplaceChild(pTokenBase1, pNewMul, 1);
										pNewMul->AddChild(pTokenBase1);
									}
								}
							}
							if (pParser->GetChanged())
								break;
						}
						it2++;
					}
				}
			}
			if (pParser->GetChanged())
				break;
			it1++;
		}
	}

	// expr3 * (expr1 * expr2) * (expr1 * expr2 * expr4)^x = expr3 * (expr1 * expr2)^(x+1) * (expr4)^x
	if (!pParser->GetChanged())
	{
		TOKENITR it1 = pToken->ChildrenBegin();
		while (it1 != pToken->ChildrenEnd())
		{
			CExpressionToken *pToken1 = *it1;
			if (pToken1->GetType() == ETT_POWF)
			{
				CExpressionToken *pTokenExp1(nullptr), *pTokenBase1(nullptr);
				if (pToken1->GetChildren(pTokenExp1, pTokenBase1))
				{
					TOKENLIST MatchList1, MatchList2, DiffList1, DiffList2;
					CompareTokens(pToken, pTokenBase1, MatchList1, MatchList2, DiffList1, DiffList2);
					if (MatchList1.size() > 0)
					{
						CExpressionToken *pNewPow = pToken->NewExpressionToken(ETT_POWF);
						CExpressionToken *pNewExp = pToken->NewExpressionToken(ETT_PLUS);
						CExpressionToken *pNewMul = pToken->NewExpressionToken(ETT_MUL);
						pNewPow->AddChild(pNewExp);
						pNewPow->AddChild(pNewMul);
						pNewExp->AddChild(pTokenExp1->Clone());
						pNewExp->AddChild(pToken->NewNumericConstant(1.0));


						for (TOKENITR it = MatchList1.begin(); it != MatchList1.end(); it++)
						{
							pToken->RemoveChild(*it);
							pNewMul->AddChild(*it);
						}

						for (TOKENITR it = MatchList2.begin(); it != MatchList2.end(); it++)
						{
							if (*it == pTokenBase1)
								pToken1->ReplaceChild(pTokenBase1, pToken1->NewNumericConstant(1.0),1);
							else
								pTokenBase1->RemoveChild(*it);
							(*it)->Delete();
						}

						if (!pTokenBase1->ChildrenCount())
						{
							pToken1->ReplaceChild(pTokenBase1, pToken1->NewNumericConstant(1.0), 1);
							pTokenBase1->Delete();
						}

						pToken->AddChild(pNewPow);
						break;
					}
				}
			}
			it1++;
		}
	}

	if (!pParser->GetChanged())
	{
		TOKENITR it1 = pToken->ChildrenBegin();
		while (it1 != pToken->ChildrenEnd())
		{
			CExpressionToken *pToken1 = *it1;
			if (pToken1->GetType() == ETT_POWF)
			{
				CExpressionToken *pTokenExp1(nullptr), *pTokenBase1(nullptr);
				CExpressionToken *pTokenExp2(nullptr), *pTokenBase2(nullptr);
				if (pToken1->GetChildren(pTokenExp1, pTokenBase1))
				{
					
					TOKENITR it2 = pToken->ChildrenBegin();
					while (it2 != pToken->ChildrenEnd())
					{
						CExpressionToken *pToken2 = *it2;
						if (it1 != it2 && pToken2->GetType() == ETT_POWF && pToken2->GetChildren(pTokenExp2, pTokenBase2))
						{
							TOKENLIST MatchList1, MatchList2, DiffList1, DiffList2;
							CompareTokens(pTokenBase1, pTokenBase2, MatchList1, MatchList2, DiffList1, DiffList2);

							if (MatchList1.size() > 0)
							{
								CExpressionToken *pNewPow = pToken->NewExpressionToken(ETT_POWF);
								CExpressionToken *pNewExp = pToken->NewExpressionToken(ETT_PLUS);
								CExpressionToken *pNewMul = pToken->NewExpressionToken(ETT_MUL);
								pNewPow->AddChild(pNewExp);
								pNewPow->AddChild(pNewMul);
								pNewExp->AddChild(pTokenExp1->Clone());
								pNewExp->AddChild(pTokenExp2->Clone());

								for (TOKENITR it = MatchList1.begin(); it != MatchList1.end(); it++)
								{
									if (*it == pTokenBase1)
										pToken1->ReplaceChild(pTokenBase1, pToken1->NewNumericConstant(1.0),1);
									else
										pTokenBase1->RemoveChild(*it);

									pNewMul->AddChild(*it);
								}

								if (!pTokenBase1->ChildrenCount())
								{
									pToken1->ReplaceChild(pTokenBase1, pToken->NewNumericConstant(1.0), 1);
									pTokenBase1->Delete();
								}

								for (TOKENITR it = MatchList2.begin(); it != MatchList2.end(); it++)
								{
									if (*it == pTokenBase2)
										pToken2->ReplaceChild(pTokenBase2, pToken1->NewNumericConstant(1.0), 1);
									else
										pTokenBase2->RemoveChild(*it);
									(*it)->Delete();
								}

								if (!pTokenBase2->ChildrenCount())
								{
									pToken2->ReplaceChild(pTokenBase2, pToken->NewNumericConstant(1.0), 1);
									pTokenBase2->Delete();
								}

								pToken->AddChild(pNewPow);
								break;
							}

						}
						it2++;
					}
				}
			}
			if (pParser->GetChanged())
				break;
			it1++;
		}
	}

	return bRes;
}

bool CTransformMul::Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result)
{
	bool bRes = false;

	_ASSERTE(pToken->GetType() == ETT_MUL);

	std::wstring Operand;
	Result.clear();
	for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
	{
		if (pChildToken == *it)
		{
			bRes = true;
		}
		else
		{
			(*it)->GetEquationOperand(pToken->m_pEquation, Operand);
			if (!Result.empty())
				Result += _T(" * ");
			Result += Operand;
		}
	}

	return bRes;
}

const _TCHAR *CTransformMul::cszMul = _T("*");