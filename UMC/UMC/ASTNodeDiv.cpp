#include "ASTNodes.h"

void CASTDiv::FoldConstants()
{
    // преобразуем разность в сумму с унарным минусом
    CASTNodeBase* pLeft = ExtractChild(Children.begin());
    CASTNodeBase* pRight = ExtractChild(std::prev(Children.end()));
    CASTMul* pNewMul = pParent->ReplaceChild<CASTMul>(this);
    pNewMul->CreateChild(pLeft);
    CASTPow* pNewPow = pNewMul->CreateChild<CASTPow>();
    pNewPow->CreateChild(pRight);
    pNewPow->CreateChild<CASTNumeric>(-1.0);
}
