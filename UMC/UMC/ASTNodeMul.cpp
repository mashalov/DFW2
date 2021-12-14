#include "ASTNodes.h"

void CASTMul::FoldZeroMultiplier()
{
    // если в произведении есть "0", все произведение равно нулю
    auto zc = std::find_if(Children.begin(), Children.end(), [](const CASTNodeBase* p) { return IsNumericZero(p); });
    if (zc != Children.end())
        pParent->ReplaceChild<CASTNumeric>(this, 0.0);
}

void CASTMul::FoldUnaryMinuses()
{
    // если в произведении есть несколько унарных минусов
    // и константы с отрицательными знаками, собираем все минусы в один общий
    // и убираем лишние

    std::list< ASTNodeList::iterator> Uminuses;         // работаем на версиях Replace/Extract на итераторах

    // собираем итераторы на дочерние унарные минусы и отрицательные константы
    for (auto c = Children.begin() ; c != Children.end() ; c++)
        switch ((*c)->GetType())
        {
        case ASTNodeType::Uminus:
            Uminuses.push_back(c);
            break;
        case ASTNodeType::Numeric:
            if (NumericValue(*c) < 0)
                Uminuses.push_back(c);
            break;
        }

    // убираем все унарные минусы
    for (auto um = Uminuses.begin(); um != Uminuses.end(); um++)
    {
        CASTNodeBase* pum = **um;
        switch (pum->GetType())
        {
        case ASTNodeType::Uminus:
            ReplaceChild(*um, pum->ExtractChild(pum->ChildNodes().begin()));
            break;
        case ASTNodeType::Numeric:
            static_cast<CASTNumeric*>(pum)->SetNumeric(-NumericValue(pum));
            break;
        }
    }

    if (Uminuses.size() % 2)
    {
        // если унарных минусов было нечетное число, вводим между произведением и родителем
        // новый унарный минус
        pParent->InsertItermdiateChild<CASTUminus>(this);
    }
}

void CASTMul::FoldConstants()
{
    FoldZeroMultiplier();
    if (Deleted()) return;
    FoldSingleArgument();
    if (Deleted()) return;
    FoldUnaryMinuses();
    if (Deleted()) return;
    // собираем все константы в одну
    CollectNumericTerms([](double left, double right) {return left * right; }, 1.0);

}

bool CASTMul::CompareNumericVariable(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Numeric);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Variable);
    return true;
}

bool CASTMul::CompareNumericFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Numeric);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Function);
    return true;
}

bool CASTMul::CompareNumericOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Numeric);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Operator);
    return true;
}

bool CASTMul::CompareVariableFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Variable);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Function);
    return true;
}

bool CASTMul::CompareVariableOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    //std::cout << "v-o" << std::endl;
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Variable);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Operator);
    // здесь надо продолжить сортировку если степени одинаковые
    return rhs->SortPower() < lhs->SortPower();
    return true;
}

bool CASTMul::CompareFunctionFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Function);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Function);
    return lhs->GetText() < rhs->GetText();
}

bool CASTMul::CompareFunctionOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    //std::cout << "f-o" << std::endl;
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Function);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Operator);
    return true;
}

bool CASTMul::CompareOperatorOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    //std::cout << "o-o" << std::endl;
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Operator);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Operator);
    double power = lhs->SortPower() - rhs->SortPower();
    if (power < 0 || power > 0) return power > 0;
    return lhs->GetType() < rhs->GetType();
}


bool CASTMul::MulSortPredicate(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    SortNodeType lt(CASTFlatOperator::NodeTypeToSort(lhs));
    SortNodeType rt(CASTFlatOperator::NodeTypeToSort(rhs));

    if (lt < SortNodeType::Error && rt < SortNodeType::Error)
    {
        ptrdiff_t col(as_integer(lt)), row(as_integer(rt));
        if (col > row)
            return !(SortTable[SortIndex(row, col)])(rhs, lhs);
        else
            return (SortTable[SortIndex(col, row)])(lhs, rhs);
    }
    else
    {
        _ASSERTE(lt < SortNodeType::Error&& rt < SortNodeType::Error);
        return false;
    }
    return false;
}

