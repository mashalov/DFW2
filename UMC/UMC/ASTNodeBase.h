#pragma once
#include "ASTTreeBase.h"
#include "ASTNodeTypes.h"

class CASTNodeBase
{
protected:
    CASTNodeBase    *pParent;       // родительский узел
    ASTNodeList     Children;       // список дочерних узлов
    CASTTreeBase    *pTree;         // указатель на родительское дерево
    std::string infix;              // строка инфиксной записи узла
    std::string Id;                 // идентификатор узла
    static inline const ptrdiff_t precedence = 0;
    static inline constexpr std::string_view static_text = "";  // заглушка общая для всех узлов - для возврата в GetText()
    bool bDeleted = false;          // признак удаления узла
    double sortPower = 1.0;         // текущая степень узла
    size_t score = 0;               // скоринг узла
    bool Constant = false;          // флаг константного выражения в узле
    bool IsError = false;           // флаг ошибки в узле
public:
    CASTNodeBase(CASTTreeBase* Tree, 
                 CASTNodeBase* Parent, 
                 std::string_view Text = std::string_view("")) : pTree(Tree), 
                                                                 pParent(Parent)
    {

    }

    // конструктор копирования
    CASTNodeBase(CASTTreeBase* Tree,
                CASTNodeBase* Parent,
                CASTNodeBase* CloneOrigin) : CASTNodeBase(Tree, Parent, CloneOrigin->GetText())
    {

    }

    virtual ~CASTNodeBase() {}
    virtual ASTNodeType GetType() const = 0;

    template<typename... Types>
    bool CheckType(ASTNodeType Type, Types... types) const
    {
        bool bRes = false;
        return CheckType(Type) | CheckType(types...);
    }

    bool CheckType(ASTNodeType type) const
    {
        return GetType() == type;
    }
    

    CASTNodeBase* GetParent() 
    { 
        return pParent; 
    }

    const CASTNodeBase* GetParent() const  
    { 
        return pParent; 
    }

    ASTNodeList& ChildNodes()
    {
        return Children;
    }

    const ASTNodeList& ChildNodes() const
    {
        return Children;
    }

    void CheckChildCount() const
    {
        if (Children.size() != DesignedChildrenCount())
            EXCEPTIONMSG("Wrong child count");
    }

    // клонирует узел в новый родительский узел
    // (исходное дерево)


    template<typename T>
    T* CloneImpl(CASTNodeBase* pParent, T* CloneOrigin)
    {
        return pParent->CreateClone<T>(CloneOrigin);
    }

    virtual CASTNodeBase* Clone(CASTNodeBase* pParent)
    {
        EXCEPTIONMSG("Function is not specified")
    }


    // клонирует дерево узла рекурсивно
    // в новый родительский узел

    CASTNodeBase* CloneTree(CASTNodeBase* pParent)
    {
        struct CloneStack
        {
            CASTNodeBase* pSourceNode;
            CASTNodeBase* pCloneNode;
        };
        std::stack<CloneStack> stack;
        ASTNodeConstSet visited;

        CASTNodeBase* pClone = nullptr;

        stack.push({ this, pParent });

        while (!stack.empty())
        {
            auto v = stack.top();

            if (v.pSourceNode->Deleted())
                EXCEPTIONMSG("Processing deleted clone node");

            stack.pop();
            if (!visited.Contains(v.pSourceNode))
            {
                visited.insert(v.pSourceNode);
                CASTNodeBase *pNewClone = v.pSourceNode->Clone(v.pCloneNode);
                if (!pClone)
                    pClone = pNewClone;
                for (auto c = v.pSourceNode->ChildNodes().rbegin() ; c != v.pSourceNode->ChildNodes().rend() ; c++)
                    stack.push({ *c, pNewClone });
            }
        }

        return pClone;
    }

    // при удалении узел физически не удаляется,
    // но помечается как удаленный
    bool Deleted() const 
    { 
        return bDeleted; 
    }

    // виртуальный метод удаления позволяет
    // обработать в потомке очистку ресурсов
    virtual void OnDelete()
    {

    }
   
    void Delete() 
    { 
        if (!Deleted())
        {
            OnDelete();
            bDeleted = true;
        }
    }

    // количество дочерних узлов данного типа узла для проверки корректности преобразований
    // если 0 - то не проверяется
    virtual ptrdiff_t DesignedChildrenCount() const = 0;

    // приоритет операции данного узла
    virtual ptrdiff_t Precedence() const
    {
        return precedence;
    }


