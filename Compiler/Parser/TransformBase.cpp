#include "..\stdafx.h"
#include "ExpressionParser.h"


void CExpressionTransform::CompareTokens(CExpressionToken* pToken1, CExpressionToken*pToken2, TOKENLIST& MatchList1, 
																							  TOKENLIST& MatchList2,
																							  TOKENLIST& DiffList1,
																							  TOKENLIST& DiffList2)
{
	TOKENLIST list1, list2;
	MatchList1.clear();
	MatchList2.clear();
	if (pToken1->GetType() == ETT_MUL)
		list1.assign(pToken1->ChildrenBegin(), pToken1->ChildrenEnd());
	else
		list1.push_back(pToken1);

	if (pToken2->GetType() == ETT_MUL)
		list2.assign(pToken2->ChildrenBegin(), pToken2->ChildrenEnd());
	else
		list2.push_back(pToken2);

	TOKENITR it1 = list1.begin();
	while (it1 != list1.end())
	{
		CExpressionToken *pLE1 = *it1;
		CExpressionToken *pLE2(nullptr);
		bool bFound = false;
		for (TOKENITR it2 = list2.begin(); it2 != list2.end(); it2++)
		{
			pLE2 = *it2;
			if (!pLE1->Compare(pLE2))
			{
				list2.erase(it2);
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			it1 = list1.erase(it1);
			MatchList1.push_back(pLE1);
			MatchList2.push_back(pLE2);
		}
		else
			it1++;
	}
	DiffList1 = list1;
	DiffList2 = list2;
}

bool CExpressionTransform::ConstantCoefficient(TOKENLIST& DiffList, double& Coe)
{
	bool bConstCoe = false;
	Coe = 1.0;
	if (DiffList.empty())
		bConstCoe = true;
	if (DiffList.size() == 1 && DiffList.front()->IsNumeric())
	{
		bConstCoe = true;
		Coe = DiffList.front()->GetNumericValue();
	}
	return bConstCoe;
}


bool CExpressionRule::Match(CExpressionToken *pToken)
{
	PARSERSTACK Stack;
	ptrdiff_t nIdentical = 0;

	m_Vars.clear();

	Stack.push(m_pSource->m_ResultStack.top());
	Stack.push(pToken);

	while (!Stack.empty() && !nIdentical)
	{
		CExpressionToken *pTree = Stack.top();
		Stack.pop();
		CExpressionToken *pPatt = Stack.top();
		Stack.pop();

		nIdentical = pTree->GetType() - pPatt->GetType();

		switch (pPatt->GetType())
		{
		case ETT_VARIABLE:
		case ETT_MODELLINK:
		{
			VARIABLEITR vit = m_Vars.find(pPatt->GetTextValue());
			if (vit == m_Vars.end())
			{
				m_Vars.insert(std::make_pair(pPatt->GetTextValue(), VariableEnum(pTree)));
				nIdentical = 0;
			}
			else
			{
				nIdentical = pTree->Compare(vit->second.m_pToken);
			}
		}
		break;
		case ETT_NUMERIC_CONSTANT:
		{
			if (nIdentical == 0)
			{
				double diff = pPatt->GetNumericValue() - pTree->GetNumericValue();
				nIdentical = (diff > 0) ? 1 : (diff < 0) ? -1 : 0;
			}
		}
		break;
		default:
			if (nIdentical == 0)
			{
				nIdentical = pTree->ChildrenCount() - pPatt->ChildrenCount();
				if (nIdentical == 0)
				{
					CONSTTOKENITR itLeft = pTree->ChildrenBegin();
					CONSTTOKENITR itRight = pPatt->ChildrenBegin();
					while (itLeft != pTree->ChildrenEnd())
					{
						Stack.push(*itRight);
						Stack.push(*itLeft);
						itLeft++;
						itRight++;
					}
				}
			}
			break;
		}
	}

	return nIdentical == 0;
}

bool CExpressionRule::Apply(CExpressionToken *pToken)
{
	if (Match(pToken))
	{
		CExpressionParser *pParser = pToken->m_pParser;
		pParser->m_SubstituteVars = m_Vars;
		CExpressionToken *pSubstToken = pParser->CloneToken(m_pDestination->m_ResultStack.top());
		
		pToken->GetParent()->ReplaceChild(pToken, pSubstToken);

		for (TOKENITR it = pSubstToken->ChildrenBegin(); it != pSubstToken->ChildrenEnd(); it++)
		{
			CExpressionToken *pVar = *it;
			if (pVar->IsVariable())
			{
				VARIABLEITR vit = m_Vars.find(pVar->GetTextValue());
				pVar->SetTextValue(pVar->GetTextValue());
			}
		}
		pToken->Delete();
	}

	return true;
}


const _TCHAR* CTransformDerlag::m_cszBlockType = _T("PBT_DERLAG");
const _TCHAR* CTransformRelayDelay::m_cszBlockType = _T("PBT_RELAYDELAY");
const _TCHAR* CTransformRelayDelayLogic::m_cszBlockType = _T("PBT_RELAYDELAYLOGIC");
const _TCHAR* CTransformLimit::m_cszBlockType = _T("PBT_LIMITERCONST");
const _TCHAR* CTransformHigher::m_cszBlockType = _T("PBT_HIGHER");
const _TCHAR* CTransformLower::m_cszBlockType = _T("PBT_LOWER");
const _TCHAR* CTransformBit::m_cszBlockType = _T("PBT_BIT");
const _TCHAR* CTransformAnd::m_cszBlockType = _T("PBT_AND");
const _TCHAR* CTransformOr::m_cszBlockType = _T("PBT_OR");
const _TCHAR* CTransformNot::m_cszBlockType = _T("PBT_NOT");
const _TCHAR* CTransformAbs::m_cszBlockType = _T("PBT_ABS");
const _TCHAR* CTransformExpand::m_cszBlockType = _T("PBT_EXPAND");
const _TCHAR* CTransformShrink::m_cszBlockType = _T("PBT_SHRINK");
const _TCHAR* CTransformDeadband::m_cszBlockType = _T("PBT_DEADBAND");
const _TCHAR* CTransformAnd::m_cszInitFunction = _T("and");
const _TCHAR* CTransformOr::m_cszInitFunction = _T("or");
const _TCHAR* CTransformNot::m_cszInitFunction = _T("not");
