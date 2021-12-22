#include "ASTNodes.h"

CASTFlatOperator::SortNodeType CASTFlatOperator::NodeTypeToSort(const CASTNodeBase* pNode)
{
    SortNodeType nodeType(SortNodeType::Error);
#ifdef _DEBUG
    #define Type(a) { _ASSERTE(nodeType == SortNodeType::Error); nodeType = (a); }
#else
    #define Type(a) nodeType = (a);
#endif
    if (IsNumeric(pNode))                                   Type(SortNodeType::Numeric);
    if (IsVariable(pNode))                                  Type(SortNodeType::Variable);
    if (IsFunction(pNode))                                  Type(SortNodeType::Function);
    if (IsOperator(pNode))                                  Type(SortNodeType::Operator);
    _ASSERTE(nodeType != SortNodeType::Error);
    return nodeType;
}

bool CASTSum::CompareNumericNumeric(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Numeric);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Numeric);
    return CASTNodeBase::NumericValue(lhs) < CASTNodeBase::NumericValue(rhs);
}


bool CASTSum::CompareNumericVariable(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Numeric);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Variable);
    return false;
}

bool CASTSum::CompareNumericFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Numeric);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Function);
    return false;
}

bool CASTSum::CompareNumericOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Numeric);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Operator);
    return false;
}

bool CASTSum::CompareVariableVariable(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Variable);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Variable);
    return lhs->GetText() < rhs->GetText();
}

bool CASTSum::CompareVariableFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Variable);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Function);
    return true;
}

bool CASTSum::CompareVariableOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    //std::cout << "v-o" << std::endl;
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Variable);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Operator);
    double pow(rhs->SortPower() - lhs->SortPower());
    if (pow < 0.0 || pow > 0.0) return pow < 0.0;
    if (IsUminus(rhs)) return true;
    return false;
}

bool CASTSum::CompareFunctionFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Function);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Function);
    return lhs->GetText() < rhs->GetText();
}

bool CASTSum::CompareFunctionOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    //std::cout << "f-o" << std::endl;
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Function);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Operator);
    return true;
}

bool CASTSum::CompareOperatorOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    //std::cout << "o-o" << std::endl;
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(lhs) == CASTFlatOperator::SortNodeType::Operator);
    _ASSERTE(CASTFlatOperator::NodeTypeToSort(rhs) == CASTFlatOperator::SortNodeType::Operator);
    double pow(rhs->SortPower() - lhs->SortPower());
    if (pow < 0.0 || pow > 0.0) return pow < 0.0;
    ptrdiff_t ndiff(rhs->ChildNodes().size() - lhs->ChildNodes().size());
    if (ndiff != 0.0) return ndiff < 0.0;
    ndiff = as_integer(rhs->GetType()) - as_integer(lhs->GetType());
    if (ndiff != 0.0) return ndiff < 0.0;
    return lhs->Infix() < rhs->Infix();
}

// сортировка внутри суммы выполняется с помощью функций, который выбираются
// по таблице. Таблица представляет собой нижний треугольник квадрата, в котором заданы
// указатели функций, сравнивающие определенные типы узлов.

bool CASTSum::SumSortPredicate(const CASTNodeBase* lhs, const CASTNodeBase* rhs)
{
    // строку и столбец таблицы выбираем с помощью приведения типа узла к типу для сравнения
    SortNodeType lt(CASTFlatOperator::NodeTypeToSort(lhs));
    SortNodeType rt(CASTFlatOperator::NodeTypeToSort(rhs));

    if (lt < SortNodeType::Error && rt < SortNodeType::Error)
    {
        // если типы определены корректно, выбираем индексы строки и столбца
        ptrdiff_t col(as_integer(lt)), row(as_integer(rt));
        // если столбец больше строки - меняем их местами (переводим верхний треугольник)
        // в симметричный ему нижний
        if (col > row)
            return !(SortTable[SortIndex(row, col)])(rhs, lhs);     // сравниваем тип2 < тип1, но нужно наоборот, поэтому
        else                                                        // инвертируем результат
            return (SortTable[SortIndex(col, row)])(lhs, rhs);      // сравниваем тип1 < тип2
    }
    else
    {
        _ASSERTE(lt < SortNodeType::Error&& rt < SortNodeType::Error);
        return false;
    }
    return false;
}

