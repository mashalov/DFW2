#pragma once

#include "ASTNodeBase.h"

class CASTRoot : public CASTNodeBase
{
    using CASTNodeBase::CASTNodeBase;
public:
    ASTNodeType GetType() const override { return ASTNodeType::Root; }
    ptrdiff_t DesignedChildrenCount() const override { return 2; }
    void ToInfix() override
    {
        infix.clear();
        ChildrenSplitByToInfix("\n");
        infix += "\n";
    }
};


class CASTEquationSection : public CASTNodeBase
{
    using CASTNodeBase::CASTNodeBase;
public:
    void ToInfix() override
    {
        infix = GetText();
        infix += "\n{\n";
        ChildrenSplitByToInfix("\n");
        infix += "\n}";
    }
};

class CASTMain : public CASTEquationSection
{
    using CASTEquationSection::CASTEquationSection;
    static inline std::string_view static_text = "main";
public:
    ASTNodeType GetType() const override { return ASTNodeType::Main; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTMain::static_text; }
};

class CASTInit : public CASTEquationSection
{
    using CASTEquationSection::CASTEquationSection;
    static inline constexpr std::string_view static_text = "init";
public:
    ASTNodeType GetType() const override { return ASTNodeType::Init; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTInit::static_text; }
};

class CASTJacobian : public CASTEquationSection
{
    using CASTEquationSection::CASTEquationSection;
    static inline constexpr std::string_view static_text = "jacobian";
public:
    ASTNodeType GetType() const override { return ASTNodeType::Jacobian; }
    ptrdiff_t DesignedChildrenCount() const override { return -1; }
    const std::string_view GetText() const override { return CASTJacobian::static_text; }
};

class CASTInitConstants : public CASTEquationSection
{
    using CASTEquationSection::CASTEquationSection;
    static inline constexpr std::string_view static_text = "constargs";
public:
    ASTNodeType GetType() const override { return ASTNodeType::ConstArgs; }
    ptrdiff_t DesignedChildrenCount() const override { return -1; }
    const std::string_view GetText() const override { return CASTInitConstants::static_text; }
};

class CASTInitExtVars : public CASTEquationSection
{
    using CASTEquationSection::CASTEquationSection;
    static inline constexpr std::string_view static_text = "initextvars";
public:
    ASTNodeType GetType() const override { return ASTNodeType::InitExtVars; }
    ptrdiff_t DesignedChildrenCount() const override { return -1; }
    const std::string_view GetText() const override { return CASTInitExtVars::static_text; }
};

class CASTVariable : public CASTNodeTextBase
{
protected:
    VariableInfo& varInfo;
public:
    CASTVariable(CASTTreeBase* Tree,
                 CASTNodeBase* Parent,
                 std::string_view Text) : CASTNodeTextBase(Tree, Parent, Text),
                                          varInfo(Tree->GetVariableInfo(Text))
    {
        // при создании переменную сразу вносим
        // в карту переменных дерева
        IsConst();
        varInfo.VarInstances.insert(this);
    }
        
    void SetText(std::string_view Text) override
    {
        EXCEPTIONMSG("SetText for CASTVariable prohibited");
    }
    ASTNodeType GetType() const override { return ASTNodeType::Variable; }
    ptrdiff_t DesignedChildrenCount() const override { return 0; }
    void ToInfix() override
    {
        infix = GetText();
    }

    void OnDelete() override
    {
        // при удалении переменной
        // удаляем ее из списка экземпляров
        // переменных в карте
        if (!varInfo.VarInstances.erase(this))
            EXCEPTIONMSG("No variable in map");
        CASTNodeTextBase::OnDelete();
    }

    VariableInfo& Info()
    {
        return varInfo;
    }

    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent);

    CASTNodeBase* Clone(CASTNodeBase* pParent) override {  return pParent->CreateChild<CASTVariable>(GetText()); }

    // для переменной условие постоянства 
    // определяется по метаданным. Все экземпляры переменных
    // с одинаковыми именами имеют общий признак константности
    bool IsConst() override
    {
        return Constant = varInfo.Constant;
    }

    void SetConst(bool bConst) override
    {
        // извлекаем признак константы из метаданных
        IsConst();
        // проверяем, что нет ситуации сделать из константы не константу
        CASTNodeBase::SetConst(bConst);
        // вводим признак константы в метаданные
        varInfo.Constant = Constant;
        // далее проходим по всем экземплярам данной переменной
        // и синхронизируем встроенные признаки CASTNodeBase из метаданных
        for (auto& v : varInfo.VarInstances)
            v->IsConst();
    }
};

class CASTAny : public CASTNodeTextBase
{
    using CASTNodeTextBase::CASTNodeTextBase;
    const CASTNodeBase* pMatch = nullptr;
    static inline const ptrdiff_t precedence = 1;
public:
    ASTNodeType GetType() const override { return ASTNodeType::Any; }
    ptrdiff_t DesignedChildrenCount() const override { return 0; }
    ptrdiff_t Precedence() const override { return precedence; };
    void SetMatch(const CASTNodeBase* pMatchedNode)
    {
        pMatch = pMatchedNode;
    }
    const CASTNodeBase* GetMatch() const
    {
        return pMatch;
    }
    void ToInfix() override
    {
        infix = "[";
        infix += pMatch ? GetText() : "Any";
        infix += "]";
    }
};

class CASTEquationSystem : public CASTNodeBase
{
    using CASTNodeBase::CASTNodeBase;
    static inline constexpr std::string_view static_text = "EQSys";
public:
    ASTNodeType GetType() const override { return ASTNodeType::EquationSystem; }
    ptrdiff_t DesignedChildrenCount() const override { return -1; }
    const std::string_view GetText() const override { return CASTEquationSystem::static_text; }
    void ToInfix() override
    {
        infix.clear();
        ChildrenSplitByToInfix("\n");
    }
};

