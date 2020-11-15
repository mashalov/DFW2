#include "ASTNodes.h"
#include "ASTSimpleEquation.h"

CASTNodeBase* CASTfnSin::GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent)
{
    if (FindVariable(Children.front(), itVar->first))
    {
        CASTTreeBase::SimpleEquation::Term term;
        if (!term.Check(Children.front(), ASTNodeType::Variable))
            EXCEPTIONMSG("Wrong function argument");

        // если в аргументе унарный минус
        // вводим в производную унарный минус

        if(term.pUminus != nullptr)
            pDerivativeParent = pDerivativeParent->CreateChild<CASTUminus>();
        CASTfnCos* pCos = pDerivativeParent->CreateChild<CASTfnCos>();
        Children.front()->CloneTree(pCos);
    }
    else
    {
        // если переменная не является аргументом синуса - производная равна нулю
        pDerivativeParent->CreateChild<CASTNumeric>(0.0);
    }
    return nullptr;
}

CASTNodeBase* CASTfnCos::GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent)
{
    if (FindVariable(Children.front(), itVar->first))
    {
        CASTTreeBase::SimpleEquation::Term term;
        if (!term.Check(Children.front(), ASTNodeType::Variable))
            EXCEPTIONMSG("Wrong function argument");

        // если в аргументе унарный минус
        // вводим в производную унарный минус

        if (term.pUminus != nullptr)
            pDerivativeParent = pDerivativeParent->CreateChild<CASTUminus>();

        CASTfnSin* pSin = pDerivativeParent->CreateChild<CASTUminus>()->CreateChild<CASTfnSin>();
        Children.front()->CloneTree(pSin);
    }
    else
    {
        // если переменная не является аргументом косинуса - производная равна нулю
        pDerivativeParent->CreateChild<CASTNumeric>(0.0);
    }
    return nullptr;
}