void CASTSum::Collect()
{
    // собираем текстовые фрагменты составляющих произведение
    ASTFragmentsList fragments;
    Fragments(fragments);
    for (auto&& c : fragments)
    {
        // при сборке фрагментов удаляются те, которые встречаются менее двух раз
        _ASSERTE(c.fragmentset.size() > 1);

        // при сборке суммы будут умножения
        struct Multipliers
        {
            CASTNodeBase* pChild;       // здесь собираются слагаемые
            CASTNodeBase* pMul;         // здесь их множители
        };

        std::list<Multipliers> multipliers;

        // просматриваем фрагменты и убеждаемся, что слагаемые можно собрать
        for (auto& cc : c.fragmentset)
        {
            // проверяем кто у слагаемого родитель
            CASTNodeBase* pParent = cc->GetParent();
            if (!pParent) continue;
            CASTNodeBase* pParentParent = pParent->GetParent();
            if (!pParentParent) continue;

            if (pParent == this)
            {
                // если сама сумма, множителя нет
                multipliers.push_back({ cc, nullptr });
            }
            else if (pParent->CheckType(ASTNodeType::Uminus) && pParentParent == this)
            {
                // если унарный минус - то он же и множитель
                multipliers.push_back({ cc, pParent});
            }
            else if (pParent->CheckType(ASTNodeType::Mul))
            {
                // если родительский узел - умножение
                if(pParentParent == this)
                {
                    // если умножение сразу идет к сумме
                    multipliers.push_back({ cc, pParent });
                }
                else if (IsUminus(pParentParent) && pParentParent->GetParent() == this)
                {
                    // если умножение идет к сумме через унарный минус
                    multipliers.push_back({ cc, pParent });
                }
            }
        }

        // все множители из которых извлекаем подобное должны быть уникальны

        multipliers.unique([](const auto& lhs, const auto& rhs)
            {
                return lhs.pMul == rhs.pMul && lhs.pMul != nullptr;
            }
        );


        // если подходящих слагаемых менее двух - пропускаем
        if (multipliers.size() < 2) continue;

        // создаем заготовку умножения с заглушкой основания
        CASTNodeBase* pNewMul = CreateChild<CASTMul>();
        CASTNodeBase* pSum = pNewMul->CreateChild<CASTSum>();
        CASTNodeBase* pDummy = pNewMul->CreateChild<CASTAny>("");


        for (auto&& m : multipliers)
        {
            if (m.pMul)
            {
                // если родитель - множитель или унарный минус
                // если заглушка нового умножения еще на месте - заменяем ее на слагаемое 
                CASTNodeBase* pTerm = m.pMul->ExtractChild(m.pChild);

                if (!pDummy->Deleted())
                    pNewMul->ReplaceChild(pDummy, pTerm);
                else
                    pTree->DeleteNode(pTerm);

                // если после извлечения слагаемого в умножении остался 1 множитель или в унарном минусе - 0 аргументов
                // добавляем единицу
                if (m.pMul->ChildNodes().size() < 2)
                    m.pMul->CreateChild<CASTNumeric>(1.0);

                if (m.pMul->CheckType(ASTNodeType::Mul) && IsUminus(m.pMul->GetParent()))
                {
                    // если умножение входит в сумму через унарный минус
                    pTerm = ExtractChild(m.pMul->GetParent());
                }
                else
                {
                    // если умножение без унарного минуса или сам унарный минус
                    // извлекаем множитель из данной суммы и добавляем в сумму множителей слагаемого
                    pTerm = ExtractChild(m.pMul);
                }
                pSum->CreateChild(pTerm);
            }
            else
            {
                // если родитель - само умножение, множитель 1 добавляем к сумме
                pSum->CreateChild<CASTNumeric>(1.0);
                // если заглушка нового умножения еще на месте - заменяем ее на слагаемое
                if (!pDummy->Deleted())
                    pNewMul->ReplaceChild(pDummy, ExtractChild(m.pChild));
                else
                    DeleteChild(m.pChild); // если заглушки нет - удаляем слагаемое
            }
        }
       
        //GetInfix();
        FoldSingleArgument();
        if (pTree->IsChanged())
            break;
    }
}

void CASTSum::Sort()
{
    Children.sort([](const auto* lhs, const auto* rhs) {  return CASTSum::SumSortPredicate(lhs, rhs); });
}
