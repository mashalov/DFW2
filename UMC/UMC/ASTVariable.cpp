#include "ASTNodes.h"

// производная по переменной зависит от контекста
CASTNodeBase* CASTVariable::GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent)
{
    CASTNodeBase* pDerivativeCtx(pDerivativeParent->GetDerivativeContext());
    bool byThisVariable(itVar->first == GetText());
    switch (pDerivativeCtx->GetType())
    {
    case ASTNodeType::Sum:  // для суммы производная по заданной переменной - 1, или 0, если это не заданная переменная
        return pDerivativeParent->CreateChild<CASTNumeric>(byThisVariable ? 1.0 : 0.0);
    case ASTNodeType::Mul: // для умножения если переменная найдена - 1, если нет - клон. Обнуление умножения 
        // в случае, если в него не входит заданная переменная выполняется производной умножения
        return byThisVariable ? pDerivativeParent->CreateChild<CASTNumeric>(1.0) : Clone(pDerivativeParent);
    case ASTNodeType::JacobiElement:
        return byThisVariable ? pDerivativeParent->CreateChild<CASTNumeric>(1.0) : Clone(pDerivativeParent);
    default:
        EXCEPTIONMSG("Unhandled derivative context");
    }
}