    // скоринг данного типа узла 
    virtual size_t NodeTypeScore() const
    {
        return 1;
    }
    // возвращает true, если узел является
    // константой (можно перекрыть, для того
    // чтобы задать конкретное условие - например 
    // константная переменная
    virtual bool IsConst()
    {
        return Constant;
    }

    virtual void SetConst(bool bConst)
    {
        if (Constant && !bConst)
            EXCEPTIONMSG("Attempt to make const node non const");
        Constant = bConst;
    }

    // расчет скоринга узла и обновление статуса константы
    size_t UpdateScore()
    {
        // скоринг состоит и скоринга самого узла
        // и суммы скорингов дочерних узлов
        score = NodeTypeScore();
        for (auto&& c : Children)
            score += NodeTypeScore() * c->Score();

        // constant propagation

        // если исходное состояние узла - константа - пропускаем. Он не может
        // стать неконстантным
        if (!IsConst())     
        {
            // если у узла нет дочерних - его состояние может измениться
            // только напрямую через SetConst
            if (Children.size() > 0)
            {
                bool cst(true);
                // перебираем все дочерние узлы
                // если все они константы - ставим для данного узла
                // флаг константы
                for (auto&& c : Children)
                    cst &= c->IsConst();
                SetConst(cst);
            }
        }
        return score;
    }

    // возвращает текущий скоринг экземпляра узла
    size_t Score() const
    {
        return score;
    }

    // атрибут правоассоциативности узла - например - ^
    virtual bool RightAssociativity() const
    {
        return false;
    }

    // "полное" сравнение двух узлов
    ptrdiff_t Compare(const CASTNodeBase* pNode) const;

    // добавляет готовый дочерний узел 
    template< typename T = CASTNodeBase>
    CASTNodeBase* CreateChild(T* pChild)
    {
        Children.push_back(pChild);
        pChild->pParent = this;
        pTree->Change();
        return pChild;
    }


    template< typename T = CASTNodeBase, typename... Args>
    T* CreateChildImpl(Args... args)
    {
        T* pNewNode(pTree->CreateNode<T>(this, args...));
        Children.push_back(pNewNode);
        return pNewNode;
    }

    template<typename T = CASTNodeBase, typename ...Args>
    T* CreateChild(Args... args)
    {
        return CreateChildImpl<T>(args...);
    }
    
    const std::string_view GetId() const
    {
        return Id;
    }

    virtual const std::string_view GetText() const
    {
        return CASTNodeBase::static_text;
    }

    virtual void SetText(std::string_view Text)
    {
        EXCEPTIONMSG("No SetText functionality in CASTNodeBase");
    }

    void SetText(double Value)
    {
        SetText(to_string(Value));
    }

    // возвращает итератор на дочерний узел по указателю
    ASTNodeList::iterator TryFindChild(const CASTNodeBase* pNode)
    {
        return std::find_if(Children.begin(), Children.end(), [&pNode](const CASTNodeBase* p) { return p == pNode; });
    }

    // возвращает true, если заданный узел является дочерним
    bool IsChild(const CASTNodeBase* pNode)
    {
        return TryFindChild(pNode) != Children.end();
    }

    bool IsChildVariable(const std::string_view VarName)
    {
        return std::find_if(Children.begin(), Children.end(), [&VarName](const CASTNodeBase* p)
            {
                return IsVariable(p) && p->GetText() == VarName;
            }) != Children.end();
    }

    // возвращает итератор на дочерний узел по указателю, если не найден - выдает исключение
    ASTNodeList::iterator FindChild(const CASTNodeBase* pNode)
    {
        auto cit = TryFindChild(pNode);
        if (cit == Children.end())
            EXCEPTIONMSG("Child not found");
        return cit;
    }

    virtual bool IsFunction() const { return false; }
    virtual bool IsHostBlock() const { return false; }


    // добавляет готовые дочерние узлы из списка
    void AppendChildren(ASTNodeList& newChildren)
    {
        for (auto& nc : newChildren)
        {
            nc->pParent = this;
            Children.push_back(nc);
            pTree->Change();
        }
    }

    // извлекает все дочерние узлы
    void ExtractChildren(ASTNodeList& extractedChildren)
    {
        extractedChildren.clear();
        extractedChildren.insert(extractedChildren.end(), Children.begin(), Children.end());
        if(!Children.empty())
            pTree->Change();
        Children.clear();
    }