class CASTNumeric : public CASTNodeTextBase
{
public:
    CASTNumeric(CASTTreeBase* Tree,
        CASTNodeBase* Parent,
        std::string_view Text = std::string_view("")) : CASTNodeTextBase(Tree, Parent, Text)
    {
        Constant = true;
    }

    CASTNumeric(CASTTreeBase* Parser,
        CASTNodeBase* Parent,
        double Value) : CASTNodeTextBase(Parser, Parent, to_string(Value)) {}

    ASTNodeType GetType() const override { return ASTNodeType::Numeric; }
    ptrdiff_t DesignedChildrenCount() const override { return 0; }
    void ToInfix() override
    {
        infix = GetText();
    }

    // производная зависит от контекста
    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent)
    {
        CASTNodeBase* pDerivativeCtx(pDerivativeParent->GetDerivativeContext());
        switch (pDerivativeCtx->GetType())
        {
        case ASTNodeType::Sum: // для суммы всегда 0
            return pDerivativeParent->CreateChild<CASTNumeric>(0.0);
        case ASTNodeType::Mul: // для умножения - сама константа
            return Clone(pDerivativeParent);
        default:
            EXCEPTIONMSG("Unhandled derivative context");
        }
    }
    
    CASTNodeBase* Clone(CASTNodeBase* pParent)
    {
        return pParent->CreateChild<CASTNumeric>(NumericValue(this));
    }
};

class CASTOperator : public CASTNodeBase
{
    using CASTNodeBase::CASTNodeBase;
public:
    std::string InfixParenthesis(const CASTNodeBase* pNode, bool bRight)
    {
        /*
            https://stackoverflow.com/questions/14175177/how-to-walk-binary-abstract-syntax-tree-to-generate-infix-notation-with-minimall
            1. You never need parentheses around the operator at the root of the AST.
            2. If operator A is the child of operator B, and A has a higher precedence than B, the parentheses around A can be omitted.
            3. If a left-associative operator A is the left child of a left-associative operator B with the same precedence,
               the parentheses around A can be omitted. A left-associative operator is one for which x A y A z is parsed as (x A y) A z.
            4. If a right-associative operator A is the right child of a right-associative operator B with the same precedence,
               the parentheses around A can be omitted. A right-associative operator is one for which x A y A z is parsed as x A (y A z).
            5. If you can assume that an operator A is associative, i.e. that (x A y) A z = x A (y A z) for all x,y,z,
               and A is the child of the same operator A, you can choose to omit parentheses around the child A.
               In this case, reparsing the expression will yield a different AST that gives the same result when evaluated.
        /*

        */
        bool bNeedParenthesis(true);
        std::string Infix(pNode->Infix());
        if (!IsLeaf(pNode))
        {
            // если приоритет дочернего оператора больше, чем приоритет данного оператора,
            // скобки не нужны
            if (pNode->Precedence() > Precedence())
                bNeedParenthesis = false;
            if (IsUminus(this) && pNode->CheckType(ASTNodeType::Mul)) // не ставим унарный минус перед произведением
                bNeedParenthesis = false;
            else
                if (pNode->RightAssociativity() == RightAssociativity() && pNode->Precedence() == Precedence())
                {
                    // если ассоциативность и приоритет равны
                    if (bRight && RightAssociativity())             // и дочерний оператор справа от данного правоассоциативного
                        bNeedParenthesis = false;                   // оператора - скобки не нужны
                    if (!bRight && !RightAssociativity())           // и дочерний оператор слева от данного левоассоциативного 
                        bNeedParenthesis = false;                   // оператора - скобки не нужны
                }
        }
        else
            bNeedParenthesis = false;

        if (bNeedParenthesis)
        {
            Infix.insert(Infix.begin(), '(');
            Infix.push_back(')');
        }
        return Infix;
    }
};

class CASTInfixBase : public CASTOperator
{
public:
    using CASTOperator::CASTOperator;
    void ToInfix() override
    {
        infix.clear();
        auto c = Children.begin();

        if (c != Children.end())
        {
            infix += InfixParenthesis(*c, false);
            c++;
            while (c != Children.end())
            {
                // для суммы с унарным минусом в правом операнде
                // убираем "+" так, чтобы вместо него в выражение вошел унарный минус
                // или отрицательное число
                if (CheckType(ASTNodeType::Sum))
                {
                    if(!IsUminus(*c) && !(CASTNodeBase::IsNumeric(*c) && CASTNodeBase::NumericValue(*c) < 0))
                        infix += GetText();
                }
                else
                    infix += GetText();

                infix += InfixParenthesis(*c, true);
                c++;
            }
        }
    }
};


class CASTFlatOperator : public CASTInfixBase
{
protected:
    void DeleteIdentities(double Identity)
    {
        auto c = Children.begin();
        while (c != Children.end())
        {
            if (IsNumeric(*c) && NumericValue(*c) == Identity)
                c = DeleteChild(c);
            else
                c++;
        }
    }
    void CollectNumericTerms(std::function<double(double, double)> Operand, double Identity)
    {
        DeleteIdentities(Identity);
        std::list<ASTNodeList::iterator> Numerics;
        double Val(Identity);
        for (auto c = Children.begin(); c != Children.end(); c++)
            if (IsNumeric(*c))
            {
                Val = (Operand)(Val, NumericValue(*c));
                Numerics.push_back(c);
            }

        if (Numerics.size() > 1)
        {
            (*Numerics.front())->SetText(Val);
            Numerics.pop_front();
            for (auto dc : Numerics)
                DeleteChild(dc);
        }

        DeleteIdentities(Identity);

        if (Children.size() == 0)
            pParent->ReplaceChild<CASTNumeric>(this, Identity);
        else
            FoldSingleArgument();
        /*
        if (SingleNumericChildValue(this, Val))
            pParent->ReplaceChild<CASTNumeric>(this, Val);
        */
    }

public:
    using CASTInfixBase::CASTInfixBase;
    virtual void Sort() {};
    virtual void Collect() {};