void CASTMul::Collect()
{
    // собираем текстовые фрагменты составляющих произведение
    ASTFragmentsList fragments;
    Fragments(fragments);

    for (auto&& c : fragments)
    {
        // при сборке фрагментов удаляются те, которые встречаются менее двух раз
        _ASSERTE(c.fragmentset.size() > 1);

        // при сборке умножения будут образовываться степени
        struct Powers
        {
            CASTNodeBase* pChild;       // здесь собираются множители
            CASTNodeBase* pPow;         // здесь их степени
        };

        std::list<Powers> powers;

        // просматриваем фрагменты и убеждаемся, что множители можно собрать
        for (auto& cc : c.fragmentset)
        {
            // проверяем кто у множителя родитель
            CASTNodeBase* pParent = cc->GetParent();
            if (!pParent) continue;
            CASTNodeBase* pParentParent = pParent->GetParent();
            if (!pParentParent) continue;

            if (pParent == this)
            {
                // если само умножение, степени нет
                powers.push_back({ cc, nullptr });
            }
            else if (pParent->CheckType(ASTNodeType::Pow) &&
                     pParentParent == this &&
                     pParent->ChildNodes().front() == cc)
            {
                // если степень - то запоминаем узел степени
                // проверяем что родитель степень 
                // 1 - тип степень
                // 2 - родитель степени - умножение
                // 3 - множитель левый сын степени
                powers.push_back({ cc, pParent });
            }
        }

        powers.unique([](const auto& lhs, const auto& rhs)
            {
                return lhs.pPow == rhs.pPow && lhs.pPow != nullptr;
            }
        );


        // если подходящих множителей менее двух - продолжаем
        if (powers.size() < 2) continue;

        // создаем заготовку степени с заглушкой основания
        CASTNodeBase *pNewPow = CreateChild<CASTPow>();
        CASTNodeBase* pDummy = pNewPow->CreateChild<CASTAny>("");
        CASTNodeBase* pSum   = pNewPow->CreateChild<CASTSum>();

        for (auto&& p : powers)
        {
            if (p.pPow)
            {
                // если родитель - степень
                // то к сумме добавляем ее показатель
                pSum->CreateChild(p.pPow->ExtractChild(p.pPow->ChildNodes().back()));
                // если заглушка новой степени еще на месте - заменяем ее на основание степени, равное нашему множителю
                if (!pDummy->Deleted())
                    pNewPow->ReplaceChild(pDummy, p.pPow->ExtractChild(p.pPow->ChildNodes().front()));
                // степень удаляем
                DeleteChild(p.pPow);
            }
            else
            {
                // если родитель - само умножение, показатель степени 1 добавляем к сумме
                pSum->CreateChild<CASTNumeric>(1.0);
                // если заглушка новой степени еще на месте - заменяем ее на множитель
                if (!pDummy->Deleted())
                    pNewPow->ReplaceChild(pDummy, ExtractChild(p.pChild));
                else
                    DeleteChild(p.pChild);
            }
        }
        // если в результате сборки множителей остался только один - заменяем умножение в родителе на этот множитель
        FoldSingleArgument();
        if (pTree->IsChanged())
            break;
    }
}

void CASTMul::Sort()
{
    Children.sort([](const auto* lhs, const auto* rhs) {  return CASTMul::MulSortPredicate(lhs, rhs); });
}

CASTNodeBase* CASTMul::GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent)
{
    // создаем новый узел умножения
    // и ищем заданную переменную в дифференцируемом узле
    CASTMul *pMul = pDerivativeParent->CreateChild<CASTMul>();
    if (!FindVariable(this, itVar->first)) // если заданную переменную не нашли, вводим 0 в умножение
        pMul->CreateChild<CASTNumeric>(0.0);
    return pMul;
}