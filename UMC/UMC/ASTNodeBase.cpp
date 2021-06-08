#include "ASTNodeBase.h"
#include "ASTNodes.h"

CASTNodeBase* CASTNodeBase::GetParentOfType(const ASTNodeType ParentType)
{
    CASTNodeBase* pCurrentParent(pParent);
    while (pCurrentParent)
    {
        if (pCurrentParent->CheckType(ParentType))
            break;
        pCurrentParent = pCurrentParent->pParent;
    }
    return pCurrentParent;
}

// если запрашивают родительское уравнение уравнения
// возвращаем само уравнение
CASTEquation* CASTNodeBase::GetParentEquation()
{
    if (CheckType(ASTNodeType::Equation))
        return static_cast<CASTEquation*>(this);
    else
        return static_cast<CASTEquation*>(GetParentOfType(ASTNodeType::Equation));
}

void CASTNodeBase::PrintTree(std::string indent, bool last) const
{
    std::cout << indent;
    if (last)
    {
        std::cout << "\\-";
        indent += "  ";
    }
    else
    {
        std::cout << "|-";
        indent += "| ";
    }
    std::cout << as_integer(GetType()) << " [" << GetText() << "]" << std::endl;

    for (auto c : Children)
        c->PrintTree(indent, c == *std::prev(Children.end()));
}


std::string& CASTNodeBase::ChildrenSplitByToInfix(std::string_view Delimiter)
{
    auto c = Children.begin();

    if (c != Children.end())
    {
        infix += (*c)->Infix();
        c++;
        while (c != Children.end())
        {
            infix += Delimiter;
            infix += (*c)->Infix();
            c++;
        }
    }
    return infix;
}


double CASTNodeBase::MaxSortPower()
{
    sortPower = 0.0;
    auto c = Children.begin();
    if (c != Children.end())
    {
        sortPower = (*c)->SortPower();
        c++;
        while (c != Children.end())
        {
            if (std::abs(sortPower) < std::abs((*c)->SortPower()))
                sortPower = (*c)->SortPower();
            c++;
        }
    }
    return sortPower;
}


ptrdiff_t CASTNodeBase::Compare(const CASTNodeBase* pNode) const
{
    ASTNodeConstStack stackleft,   stackright;
    ASTNodeConstSet   visitedleft, visitedright;
    // создаем два стека для this и данного узлов
    stackleft.push(this);  stackright.push(pNode);
    while (true)
    {
        // проверяем глубину стека
        ptrdiff_t nDiff = stackleft.size() - stackright.size();
        if (nDiff != 0) return nDiff;

        // проверяем наш стек, если пустой - выходим
        // стек данного тоже пуст (мы проверили глубину раньше)
        if (stackleft.empty())
        {
            _ASSERTE(Infix() == pNode->Infix());
            return 0;
        }

        auto left  = stackleft.top();   stackleft.pop();
        auto right = stackright.top();  stackright.pop();

        // сравниваем типы узлов
        nDiff = as_integer(left->GetType()) - as_integer(right->GetType());
        if (nDiff != 0) return nDiff;

        // сравниваем количество дочерних узлов
        nDiff = left->ChildNodes().size() - right->ChildNodes().size();
        if (nDiff != 0) return nDiff;

        switch (left->GetType())
        {
        case ASTNodeType::Variable:
            // если узел - переменная - сравниваем имена переменных
            nDiff = left->GetText().compare(right->GetText());
            if (nDiff != 0) return nDiff;
            break;
        case ASTNodeType::Numeric:
            {
                // если узел число - сравниваем числа
                double dDiff = NumericValue(left) - NumericValue(right);
                if (dDiff < 0.0 || dDiff > 0.0) return dDiff < 0.0 ? -1 : 1;
            }
            break;
        }

        bool leftVisited = visitedleft.Contains(left);
        bool rightVisited = visitedright.Contains(right);

        if (!leftVisited && !rightVisited)
        {
            visitedleft.insert(left);   visitedright.insert(right);
            // добавляем дочерние узлы в стек
            for (auto c : left->ChildNodes())
                stackleft.push(c);
            for (auto c : right->ChildNodes())
                stackright.push(c);

            // если сравниваем функции, в стек добавляем также и константные узлы
            if (left->IsFunction())
            {
                if (!right->IsFunction())
                    EXCEPTIONMSG("Function is compared with non-fuction");
                const CASTFunctionBase* fnLeft(static_cast<const CASTFunctionBase*>(left));
                const CASTFunctionBase* fnRight(static_cast<const CASTFunctionBase*>(right));

                // для каждой функции подставляем в цикл сравнения саму функцию и ее стек
                for (auto& fn : { std::make_pair(fnLeft, &stackleft),  std::make_pair(fnRight, &stackright) })
                {
                    // проходим по константным аргументам функции
                    for (auto& c : fn.first->ConstantArguments)
                    {
                        if (!c.first->CheckType(ASTNodeType::Equation))
                            EXCEPTIONMSG("Constant argument must be in equation form");
                        c.first->CheckChildCount();
                        // помещаем константный аргумент в заданный стек
                        fn.second->push(c.first->ChildNodes().back());
                    }
                }
            }
        }
        else
        {
            // странная ситуация - одно поддерево обошли,
            // другое нет
            if (leftVisited != rightVisited)
                return leftVisited ? -1 : 1;
        }
    }
}