    void FoldSingleArgument()
    {
        if (Children.size() == 1)
            pParent->ReplaceChild(this, ExtractChild(Children.front()));
    }

    virtual void Flatten()
    {
        FlattenImpl(this);
    }

    static void FlattenImpl(CASTNodeBase* pNode)
    {
        // находим в дочерних узлах узлы с типом, идентичным родительскому
        auto c = pNode->ChildNodes().begin();
        while (c != pNode->ChildNodes().end())
        {
            if ((*c)->GetType() == pNode->GetType())
            {
                // если тип дочернего узла идентичен типу данного узла
                ASTNodeList newChildren;
                // вынимаем дочерние узлы дочернего узла
                (*c)->ExtractChildren(newChildren);
                // добавляем вынутые узлы как дочерние к данному
                pNode->AppendChildren(newChildren);
                // дочерний узел убираем
                c = pNode->DeleteChild(c);
            }
            else
                c++;
        }
    }

    enum class SortNodeType
    {
        Numeric,
        Variable,
        Function,
        Operator,
        Error
    };

    static SortNodeType NodeTypeToSort(const CASTNodeBase* pNode);

    static inline constexpr size_t CompareTableSize = as_integer(CASTFlatOperator::SortNodeType::Error);
    static inline constexpr size_t CompareArraySize = ((CompareTableSize + 1) * CompareTableSize) / 2;

    static inline size_t SortIndex(ptrdiff_t Col, ptrdiff_t Row)
    {
        if (Col >= CompareTableSize || Row >= CompareTableSize)
            EXCEPTIONMSG("Wrong Col or Row");

        size_t index(CompareTableSize * Col - (Col) * (Col - 1) / 2 + Row - Col);
        if(index >= CompareArraySize)
            EXCEPTIONMSG("Wrong index computed");
        return index;
    };

};


class CASTSum : public CASTFlatOperator
{
    using CASTFlatOperator::CASTFlatOperator;
    static inline constexpr std::string_view static_text = "+";
public:
    static inline const ptrdiff_t precedence = 5;
    ASTNodeType GetType() const override { return ASTNodeType::Sum; }
    ptrdiff_t DesignedChildrenCount() const override { return -1; }
    ptrdiff_t Precedence() const override { return precedence; };
    const std::string_view GetText() const override { return CASTSum::static_text; }
    void FoldConstants() override
    {
        FoldSingleArgument();
        if (Deleted()) return;
        CollectNumericTerms([](double left, double right) {return left + right; }, 0.0);
    }
    void Sort() override;
    void Collect() override;
    static bool SumSortPredicate(const CASTNodeBase* lhs, const CASTNodeBase* rhs);

    static bool CompareNumericNumeric(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareNumericVariable(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareNumericFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareNumericOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareVariableVariable(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareVariableFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareVariableOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareFunctionFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareFunctionOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareOperatorOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs);

    // таблица сравнения узлов с разными типами (нижний треугольник таблицы)
    static const std::array<bool(*)(const CASTNodeBase*, const CASTNodeBase*), CASTFlatOperator::CompareArraySize > constexpr SortTable =
    {
         CompareNumericNumeric,    CompareNumericVariable,   CompareNumericFunction,   CompareNumericOperator ,
         CompareVariableVariable,  CompareVariableFunction,  CompareVariableOperator,
         CompareFunctionFunction,  CompareFunctionOperator,
         CompareOperatorOperator
    };

    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVariable, CASTNodeBase* pDerivativeParent) override
    {
        CASTSum* pSum = pDerivativeParent->CreateChild<CASTSum>();
        return pSum;
    }

    CASTNodeBase* Clone(CASTNodeBase* pParent) override
    {
        return CloneImpl(pParent, this);
    }

};

class CASTMinus: public CASTInfixBase
{
    using CASTInfixBase::CASTInfixBase;
    static inline constexpr std::string_view static_text = "-";
public:
    static inline const ptrdiff_t precedence = CASTSum::precedence;
    ASTNodeType GetType() const override { return ASTNodeType::Sum; }
    ptrdiff_t DesignedChildrenCount() const override { return 2; }
    ptrdiff_t Precedence() const override { return precedence; };
    void Flatten() override;
    const std::string_view GetText() const override { return CASTMinus::static_text; }
};

class CASTMul : public CASTFlatOperator
{
    using CASTFlatOperator::CASTFlatOperator;
    void FoldZeroMultiplier();
    void FoldUnaryMinuses();
    static inline constexpr std::string_view static_text = "*";
public:
    static inline const ptrdiff_t precedence = CASTSum::precedence + 1;
    ASTNodeType GetType() const override { return ASTNodeType::Mul; }
    ptrdiff_t DesignedChildrenCount() const override { return -1; }
    ptrdiff_t Precedence() const override { return precedence; };
    const std::string_view GetText() const override { return CASTMul::static_text; }
    void FoldConstants() override;
    void Sort() override;
    void Collect() override;
    static bool MulSortPredicate(const CASTNodeBase* lhs, const CASTNodeBase* rhs);

