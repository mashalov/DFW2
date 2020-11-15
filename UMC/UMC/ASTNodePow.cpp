#include "ASTNodes.h"

void CASTPow::Flatten()
{
	CASTNodeBase* pLeft = Children.front();
	if (pLeft->CheckType(ASTNodeType::Pow))
	{
		// основание вложенной степени станет основанием данной степени
		CASTNodeBase* pNewLeft = pLeft->ExtractChild(pLeft->ChildNodes().front());
		// показатель вложеноей степени
		CASTNodeBase* pNewMulRight1 = pLeft->ExtractChild(pLeft->ChildNodes().front());
		// показатель данной степени
		CASTNodeBase* pNewMulRight2 = ExtractChild(Children.back());
		// меняем вложенную степень на основание вложенной степени
		ReplaceChild(pLeft, pNewLeft);
		// создаем узел умножения с дочерьми показателями данной степени и вложенной
		CASTNodeBase* pNewMul = CreateChild<CASTMul>();
		pNewMul->CreateChild(pNewMulRight1);
		pNewMul->CreateChild(pNewMulRight2);
	}
}


CASTNodeBase* CASTPow::GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent)
{
	if (FindVariable(Children.front(), itVar->first))
	{
		// если в аргументе унарный минус
		// вводим в производную унарный минус

		if (!IsUminus(Children.front()) && !IsVariable(Children.front()))
			EXCEPTIONMSG("Wrong function argument");

		if (IsUminus(Children.front()))
			pDerivativeParent = pDerivativeParent->CreateChild<CASTUminus>();

		CASTMul* pMul = pDerivativeParent->CreateChild<CASTMul>(); // создаем умножение производной
		Children.back()->CloneTree(pMul);						   // в умножение входит исходная степень
		CASTPow* pPow = pMul->CreateChild<CASTPow>();			   // далее идет новая степень
		Children.front()->CloneTree(pPow);						   // аргумент степени равен аргументу исходной степени
		CASTSum* pSum = pPow->CreateChild<CASTSum>();			   // показатель степени - сумма
		pSum->CreateChild<CASTNumeric>(-1.0);					   // -1
		Children.back()->CloneTree(pSum);						   // и исходной степени
	}
	else
	{
		// если переменная не является аргументом степени - производная равна нулю
		pDerivativeParent->CreateChild<CASTNumeric>(0.0);
	}
	return nullptr;
}

CASTNodeBase* CASTfnExp::GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent)
{
	if (FindVariable(Children.front(), itVar->first))
	{
		// если в аргументе унарный минус
		// вводим в производную унарный минус

		if (!IsUminus(Children.front()) && !IsVariable(Children.front()))
			EXCEPTIONMSG("Wrong function argument");

		if (IsUminus(Children.front()))
			pDerivativeParent = pDerivativeParent->CreateChild<CASTUminus>();

		CASTfnExp* pExp = pDerivativeParent->CreateChild<CASTfnExp>(); // создаем степень производной
		Children.front()->CloneTree(pExp);
	}
	else
	{
		// если переменная не является аргументом степени - производная равна нулю
		pDerivativeParent->CreateChild<CASTNumeric>(0.0);
	}
	return nullptr;
}


void CASTfnSqrt::FoldConstants()
{
	// преобразуем sqrt в степень
	CASTNodeBase* pArg = ExtractChild(Children.begin());
	CASTPow *pPow = pParent->ReplaceChild<CASTPow>(this);
	pPow->CreateChild(pArg);
	pPow->CreateChild<CASTNumeric>(-2);
}
