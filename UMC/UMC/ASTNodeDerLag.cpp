#include "ASTNodes.h"

void CASTfnDerLag::FoldConstants()
{
    auto pDb = GetConstantArgument(1);
    if (pDb != nullptr && IsNumeric(pDb))
    {
        double dDb = NumericValue(pDb);
        // параметр должен быть неотрицательным
        if (dDb < 0.0)
            Error(fmt::format(szErrorParamterMustBeNonNegative, GetText(), dDb), true);
    }

    // если параметр - константа - то выход - ноль
    if (ChildNodes().front()->IsConst())
        pParent->ReplaceChild<CASTNumeric>(this, 0.0);
}