    static bool CompareNumericVariable(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareNumericFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareNumericOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareVariableFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareVariableOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareFunctionFunction(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareFunctionOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs);
    static bool CompareOperatorOperator(const CASTNodeBase* lhs, const CASTNodeBase* rhs);

    // некоторые функции сравнения в умножении подходят от суммы
    static const std::array<bool(*)(const CASTNodeBase*, const CASTNodeBase*), CASTFlatOperator::CompareArraySize > constexpr SortTable =
    {
         CASTSum::CompareNumericNumeric,    CompareNumericVariable,   CompareNumericFunction,   CompareNumericOperator ,
         CASTSum::CompareVariableVariable,  CompareVariableFunction,  CompareVariableOperator,
         CompareFunctionFunction,           CompareFunctionOperator,
         CompareOperatorOperator
    };

    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent) override;
    CASTNodeBase* Clone(CASTNodeBase* pParent) override
    {
        return pParent->CreateChild<CASTMul>();
    }
};

class CASTDiv : public CASTInfixBase
{
    using CASTInfixBase::CASTInfixBase;
    static inline constexpr std::string_view static_text = "/";
public:
    ASTNodeType GetType() const override { return ASTNodeType::Pow; }
    ptrdiff_t DesignedChildrenCount() const override { return 2; }
    static inline const ptrdiff_t precedence = CASTMul::precedence;
    ptrdiff_t Precedence() const override { return precedence; };
    const std::string_view GetText() const override { return CASTDiv::static_text; }
    void FoldConstants() override;
};


class CASTModLinkBase : public CASTNodeBase
{
    using CASTNodeBase::CASTNodeBase;
public:
    ASTNodeType GetType() const override { return ASTNodeType::ModLinkBase; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    void ToInfix() override
    {
        infix = "#";
        infix += Children.front()->GetText();
    }
};

class CASTUminus : public CASTOperator
{
    using CASTOperator::CASTOperator;
    static inline constexpr std::string_view static_text = "-";
public:
    ASTNodeType GetType() const override { return ASTNodeType::Uminus; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    bool RightAssociativity() const override { return true; }
    static inline const ptrdiff_t precedence = CASTMul::precedence + 1;
    ptrdiff_t Precedence() const override { return precedence; };
    const std::string_view GetText() const override { return CASTUminus::static_text; }

    void FoldConstants() override
    {
        double arg(0.0);
        if (SingleNumericChildValue(this, arg))
            pParent->ReplaceChild<CASTNumeric>(this, -arg);
    }

    void FlattenTwoConsecutive()
    {
        // удаляем два последовательных унарных минуса
        CASTNodeBase* pChild(Children.front());
        if (pChild->GetType() == GetType())
        {
            CASTNodeBase* p2ndMinusArg = pChild->ExtractChild(pChild->ChildNodes().begin());
            GetParent()->ReplaceChild(this, p2ndMinusArg);
        }
    }

    void FlattenMinusSum()
    {
        // раскрываем скобки у суммы с минусом
        CASTNodeBase* pChild(Children.front());
        if (IsSum(pChild))
        {
            // раскрываем скобки в случае, если внутри суммы не менее чем у половины слагаемых унарные минусы
            size_t UminusCount = std::count_if(pChild->ChildNodes().begin(), pChild->ChildNodes().end(), [](const auto& p)
                {
                    return IsUminus(p);
                });

            if (UminusCount * 2 >= pChild->ChildNodes().size())
            {
                CASTNodeBase* pSum = ExtractChild(pChild);
                GetParent()->ReplaceChild(this, pSum);
                ASTNodeList newChildren;
                pSum->ExtractChildren(newChildren);
                for (auto&& c : newChildren)
                    pSum->CreateChild<CASTUminus>()->CreateChild(c);
            }
        }
    }

    void Flatten() override
    {
        FlattenTwoConsecutive();
        if (Deleted()) return;
        FlattenMinusSum();
    }

    void ToInfix() override
    {
        infix  = GetText();
        infix += InfixParenthesis(Children.front(), true);
    }

    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent) override
    {
        return pDerivativeParent->CreateChild<CASTUminus>();
    }

    CASTNodeBase* Clone(CASTNodeBase* pParent) override
    {
        return pParent->CreateChild<CASTUminus>();
    }
};

class CASTFunctionBase : public CASTNodeBase
{
    using CASTNodeBase::CASTNodeBase;
    static inline const ptrdiff_t precedence = CASTUminus::precedence + 1;
public:

    struct FunctionInfo
    {
        // данные о свойствах аргументов функици
        struct Argument
        {
            bool ConstantOnly = false;        // признак только константного аргумента
            bool VariableOnly = false;        // признак того, что в аргументе может быть только переменная
            bool Optional     = false;        // признак того, что аргумент может быть не задан
            bool OptionalList = false;        // признак того, что аргумент является опциональным, и после него может быть любое количество аргументов
        };

        static inline constexpr Argument ConstantOnly = { true , false, false, false };
        static inline constexpr FunctionInfo::Argument VariableOnly = { false, true, false, false };
        static inline constexpr FunctionInfo::Argument Any = { false, false, false, false };
        static inline constexpr FunctionInfo::Argument Optional = { false, false, true, false };
        static inline constexpr FunctionInfo::Argument OptionalList = { false, false, true, true };

        const std::vector<Argument> Args;               // вектор свойств аргументов
        const size_t MandatoryArguments;                // количество обязательных аргументов
        const bool Variadic = false;                    // признак вариадик-функции

        // копируем атрибуты аргументов, считаем количество обязательных аргументов,
        // определяем является ли функция вариадиком по флагу последнего аргумента
        FunctionInfo(std::initializer_list<Argument> arg) : Args(arg),
                                                            MandatoryArguments(std::count_if(Args.begin(), Args.end(), [](const auto& arg) { return !arg.Optional; })),
                                                            Variadic(Args.size() > 0 && Args.back().OptionalList)
        { }
    };

    struct ArgumentsToChildren
    {
        const FunctionInfo::Argument& argument;
        CASTNodeBase* pChild;
    };

    using ArgumentProps = std::vector<ArgumentsToChildren>;

    CASTFunctionBase(CASTTreeBase* Tree,
                     CASTNodeBase* Parent,
                     CASTFunctionBase* CloneOrigin) : CASTFunctionBase(Tree, Parent, CloneOrigin->GetText())
    {
        ConstantArguments = CloneOrigin->ConstantArguments;
    }

    // возвращает аргумент функции, отмеченный как константный по индексу
    CASTNodeBase* GetConstantArgument(size_t Index)
    {
        CASTNodeBase* pArg = nullptr;
        // ищем индекс в списке константных аргументов
        auto it = std::find_if(ConstantArguments.begin(), ConstantArguments.end(), [&Index](const auto& p)
            {
                return p.second == Index;
            });

        // если нашли, проверяем, является ли 
        // найденное уравнением
        if (it != ConstantArguments.end())
        {
            pArg = it->first;
            if (!pArg->CheckType(ASTNodeType::Equation))
                EXCEPTIONMSG("Constant argument must point to equation");
            // проверяем что у уравнения два дочерних узла
            pArg->CheckChildCount();
            // и возвращаем второй
            pArg = pArg->ChildNodes().back();
        }


        return pArg;
    }

