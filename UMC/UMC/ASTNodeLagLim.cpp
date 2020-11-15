#include "ASTNodes.h"

void CASTfnLagLim::FoldConstants()
{
    
    // проверяем постоянную времени
    auto pT = GetConstantArgument(1);
    if(pT != nullptr && IsNumeric(pT))
    {
        double dT = NumericValue(pT);
        // параметр должен быть неотрицательным
        if (dT < 0.0)
            Error(fmt::format(szErrorParamterMustBeNonNegative, GetText(), dT), true);
    }

    // если вход - константа - заменяем блок на ограничитель
    if (ChildNodes().front()->IsConst())
    {
        // создаем новый limit
        CASTfnLimit *pLimit = pTree->CreateNode<CASTfnLimit>(nullptr, pTree->GetHostBlocks().size());
        // переносим дочерние узлы из данного блока в limit
        ASTNodeList children;
        ExtractChildren(children);
        pLimit->AppendChildren(children);
        // переносим ограничения из блока в limit
        // в блоке ограничения могут быть в некоем диапазоне параметров
        // поэтому выносим эти два параметра из диапазона константных аргументов
        // а остальные оставляем в блоке. Они будут удалены при удалении блока

        // вообще имеет смысл сделать базовый класс для примитива с ограничениями и
        // преобразовывать ограниченные примитивы в ограничитель единой функцией

        // а пока просто перебираем константные аргументы
        auto itConstArg = ConstantArguments.begin();
        // номера константных аргументов должны иметь номера +1 к входному. Для
        // ограничителя это 1
        size_t ArgNumber(pLimit->ChildNodes().size());
        // указываем диапазон, в котором в данном блоке находятся 
        // константные аргументы min/max
        auto range = std::make_pair(size_t(2),size_t(3));

        while (itConstArg != ConstantArguments.end())
        {
            // если номер аргумента данного блока находится в заданном диапазоне
            if (itConstArg->second >= range.first && itConstArg->second <= range.second)
            {
                // добавляем константный аргумент в новый ограничитель
                pLimit->ConstantArguments.push_back(std::make_pair(itConstArg->first, ArgNumber));
                ArgNumber++;
                // удаляем аргумент из константных аргументов данного блока
                itConstArg = ConstantArguments.erase(itConstArg);
            }
            else
                itConstArg++; // если номер аргумента вне диапазаона - пропускаем
        }

        // заменяем данный блок на limit в родителе
        pParent->ReplaceChild(this, pLimit);
    }
    else
    {
        // проверяем ограничения
        auto pMin = GetConstantArgument(2);
        auto pMax = GetConstantArgument(3);
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
                // если ограничения равны - заменяем блок на константу, равную ограничению
                pParent->ReplaceChild<CASTNumeric>(this, dMin);
            }
        }
    }
}