    // извлекает дочерний узел по указателю
    CASTNodeBase* ExtractChild(CASTNodeBase* pNode)
    {
        return ExtractChild(FindChild(pNode));
    }

    // извлекает дочерний узел по итератору
    CASTNodeBase* ExtractChild(ASTNodeList::iterator itChild)
    {
        CASTNodeBase* pNode(nullptr);
        if (itChild != Children.end())
        {
            pNode = *itChild;
            Children.erase(itChild);
            pTree->Change();
        }

        if(!pNode)
            EXCEPTIONMSG("Node has not been extracted")

        return pNode;
    }

    // заменяет дочерний узел по итератору (с удалением) на готовый дочерний узел
    CASTNodeBase* ReplaceChild(ASTNodeList::iterator itChild, CASTNodeBase* pReplaceTo)
    {
        if (itChild != Children.end())
        {
            auto pToDelete = *itChild;
            *itChild = pReplaceTo;
            pReplaceTo->pParent = this;
            pTree->DeleteNode(pToDelete);
        }
        else
            pReplaceTo = nullptr;

        if (!pReplaceTo)
            EXCEPTIONMSG("Node has not been replaced");
        return pReplaceTo;
    }

    // заменяет дочерний узел по указателю (с удалением) на готовый дочерний узел
    CASTNodeBase* ReplaceChild(CASTNodeBase* pToReplace, CASTNodeBase* pReplaceTo)
    {
        return ReplaceChild(FindChild(pToReplace), pReplaceTo);
    }

    // заменяет дочерний узел по указателю (с удалением) на новый дочерний узел с double значением
    template< typename T = CASTNodeBase>
    T* ReplaceChild(CASTNodeBase* pToReplace, double Value)
    {
        return ReplaceChild<T>(pToReplace, to_string(Value));
    }

    // заменяет дочерний узел по итератору (с удалением) на новый дочерний узел с double значением
    template< typename T = CASTNodeBase>
    T* ReplaceChild(ASTNodeList::iterator itChild, double Value)
    {
        return ReplaceChild<T>(itChild, to_string(Value));
    }

    // заменяет дочерний узел по итератору (с удалением) на новый дочерний узел с текстовым значением
    template< typename T = CASTNodeBase>
    T* ReplaceChild(ASTNodeList::iterator itChild, std::string_view Text)
    {
        T* pReplaceTo(nullptr);
        if (itChild != Children.end())
        {
            auto pToDelete = *itChild;
            *itChild = pReplaceTo = pTree->CreateNode<T>(this, Text);
            pTree->DeleteNode(pToDelete);
        }
        if (!pReplaceTo)
            EXCEPTIONMSG("Node has not been replaced");

        return pReplaceTo;
    }
    // заменяет дочерний узел по итератору (с удалением) на новый дочерний узел
    template< typename T = CASTNodeBase>
    T* ReplaceChild(ASTNodeList::iterator itChild)
    {
        T* pReplaceTo(nullptr);
        if (itChild != Children.end())
        {
            auto pToDelete = *itChild;
            *itChild = pReplaceTo = pTree->CreateNode<T>(this);
            pTree->DeleteNode(pToDelete);
        }
        if (!pReplaceTo)
            EXCEPTIONMSG("Node has not been replaced");
        return pReplaceTo;
    }
    
    // заменяет дочерний узел по указателю на новый дочерний узел с текстовым значением
    template< typename T = CASTNodeBase>
    T* ReplaceChild(CASTNodeBase* pToReplace, std::string_view Text)
    {
        return ReplaceChild<T>(FindChild(pToReplace), Text);
    }

    // заменяет дочерний узел по указателю (с удалением) на новый
    template< typename T = CASTNodeBase>
    T* ReplaceChild(CASTNodeBase* pToReplace)
    {
        return ReplaceChild<T>(FindChild(pToReplace));
    }

    // вставляет новый промежуточный дочерний узел
    template<typename T = CASTNodeBase>
    T* InsertItermdiateChild(CASTNodeBase* pToReplace)
    {
        T* pInsertTo(nullptr);
        // находим узел, перед которым нужно вставить новый
        auto cit = FindChild(pToReplace);
        if (cit != Children.end())
        {
            // если нашли создаем узел заданного типа
            pInsertTo = pTree->CreateNode<T>(this);
            // меняем найденный дочерний узел на новый
            *cit = pInsertTo;
            // к новому удочеряем найденный узел
            pInsertTo->CreateChild(pToReplace);
        }
        if (!pInsertTo)
            EXCEPTIONMSG("Node has not been inserted");
        return pInsertTo;
    }