    std::list< std::pair<CASTNodeBase*, size_t>> ConstantArguments;
    ptrdiff_t Precedence() const override { return precedence; };
    ptrdiff_t DesignedChildrenCount() const override { return 0; }
    bool IsFunction() const override { return true; }

    void ToInfix() override
    {
        infix = GetText();
        infix += "(";
        ChildrenSplitByToInfix(",");
        // к инфиксу добавляем список
        // константных аргументов
        for (auto& c : ConstantArguments)
        {
            // константные аргументы организованы
            // в уравнения
            if (!c.first->CheckType(ASTNodeType::Equation))
                EXCEPTIONMSG("Constant arguments is not equation");
            c.first->CheckChildCount();
            infix += ",";
            // берем правый дочерний узел уравнения -
            // выражение для расчета константного аргумента
            infix += (c.first->ChildNodes().back())->Infix();
        }

        infix += ")";
    }
    virtual const FunctionInfo& GetFunctionInfo() const = 0;

    // возвращает вектор соответствия текущих дочерних узлов в порядке
    // соответствия атрибутам аргументов функции
    void GetArgumentProps(ArgumentProps& props) const
    {
        props.clear();
        auto& fi = GetFunctionInfo().Args;
        auto itchild = Children.begin();

        // рзамер вектора соответствия всегда равен размеру вектора атрибутов
        // если количество дочерних узлов не соответствует размеру вектора
        // атрибутов - возвращаем в паре с атрибутом nullptr
        for (auto& a : fi)
        {
            if (itchild != Children.end())
            {
                props.push_back({ a, *itchild });
                itchild++;
            }
            else
                props.push_back({a, nullptr});
        }
    }

    void OnDelete() override
    {
        for (auto& ca : ConstantArguments)
            ca.first->GetParent()->DeleteChild(ca.first);
        ConstantArguments.clear();
        CASTNodeBase::OnDelete();
    }
};

class CASTHostBlockBase : public CASTFunctionBase
{
protected:
    size_t HostBlockIndex = 0;
public:

    // информация о свойствах хост-блока
    struct HostBlockInfo
    {
        CASTFunctionBase::FunctionInfo functionInfo;          // свойства аргументов
        std::string PrimitiveCompilerName;                    // имя типа блока для вывода исходного теста
        size_t EquationsCount = 1;
    };

    VarInfoList Outputs;
    CASTHostBlockBase(CASTTreeBase* Tree,
                      CASTNodeBase* Parent, size_t Index) : CASTFunctionBase(Tree, Parent), HostBlockIndex(Index)
    {
        pTree->GetHostBlocks().insert(this);
    }

    // конструктор копирования
    CASTHostBlockBase(CASTTreeBase* Tree,
                      CASTNodeBase* Parent, 
                      CASTHostBlockBase*  CloneOrigin) : CASTFunctionBase(Tree, Parent, CloneOrigin)
    {
        HostBlockIndex = CloneOrigin->HostBlockIndex;
        pTree->GetHostBlocks().insert(this);
    }

    void OnDelete() override
    {
        if (!pTree->GetHostBlocks().erase(this))
            EXCEPTIONMSG("No host block in map");
        // меняем индексы всех блоков так, чтобы они не превосходили
        // размерности сета блоков в дереве. Для этого все индексы
        // большие удаляемого уменьшаем на единицу
        for (auto& h : pTree->GetHostBlocks())
            if (h->HostBlockIndex > HostBlockIndex)
                h->HostBlockIndex--;
        CASTFunctionBase::OnDelete();
    }

    virtual const HostBlockInfo& GetHostBlockInfo() const = 0;
    // хост блок является функцией, и возвращает свойства аргументов хост-блока
    // как будто это функция
    const FunctionInfo& GetFunctionInfo() const override { return GetHostBlockInfo().functionInfo; }
    ptrdiff_t DesignedChildrenCount() const override { return 0; }
    bool IsHostBlock() const override { return true; }
    size_t GetHostBlockIndex() const { return HostBlockIndex; }
};

class CASTEquation : public CASTNodeBase
{
    using CASTNodeBase::CASTNodeBase;
    static inline constexpr std::string_view static_text = "EQ";
    ASTEquationsSet& Equations;
    std::string sourceEquation;
    size_t sourceEquationLine = -1;
public:

    // результат проверки разрешимости уравнения
    enum class Resolvable
    {
        Yes,        // да, можно разрешить
        No,         // нет, нельзя разрешить (более 1 неразрешенной переменной)
        Already     // уже разрешено (все переменные разрешены)
    };

    CASTHostBlockBase* pHostBlock = nullptr;
    VarInfoSet Vars;
    VarInfoMap::const_iterator itResolvedBy;        // уравнение разрешено по переменной
    VarInfoMap::const_iterator itTieBrokenBy;       // уравнение разорвано в алгебраическом цикле по переменной
    VarInfoMap::const_iterator itInitVar;           // уравнение инициализирует переменную
    ASTNodeType GetType() const override { return ASTNodeType::Equation; }
    ptrdiff_t DesignedChildrenCount() const override { return 2; }
    const std::string_view GetText() const override { return CASTEquation::static_text; }

    CASTEquation(CASTTreeBase* Tree,
                 CASTNodeBase* Parent,
                 std::string_view Text = std::string_view("")) : CASTNodeBase(Tree, Parent, Text), 
                                                                 Equations(Tree->GetEquations()),
                                                                 itResolvedBy(pTree->GetVariables().end()),
                                                                 itTieBrokenBy(pTree->GetVariables().end())
    {
        // при создании уравнения
        // вносим его в сет уравнений дерева
        Equations.insert(this);
    }

