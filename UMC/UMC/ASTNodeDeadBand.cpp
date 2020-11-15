#include "ASTNodes.h"

void CASTfnDeadBand::FoldConstants()
{
    auto pDb = GetConstantArgument(1);
    if (pDb != nullptr && IsNumeric(pDb))
    {
        double dDb = NumericValue(pDb);
        // параметр должен быть неотрицательным
        if (dDb < 0.0)
            Error(fmt::format(szErrorParamterMustBeNonNegative, GetText(), dDb), true);
        else if (dDb == 0.0)
        {
            // если deadband ноль - заменяем его на вход
            pParent->ReplaceChild(this, ExtractChild(Children.begin()));
        }
        else
        {
            // если параметр положительный и вход - константа - вычисляем значение и заменяем 
            // deadband на константу
            double arg(0.0);
            if (SingleNumericChildValue(this, arg))
            {
                if (std::abs(arg) > dDb)
                    arg = (arg > 0.0) ? arg - dDb : arg + dDb;
                else
                    arg = 0.0;
                pParent->ReplaceChild<CASTNumeric>(this, arg);
            }
        }
    }
    else
    {
        // если численный параметр не определен, но вход нулевой - то заменяем узел на константу
        double arg(0.0);
        if (SingleNumericChildValue(this, arg) && arg == 0.0)
            pParent->ReplaceChild<CASTNumeric>(this, arg);
    }
}