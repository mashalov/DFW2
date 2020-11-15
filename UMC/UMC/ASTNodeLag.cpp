#include "ASTNodes.h"

void CASTfnLag::FoldConstants()
{
    auto pDb = GetConstantArgument(1);
    if (pDb != nullptr && IsNumeric(pDb))
    {
        double dDb = NumericValue(pDb);
        // параметр должен быть неотрицательным
        if (dDb < 0.0)
            Error(fmt::format(szErrorParamterMustBeNonNegative, GetText(), dDb), true);
    }

    // если параметр - константа - то выход равен входу
    if (ChildNodes().front()->IsConst())
        pParent->ReplaceChild(this, ExtractChild(ChildNodes().begin()));
}