    // свертка констатнт
    virtual void FoldConstants() {}
    // сборка плоских операторов и преобразования минуса, унарного минуса и деления
    virtual void Flatten() {}
    // возвращает максимальную степень в узле
    double MaxSortPower();
    // возвращает текущую степень в узле
    virtual double SortPower() const { return sortPower; }

    // удаляет дочерний узел по итератору
    ASTNodeList::iterator DeleteChild(ASTNodeList::iterator Child)
    {
        if (Child == Children.end())
            EXCEPTIONMSG("Wrong child iterator");
        auto* pToDelete = *Child;
        auto nextChild = Children.erase(Child);
        pTree->DeleteNode(pToDelete);
        return nextChild;
    }

    // удаляет дочерний узел по указателю
    void DeleteChild(CASTNodeBase* pChild)
    {
        DeleteChild(FindChild(pChild));
    }

    void PrintTree(std::string indent, bool last) const;

    template<typename T>
    T* CreateClone(T* CloneOrigin)
    {
        T* pNewNode(pTree->CloneNode<T>(this, CloneOrigin));
        Children.push_back(pNewNode);
        return pNewNode;
    }



    // возвращает ближайший родительский узел данного типа
    CASTNodeBase* GetParentOfType(const ASTNodeType ParentType);

    // возвращает родительское уравнение
    CASTEquation* GetParentEquation();
    
    // функции вывода информации о предупреждениях и ошибках
    // по умолчанию выводится обрабатываемое выражение и данные об исходном
    // тексте (если есть). При необходимости можно отказаться от вывода
    // обрабатываемого выражения в случае, если его еще нет или оно
    // отличается от исходника (например - выражение операторного дерева 
    // или функция с удаленными константными аргументами)

    void Warning(std::string_view warning, bool SourceOnly = false);
    void Error(std::string_view error, bool SourceOnly = false);
    std::string ParentEquationDescription();
    std::string ParentEquationSourceDescription();

    // возвращает текущую инфиксную запись
    virtual const std::string& Infix() const
    {
        return infix;
    }
    
    // создает инфиксную запись
    virtual void ToInfix() = 0;

    // генерирует инфиксную запись данного узла с поддеревом
    std::string& GetInfix()
    {
        pTree->DFSPost(this, [](CASTNodeBase* pNode)
            {
                pNode->ToInfix();
                return true;
            }
        );

        return infix;
    }

    // возвращает список повторяющихся в зле фрагментов
    void Fragments(ASTFragmentsList& fragments)
    {
        // после сортировки извлекает инфиксные фрагменты узлов
        // и строит карту с инфиксов к узлам. Позволяет определить
        // повторяющиеся узлы и количество повторов
        // для выбора узлов для обработки можно придумать функцию оценки
        // (например - по глубине дерева фрагмента или по сложности вычислений)
        ASTFragmentsMap fragmentsmap;
        fragmentsmap.clear();
        pTree->DFSPost(this, [&fragmentsmap](CASTNodeBase* p)
            {
                p->FoldConstants();
                if(!p->Deleted())
                    fragmentsmap[p->GetInfix()].insert(p);
                return true;
            }
        );
        // удаляем фрагменты, которые встретились менее двух раз
        pTree->RemoveSingleFragments(fragmentsmap);
        fragments.clear();
        // создаем список фрагментов, который можно отсортировать
        for (auto&& c : fragmentsmap)
            fragments.push_back({ c.first, c.second });

        // сортируем фрагменты по убыванию скоринга (сложности вычисления фрагментов умноженного на их количество)
        fragments.sort([](const auto& lhs, const auto& rhs)
            {
                return lhs.score > rhs.score;
            }
        );
    }

    // возвращает true если в дереве узла есть переменная
    // с заданным именем
    bool FindVariable(CASTNodeBase* pNode, std::string_view Text)
    {
        bool bFound(false);
        pTree->DFS(pNode, [&Text, &bFound](const CASTNodeBase* p)
            {
                if (IsVariable(p) && p->GetText() == Text)
                    bFound = true;
                // лябмда прерывает обход дерева если находит нужную переменную
                return !bFound;
            });
        return bFound;
    }

    // возвращает узел, относительно которого
    // идет дифференцирование
    CASTNodeBase* GetDerivativeContext()
    {
        // по умолчанию узел контекста - родительский узел
        CASTNodeBase* pParent(this);
        // но если это узел унарного минуса
        // ищем более ранний узел в дереве
        while (pParent)
        {
            if (!IsUminus(pParent))
                break;
            pParent = pParent->pParent;
        }
        return pParent;
    }