// возвращает строку с данными об уравнении, к которому
// принадлежит данный узел

std::string CASTNodeBase::ParentEquationDescription()
{
    CASTEquation* pEquation(GetParentEquation());
    if (pEquation)
        return pEquation->GetEquationDescription();
    else
        EXCEPTIONMSG("Parent equation not found for description");
}

// возвращает строку с данными об уравнении, к которому
// принадлежит данный узел (только исходный текст)

std::string CASTNodeBase::ParentEquationSourceDescription()
{
    CASTEquation* pEquation(GetParentEquation());
    if (pEquation)
        return pEquation->GetEquationSourceDescription();
    else
        EXCEPTIONMSG("Parent equation not found for description");
}


void CASTNodeBase::Warning(std::string_view warning, bool SourceOnly)
{
     pTree->Warning(fmt::format("{} {}", warning, SourceOnly ? ParentEquationSourceDescription() : ParentEquationDescription()));
}

void CASTNodeBase::Error(std::string_view error, bool SourceOnly)
{
    if (!IsError)
    {
        IsError = true;
        pTree->Error(fmt::format("{} {}", error, SourceOnly ? ParentEquationSourceDescription() : ParentEquationDescription()));
    }
}

bool CASTNodeBase::TrasformNodeToOperator(CASTNodeBase* pNode)
{
    // здесь может быть правило, по которому создается уравнение для оператора
    auto pParent(pNode->GetParent());
    if (!pParent) return false;
    // если узел трансформирует и его родитель тоже трансформирует - вводим уравнение
    if (IsTransforming(pNode) && IsTransforming(pParent))
        return true;
    // если родитель хост-блок, а узел константа - смотрим, не должен ли быть 
    // соответствующий дочерний узел хост-блока переменной
    if (IsHostBlock(pParent) && pNode->IsConst())
    {
        // просматриваем список пар атрибутов аргументов 
        // и дочерних узлов. Находим текущий узел, и если 
        // он не должен быть константой, требуем, чтобы для
        // него было создано отдельное уравнение, так как хост-блоки
        // могут принимать на вход либо константы, либо переменные состояния

        CASTFunctionBase::ArgumentProps props;
        static_cast<const CASTHostBlockBase*>(pParent)->GetArgumentProps(props);
        for (auto& c : props)
            if (c.pChild == pNode && c.argument.VariableOnly)   // вместо переменной в аргументе константа
                return true;                                    // формируем уравнение
    }
    return false;
}
