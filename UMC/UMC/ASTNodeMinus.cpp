#include "ASTNodes.h"

void CASTMinus::Flatten()
{
    // преобразуем разность в сумму с унарным минусом
    CASTNodeBase* pLeft  = ExtractChild(Children.begin());
    CASTNodeBase* pRight = ExtractChild(Children.back());
    CASTSum* pNewSum = pParent->ReplaceChild<CASTSum>(this);
    pNewSum->CreateChild(pLeft);
    pNewSum->CreateChild<CASTUminus>()->CreateChild(pRight);
}
