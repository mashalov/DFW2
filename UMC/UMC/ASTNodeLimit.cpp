#include "ASTNodes.h"

void CASTfnLimit::FoldConstants()
{
    // проверяем ограничения
    auto pMin = GetConstantArgument(1);
    auto pMax = GetConstantArgument(2);
    // если они заданы численно
    if (pMin != nullptr && pMax != nullptr && IsNumeric(pMin) && IsNumeric(pMax))
    {
        // проверяем неравенство
        double dMin = NumericValue(pMin);
        double dMax = NumericValue(pMax);
        double diff = dMax - dMin;
        if (diff < 0.0)
            Error(fmt::format(szErrorLimitsWrong, GetText(), dMin, dMax), true);
        else if (diff == 0.0)
        {
            // если ограничения равны - заменяем ограничитель на константу, равную ограничению
            pParent->ReplaceChild<CASTNumeric>(this, dMin);
        }
        else
        {
            // если ограчичения ОК, проверяем вход. Если число - подгоняем под ограничения
            // и заменяем ограничитель на константу
            double arg(0.0);
            if (SingleNumericChildValue(this, arg))
            {
                if (arg > dMax) arg = dMax;
                if (arg < dMin) arg = dMin;
                pParent->ReplaceChild<CASTNumeric>(this, arg);
            }
        }
    }
}