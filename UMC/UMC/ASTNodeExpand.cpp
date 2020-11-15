#include "ASTNodes.h"

void CASTfnExpand::FoldConstants()
{
    CASTfnExpand::FoldConstantsExpandShrink(this);
}

void CASTfnExpand::FoldConstantsExpandShrink(CASTFunctionBase* pNodeShrinkExpand)
{
    auto pParameter = pNodeShrinkExpand->GetConstantArgument(1);
    if (pParameter != nullptr && IsNumeric(pParameter))
    {
        double dParameter = NumericValue(pParameter);
        // параметр должен быть неотрицательным
        if (dParameter < 0.0)
            pNodeShrinkExpand->Error(fmt::format(szErrorParamterMustBeNonNegative, pNodeShrinkExpand->GetText(), dParameter), true);
        else if (dParameter == 0.0)
        {
            // если параметр ноль - то выход всегда ноль
            pNodeShrinkExpand->GetParent()->ReplaceChild<CASTNumeric>(pNodeShrinkExpand, 0.0);
        }
    }

    // если вход - ноль - то выход всегда ноль
    double arg(0.0);
    if (SingleNumericChildValue(pNodeShrinkExpand, arg) && arg == 0.0)
        pNodeShrinkExpand->GetParent()->ReplaceChild<CASTNumeric>(pNodeShrinkExpand, arg);
}