    // возвращает дерево производной для данного узла
    // по заданной переменной в заданном родительском узле

    virtual CASTNodeBase* GetDerivative(VarInfoMap::iterator itVariable, CASTNodeBase* pDerivativeParent)
    {
        EXCEPTIONMSG("No function specified");
    }

    void PrintInfix()
    {
        std::cout << GetInfix() << std::endl;
    }

    std::string& ChildrenSplitByToInfix(std::string_view Delimiter);

    static bool IsHostBlock(const CASTNodeBase* pNode)
    {
        return !pNode->Deleted() && pNode->IsHostBlock();
    }

    static bool IsFunction(const CASTNodeBase* pNode)
    {
        return !pNode->Deleted() && pNode->IsFunction();
    }

    static bool IsNumeric(const CASTNodeBase* pNode)
    {
        return !pNode->Deleted() && pNode->CheckType(ASTNodeType::Numeric);
    }

    static bool IsVariable(const CASTNodeBase* pNode)
    {
        return !pNode->Deleted() && pNode->CheckType(ASTNodeType::Variable);
    }

    static bool IsNumericZero(const CASTNodeBase* pNode)
    {
        return (IsNumeric(pNode) && NumericValue(pNode) == 0.0);
    }

    static bool IsNumericUnity(const CASTNodeBase* pNode)
    {
        return (IsNumeric(pNode) && NumericValue(pNode) == 1.0);
    }

    static double NumericValue(const CASTNodeBase* pNode)
    {
        if (!IsNumeric(pNode))
            EXCEPTIONMSG("Node is not numeric");
        return std::stod(std::string(pNode->GetText()));
    }

    static bool IsFlatOperator(const CASTNodeBase* pNode)
    {
        switch (pNode->GetType())
        {
        case ASTNodeType::Mul:
        case ASTNodeType::Sum:
            return true;
        }
        return false;
    }

    static bool IsUminus(const CASTNodeBase* pNode)
    {
        return pNode->CheckType(ASTNodeType::Uminus);
    }

    static bool IsSum(const CASTNodeBase* pNode)
    {
        return pNode->CheckType(ASTNodeType::Sum);
    }

    static bool TrasformNodeToOperator(CASTNodeBase* pNode);

    static bool IsTransforming(const CASTNodeBase* pNode)
    {
        if (IsFunction(pNode))
            return true;
        if (IsOperator(pNode) && !pNode->CheckType(ASTNodeType::Equation))
            return true;

        return false;
    }

    static bool IsOperator(const CASTNodeBase* pNode)
    {
        switch (pNode->GetType())
        {
            case ASTNodeType::Mul:
            case ASTNodeType::Sum:
            case ASTNodeType::Equation:
            case ASTNodeType::Uminus:
            case ASTNodeType::Pow:
                return true;
        }
        return false;
    }

    static bool SingleNumericChildValue(const CASTNodeBase* pNode, double& dValue)
    {
        if (pNode->ChildNodes().size() == 1 && IsNumeric(pNode->ChildNodes().front()))
        {
            dValue = std::stod(std::string(pNode->ChildNodes().front()->GetText()));
            return true;
        }
        else
            return false;
    }

    static bool IsLeaf(const CASTNodeBase* pNode)
    {
        switch (pNode->GetType())
        {
            case ASTNodeType::Numeric:
            case ASTNodeType::Variable:
            case ASTNodeType::Any:
                return true;
        }
        return false;
    }

    static inline const char *szErrorParamterMustBeNonNegative = u8"Неправильный параметр функции {} : параметр должен быть неотрицательным {}";
    static inline const char* szErrorLimitsWrong = u8"Неправильные ограничения функции {} : Min>Max ({}>{})";
};

class CASTNodeTextBase : public CASTNodeBase
{
protected:
    std::string text;
public:
    using CASTNodeBase::CASTNodeBase;
    CASTNodeTextBase(CASTTreeBase* Tree,
                     CASTNodeBase* Parent,
                     std::string_view Text) : CASTNodeBase(Tree,Parent),
                                              text(Text)
    {
    }

    const std::string_view GetText() const override
    {
        return text;
    }

    void OnDelete() override
    {
        CASTNodeBase::OnDelete();
    }

    void SetText(std::string_view Text)
    {
        text = Text;
    }
};