    CASTEquation(CASTTreeBase* Tree,
                 CASTNodeBase* Parent,
                 CASTEquation* CloneOrigin) : CASTEquation(Tree, Parent, CloneOrigin->GetText())
    {

    }

    void SetSourceEquation(std::string_view SourceEquation, size_t SourceEquationLine)
    {
        sourceEquation = SourceEquation;
        sourceEquationLine = SourceEquationLine;
    }

    // полное описание уравнения (текущий инфикс + исходник)
    std::string GetEquationDescription()
    {
        std::ostringstream result;
        std::string strInfix = ctrim(GetInfix());
        if (!strInfix.empty())
            result << "\"" << strInfix << "\"";
        result << GetEquationSourceDescription();
        return result.str();
    }

    // описание исходника уравнения
    std::string GetEquationSourceDescription()
    {
        std::ostringstream result;
        std::string strSource = ctrim(GetSourceEquation());
        size_t line(GetSourceEquationLine());
        if (line >= 0 && !strSource.empty())
        {
            result << " Исходный текст: строка " << line;
            result << " фрагмент \"" << strSource << "\"";
        }
        return result.str();
    }

    std::string GetSourceEquation() const
    {
        return sourceEquation;
    }

    size_t GetSourceEquationLine() const
    {
        return sourceEquationLine;
    }
    
    void Flatten() override
    {
        // собираем все в левую часть только для уравнений в Main-секции
        if (GetParentOfType(ASTNodeType::Init)) return;
        if (!IsNumericZero(Children.back()))
        {
            CASTNodeBase* pRightHand = ExtractChild(Children.back());
            CASTNodeBase* pLeftHand = ExtractChild(Children.front());
            CASTNodeBase* pNewLeft = CreateChild<CASTSum>();
            pNewLeft->CreateChild(pLeftHand);
            pNewLeft->CreateChild<CASTUminus>()->CreateChild(pRightHand);
            CASTNodeBase* pNumericZero = CreateChild<CASTNumeric>("0");
        }
    }

    void OnDelete() override
    {
        // при удалении уравнения
        // удаляем его из сета уравнений дерева
        if (!Equations.erase(this))
            EXCEPTIONMSG("No Equation in map");
    }

    void ToInfix() override
    {
        infix.clear();
        ChildrenSplitByToInfix(" = ");
        infix.insert(infix.begin(), '\t');
    }

    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }

