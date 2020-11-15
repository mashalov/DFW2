#include "ASTNodes.h"
#include "ASTSimpleEquation.h"

void CASTfnAndOrBase::Flatten()
{
    CASTFlatOperator::FlattenImpl(this);
}

void CASTfnOr::FoldConstants()
{
    // если хотя бы один аргумент число и число больше нуля (истина), заменяем весь оператор на константу "1"
    if (std::find_if(Children.begin(), Children.end(), [](const CASTNodeBase* p) { return IsNumeric(p) && NumericValue(p) > 0; }) != Children.end())
        pParent->ReplaceChild<CASTNumeric>(this, 1.0);
}

void CASTfnAnd::FoldConstants()
{
    // если хотя бы один аргумент число и число меньше или равно нулю (ложь), заменяем весь оператор на константу "0"
    if (std::find_if(Children.begin(), Children.end(), [](const CASTNodeBase* p) { return IsNumeric(p) && NumericValue(p) <= 0; }) != Children.end())
        pParent->ReplaceChild<CASTNumeric>(this, 0.0);
}

CASTNodeBase* CASTfnAndOrBase::GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent)
{
    pDerivativeParent->CreateChild<CASTNumeric>(0.0);
    /*
    if (FindVariable(Children.front(), itVar->first))
    {
        CASTTreeBase::SimpleEquation::Term term;
        if (!term.Check(Children.front(), ASTNodeType::Variable))
            EXCEPTIONMSG("Wrong function argument");

        // если в аргументе унарный минус
        // вводим в производную унарный минус

        if (term.pUminus != nullptr)
            pDerivativeParent = pDerivativeParent->CreateChild<CASTUminus>();
        CASTfnCos* pCos = pDerivativeParent->CreateChild<CASTfnCos>();
        Children.front()->CloneTree(pCos);
    }
    else
    {
        // если переменная не является аргументом ИЛИ - производная равна нулю
        pDerivativeParent->CreateChild<CASTNumeric>(0.0);
    }
    */
    return nullptr;
}