    Resolvable IsResolvable(VarInfoSet::iterator& itResolvingVariable);
    VarInfoSet::iterator Resolve();

};

class CASTfnSin : public CASTFunctionBase
{
    using CASTFunctionBase::CASTFunctionBase;
    static inline constexpr std::string_view static_text = "sin";
    static inline const FunctionInfo fi = { CASTFunctionBase::FunctionInfo::Any };
public:
    void FoldConstants() override
    {
        double arg(0.0);
        if (SingleNumericChildValue(this, arg))
            pParent->ReplaceChild<CASTNumeric>(this, std::sin(arg));
    }
    ASTNodeType GetType() const override { return ASTNodeType::FnSin; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnSin::static_text; }
    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent) override;
    const FunctionInfo& GetFunctionInfo() const override { return fi; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};


class CASTfnTime : public CASTFunctionBase
{
    using CASTFunctionBase::CASTFunctionBase;
    static inline constexpr std::string_view static_text = "CustomDeviceData.time";
    static inline const FunctionInfo fi = { };
public:
    ASTNodeType GetType() const override { return ASTNodeType::FnTime; }
    ptrdiff_t DesignedChildrenCount() const override { return 0; }
    const std::string_view GetText() const override { return CASTfnTime::static_text; }
    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent) override
    {
        pDerivativeParent->CreateChild<CASTNumeric>(0.0);
        return nullptr;
    }
    const FunctionInfo& GetFunctionInfo() const override { return fi; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};

class CASTfnCos : public CASTFunctionBase
{
    using CASTFunctionBase::CASTFunctionBase;
    static inline constexpr std::string_view static_text = "cos";
    static inline const FunctionInfo fi = { CASTFunctionBase::FunctionInfo::Any };
public:
    void FoldConstants() override
    {
        double arg(0.0);
        if (SingleNumericChildValue(this, arg))
            pParent->ReplaceChild<CASTNumeric>(this, std::cos(arg));
    }
    ASTNodeType GetType() const override { return ASTNodeType::FnCos; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnCos::static_text; }
    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent) override;
    const FunctionInfo& GetFunctionInfo() const override { return fi; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};


class CASTfnExp : public CASTFunctionBase
{
    using CASTFunctionBase::CASTFunctionBase;
    static inline constexpr std::string_view static_text = "exp";
    static inline const FunctionInfo fi = { CASTFunctionBase::FunctionInfo::Any };
public:
    void FoldConstants() override
    {
        double arg(0.0);
        if (SingleNumericChildValue(this, arg))
            pParent->ReplaceChild<CASTNumeric>(this, std::exp(arg));
    }
    ASTNodeType GetType() const override { return ASTNodeType::FnExp; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnExp::static_text; }
    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent) override;
    const FunctionInfo& GetFunctionInfo() const override { return fi; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};

class CASTfnSqrt : public CASTFunctionBase
{
    using CASTFunctionBase::CASTFunctionBase;
    static inline constexpr std::string_view static_text = "sqrt";
    static inline const FunctionInfo fi = { CASTFunctionBase::FunctionInfo::Any };
public:
    void FoldConstants() override;
    ASTNodeType GetType() const override { return ASTNodeType::FnSqrt; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnSqrt::static_text; }
    const FunctionInfo& GetFunctionInfo() const override { return fi; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};

class CASTfnAndOrBase : public CASTFunctionBase
{
    using CASTFunctionBase::CASTFunctionBase;
    static inline const FunctionInfo fi = {
                                                CASTFunctionBase::FunctionInfo::Any,
                                                CASTFunctionBase::FunctionInfo::Any,
                                                CASTFunctionBase::FunctionInfo::OptionalList
    };
public:
    void Flatten() override;
    ptrdiff_t DesignedChildrenCount() const override { return -1; } // переменное число аргументов
    const FunctionInfo& GetFunctionInfo() const override { return fi; }
    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent) override;
};

class CASTfnOr : public CASTfnAndOrBase
{
    using CASTfnAndOrBase::CASTfnAndOrBase;
    static inline constexpr std::string_view static_text = "Or";
public:
    void FoldConstants() override;
    ASTNodeType GetType() const override { return ASTNodeType::FnOr; }
    const std::string_view GetText() const override { return CASTfnOr::static_text; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};

class CASTfnAnd : public CASTfnAndOrBase
{
    using CASTfnAndOrBase::CASTfnAndOrBase;
    static inline constexpr std::string_view static_text = "And";
public:
    void FoldConstants() override;
    ASTNodeType GetType() const override { return ASTNodeType::FnAnd; }
    const std::string_view GetText() const override { return CASTfnAnd::static_text; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};


class CASTfnRelay : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = { 
                                                    { 
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly,
                                                    },
                                                    "PBT_RELAYDELAYLOGIC", 
                                                    1 
                                               };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "relay";
    ASTNodeType GetType() const override { return ASTNodeType::FnRelay; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnRelay::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override  { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override {  return CloneImpl(pParent, this); }
    void FoldConstants() override
    {
        CheckChildCount();
        // если выдержка времени равна нулю, и выход можно разрешить - то 
        // заменяем на константу
        if (IsNumeric(Children.front()))
        {
            CASTNodeBase* pDelay = GetConstantArgument(2);
            if (pDelay && IsNumericZero(pDelay))
            {
                CASTNodeBase* pTresh = GetConstantArgument(2);
                if (pTresh && IsNumeric(pTresh))
                    pParent->ReplaceChild<CASTNumeric>(this, NumericValue(Children.front()) > NumericValue(pTresh));
            }
        }
    }
};

class CASTfnRelayD : public CASTfnRelay
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly,
                                                    },
                                                    "PBT_RELAYMINDELAYLOGIC",
                                                    1
    };
public:
    using CASTfnRelay::CASTfnRelay;
    static inline constexpr std::string_view static_text = "relayd";
    ASTNodeType GetType() const override { return ASTNodeType::FnRelayD; }
    const std::string_view GetText() const override { return CASTfnRelayD::static_text; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};

class CASTHigher : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::VariableOnly
                                                    },
                                                    "PBT_HIGHER",
                                                    1
                                               };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "higher";
    ASTNodeType GetType() const override { return ASTNodeType::FnHigher; }
    ptrdiff_t DesignedChildrenCount() const override { return 2; }
    const std::string_view GetText() const override { return CASTHigher::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override
    {
        CheckChildCount();
        CASTNodeBase* pLeft  = Children.front();
        CASTNodeBase* pRight = Children.back();

        // если аргументы - числа - выполняем сравнение и заменяем на результа
        // если аргументы одинаковые - заменяем на ноль

        if(IsNumeric(pLeft) && IsNumeric(pRight))
            pParent->ReplaceChild<CASTNumeric>(this, NumericValue(pLeft) > NumericValue(pRight) ? 1.0 : 0.0);
        else if(pLeft->Compare(pRight) == 0)
            pParent->ReplaceChild<CASTNumeric>(this, 0.0);

        // еще можно посокращать одинаковые выражения слева и справа и сравнить остатки, если они останутся
        // числами или одинаковыми
    }
};

class CASTLower : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::VariableOnly
                                                    },
                                                    "PBT_LOWER",
                                                    1
                                               };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "lower";
    ASTNodeType GetType() const override { return ASTNodeType::FnLower; }
    ptrdiff_t DesignedChildrenCount() const override { return 2; }
    const std::string_view GetText() const override { return CASTLower::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }

    void FoldConstants() override
    {
        CheckChildCount();

        CASTNodeBase* pLeft = Children.front();
        CASTNodeBase* pRight = Children.back();

        // если аргументы - числа - выполняем сравнение и заменяем на результа
        // если аргументы одинаковые - заменяем на ноль

        if (IsNumeric(pLeft) && IsNumeric(pRight))
            pParent->ReplaceChild<CASTNumeric>(this, NumericValue(pLeft) < NumericValue(pRight) ? 1.0 : 0.0);
        else
        if (pLeft->Compare(pRight) == 0)
            pParent->ReplaceChild<CASTNumeric>(this, 0.0);
    }
};

class CASTfnAbs : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly
                                                    },
                                                    "PBT_ABS",
                                                    1
                                               };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "abs";
    ASTNodeType GetType() const override { return ASTNodeType::FnAbs; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnAbs::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override
    {
        double arg(0.0);
        if (SingleNumericChildValue(this, arg))
            pParent->ReplaceChild<CASTNumeric>(this, std::abs(arg));
    }

    void Flatten() override
    {
        // удаляем унарный минус 
        CheckChildCount();
        CASTNodeBase* pChild(Children.front());
        if (pChild->CheckType(ASTNodeType::Uminus))
        {
            pChild->CheckChildCount();
            CASTNodeBase* pMinusArg = pChild->ExtractChild(pChild->ChildNodes().begin());
            ReplaceChild(pChild, pMinusArg);
        }
    }
};


class CASTfnLagLim : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly
                                                    },
                                                    "PBT_LIMITEDLAG",
                                                    1
    };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "laglim";
    ASTNodeType GetType() const override { return ASTNodeType::FnLagLim; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnLagLim::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override;
};

class CASTfnLag : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly
                                                    },
                                                    "PBT_LAG",
                                                    1
    };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "lag";
    ASTNodeType GetType() const override { return ASTNodeType::FnLag; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnLag::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override;
};


class CASTfnDerLag : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly
                                                    },
                                                    "PBT_DERLAG",
                                                    1
    };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "derlag";
    ASTNodeType GetType() const override { return ASTNodeType::FnDerLag; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnDerLag::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override;
};


class CASTfnExpand : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly
                                                    },
                                                    "PBT_EXPAND",
                                                    1
    };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "expand";
    ASTNodeType GetType() const override { return ASTNodeType::FnExpand; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnExpand::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override;
    static void FoldConstantsExpandShrink(CASTFunctionBase* pNodeShrinkExpand);
};


class CASTfnShrink: public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly
                                                    },
                                                    "PBT_SHRINK",
                                                    1
    };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "shrink";
    ASTNodeType GetType() const override { return ASTNodeType::FnShrink; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnShrink::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override;
};

class CASTfnDeadBand : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly
                                                    },
                                                    "PBT_DEADBAND",
                                                    1
    };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "deadband";
    ASTNodeType GetType() const override { return ASTNodeType::FnDeadBand; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnDeadBand::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override;
};

class CASTfnLimit : public CASTHostBlockBase
{
    static inline const HostBlockInfo hbInfo = {
                                                    {
                                                        CASTFunctionBase::FunctionInfo::VariableOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly,
                                                        CASTFunctionBase::FunctionInfo::ConstantOnly
                                                    },
                                                    "PBT_LIMITERCONST",
                                                    1
    };
public:
    using CASTHostBlockBase::CASTHostBlockBase;
    static inline constexpr std::string_view static_text = "limit";
    ASTNodeType GetType() const override { return ASTNodeType::FnLimit; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnLimit::static_text; }
    const HostBlockInfo& GetHostBlockInfo() const override { return hbInfo; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    void FoldConstants() override;
};

// в зависимости от того, как наследована степень, она будет функцией pow(x,y) или инфиксным оператором ^
template<class T>
class CASTPowT : public T
{
    using T::T;
};

using CASTPowBase = CASTPowT<CASTInfixBase>;
//using CASTPowBase = CASTPowT<CASTFunctionBase>;

class CASTPow : public CASTPowBase
{
    using CASTPowBase::CASTPowBase;
    static inline constexpr std::string_view static_text = std::is_base_of<CASTFunctionBase, CASTPowBase>::value ? "pow" : "^";
public:
    ASTNodeType GetType() const override { return ASTNodeType::Pow; }
    ptrdiff_t DesignedChildrenCount() const override { return 2; }
    bool RightAssociativity() const override { return true; }
    static inline const ptrdiff_t precedence = CASTUminus::precedence + 1;
    ptrdiff_t Precedence() const override { return precedence; };
    const std::string_view GetText() const override { return CASTPow::static_text; }

    // выводим для с++ всегда в форме функции pow(x,y)
    void ToInfix() override
    {
        infix = "std::pow(";
        ChildrenSplitByToInfix(",");
        infix += ")";
    }

    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
    double SortPower() const override
    {
        if (IsNumeric(Children.back()))
            return NumericValue(Children.back());
        else
            return CASTPowBase::SortPower();
    }
    void Flatten() override;
    void FoldConstants() override
    {
        if (IsNumeric(Children.back()))
        {
            double exp(NumericValue(Children.back()));
            if (exp == 0.0)
            {
                if (IsNumericZero(Children.front()))
                    Warning("Ноль в степени ноль, принято значение 1.0");
                pParent->ReplaceChild<CASTNumeric>(this, 1.0);
            }
            else if (exp == 1.0)
                pParent->ReplaceChild(this, ExtractChild(Children.begin()));
            else if (IsNumeric(Children.front()))
                pParent->ReplaceChild<CASTNumeric>(this, std::pow(NumericValue(Children.front()), exp));
        }
    }

    CASTNodeBase* GetDerivative(VarInfoMap::iterator itVar, CASTNodeBase* pDerivativeParent) override;
};

class CASTJacobiElement : public CASTNodeBase
{
public:
    VarInfoMap::iterator var;
    CASTEquation* pequation = nullptr;
    CASTJacobiElement(CASTTreeBase* Tree, 
                      CASTNodeBase* Parent,
                      CASTEquation* pEquation, 
                      VarInfoMap::iterator itVarMap) : CASTNodeBase(Tree, Parent),
                                                       pequation(pEquation),
                                                       var(itVarMap)
    {

    }

    ASTNodeType GetType() const override { return ASTNodeType::JacobiElement; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    ptrdiff_t Precedence() const override { return 0; };
    void ToInfix() override
    {
        infix = fmt::format("\td({})/d({}) = {}", 
                    ctrim(pequation->GetInfix()),
                    var->first.c_str(),
                    Children.front()->Infix());
    }
};

// Для функций типа starter и action нам нужно связать переменную V, которая
// используется в формуле, и ссылку на внешнюю переменную в формате modellink, которая
// задается в атрибутах стартера и экшена.
// Для того чтобы ввести такую конструкцию в систему уравнений мы используем прокси-функцию
// в виде S1 = starter(Expression, Modellink). В PostParseCheck мы заменяем переменные
// V и BASE в Expression на Modellink, и заменяем starter на Expression. Таким образом
// мы формируем полноценное уравнение из формата стартера и экшена.

class CASTfnProxyVariable : public CASTFunctionBase
{
    using CASTFunctionBase::CASTFunctionBase;
    static inline constexpr std::string_view static_text = "starter";
    static inline const FunctionInfo fi = { 
                                             CASTFunctionBase::FunctionInfo::Any,
                                             CASTFunctionBase::FunctionInfo::Optional
                                          };
public:
    ASTNodeType GetType() const override { return ASTNodeType::FnProxyVariable; }
    ptrdiff_t DesignedChildrenCount() const override { return 1; }
    const std::string_view GetText() const override { return CASTfnProxyVariable::static_text; }
    const FunctionInfo& GetFunctionInfo() const override { return fi; }
    CASTNodeBase* Clone(CASTNodeBase* pParent) override { return CloneImpl(pParent, this); }
};
