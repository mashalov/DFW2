﻿/*
* UMC - модуль оптимизируюещего компилятора моделей оборудования для проекта Raiden
* (С) 2018 - Н.В. Евгений Машалов
* Свидетельство о государственной регистрации программы для ЭВМ №2021663231
* https://fips.ru/EGD/c7c3d928-893a-4f47-a218-51f3c5686860
*
* UMC - user model compiler for Raiden project
* (C) 2018 - present Eugene Mashalov
* pat. RU №2021663231
*
* Реализует метод трансляции описания модели
* https://www.elibrary.ru/item.asp?id=44384350 (C) 2020 Евгений Машалов
*
*/

#include <numeric>
#include "ASTTreeBase.h"
#include "ASTNodes.h"
#include "MatchInfo.h"
#include "ASTSimpleEquation.h"


ASTFragmentInfo::ASTFragmentInfo(std::string_view Infix, ASTNodeSet& FragmentSet) : infix(Infix), fragmentset(FragmentSet), score(0)
{
    for (auto&& c : fragmentset)
        score += c->Score();
}


CASTTreeBase::CASTTreeBase(PropertyMap& Properties) : properties(Properties)
{
    _ASSERTE(CASTSum(this, nullptr).Precedence() < CASTMul(this, nullptr).Precedence());
    _ASSERTE(CASTMul(this, nullptr).Precedence() < CASTUminus(this, nullptr).Precedence());
    _ASSERTE(CASTUminus(this, nullptr).Precedence() < CASTPow(this, nullptr).Precedence());
}

CASTTreeBase::~CASTTreeBase()
{
	for(auto&& node : NodeList)
		delete node;
}

//! Сбрасывает маркер визитов в узлах
/*!
    После обхода дерева с процессингом в обрабатываемых
    узлах не сбрасывались маркеры. Поэтому эта функция 
    перечисляет все узлы дерева и ставит заданный маркер визита DFSLevel_
*/
void CASTTreeBase::Unvisit(CASTNodeBase* Root)
{
    // ставим для посещенных узлов максимально возможный маркер

    const ptrdiff_t DFSLevelMarker{ std::numeric_limits<ptrdiff_t>::max() };

    // резервируем стек и список посещенных узлов
    CDFSStack stack(NodeList.size());
    ASTNodeVec visited;
    visited.reserve(NodeList.size());

    stack.push(Root);

    while (!stack.empty())
    {
        auto v{ stack.top() };
        stack.pop();
        if (!v->Visited(DFSLevelMarker))
        {
            v->Visit(DFSLevelMarker);
            visited.push_back(v);
            for (auto c : v->ChildNodes())
                stack.push(c);
        }
    }

    // для перечисленных узлов ставим текущее значения маркера визита
    for (auto&& node : visited)
        node->Visit(DFSLevel_);
}

void CASTTreeBase::DFS(CASTNodeBase* Root, DFSVisitorFunction Visitor)
{
    // увеличиваем текущий номер обхода
    DFSLevel_++;

    // резервируем стек и список посещенных узлов
    CDFSStack stack(NodeList.size() * 2);
    
    stack.push(Root);

    while (!stack.empty())
    {
        auto v = stack.top();

        if (v->Deleted())
            EXCEPTIONMSG("Processing deleted node");

        stack.pop();
        if (!v->Visited(DFSLevel_))
        {
            v->Visit(DFSLevel_);
            if (!(Visitor)(v))
                break;
            for (auto c : v->ChildNodes())
                stack.push(c);
        }
    }

    // Возвращаем номер обхода
    DFSLevel_--;
    // и восстанавливаем исходные посещения у посещенных в данном обходе узлов
    Unvisit(Root);
}

void CASTTreeBase::DFSPost(CASTNodeBase* Root, DFSVisitorFunction Visitor)
{
    // увеличиваем текущий номер обхода
    DFSLevel_++;

    // резервируем стек и список посещенных узлов
    CDFSStack stack(NodeList.size() * 2);

    stack.push(Root);

    while (!stack.empty())
    {
        auto v = stack.top();

        if (v->Deleted())
            EXCEPTIONMSG("Processing deleted node");

        if (!v->Visited(DFSLevel_))
        {
            v->Visit(DFSLevel_);
            for (auto c : v->ChildNodes())
                stack.push(c);
        }
        else
        {
            stack.pop();
            if (v->DesignedChildrenCount() >= 0 &&
                v->ChildNodes().size() != v->DesignedChildrenCount())
                EXCEPTIONMSG("Designed child count mismatch");
            if (!(Visitor)(v))
                break;
        }
    }

    // Возвращаем номер обхода
    DFSLevel_--;
    // и восстанавливаем исходные посещения у посещенных в данном обходе узлов
    Unvisit(Root);
}


void CASTTreeBase::DeleteNode(CASTNodeBase* pNode)
{
    ASTNodeConstSet nodesToDelete;
    DFS(pNode, [&nodesToDelete](CASTNodeBase* p) { nodesToDelete.insert(p); p->Delete(); return true; });


    if (nodesToDelete.empty())
        Change();

    for(const auto& node : NodeList)
        if(!node->Deleted() && (nodesToDelete.Contains(node) || nodesToDelete.Contains(node->GetParent())))
            EXCEPTIONMSG("Node or its parent has been already deleted");

    //DFS(pRoot, [&nodesToDelete](CASTNodeBase* p) 
    //    { 
    //        if (nodesToDelete.Contains(p) || nodesToDelete.Contains(p->GetParent()))
    //            EXCEPTIONMSG("Node or its parent has been already deleted");
    //        return true;
    //    });
}

void CASTTreeBase::Flatten()
{
    do
    {
        Unchange();
        DFSPost(pRoot, [](CASTNodeBase* p) 
            { 
                p->Flatten(); 
                if(!p->Deleted())
                    p->FoldConstants();  
                return true;
            });
    } while (IsChanged());
}

void CASTTreeBase::PrintErrorsWarnings() const
{
    Information(fmt::format("Ошибок : {}, Предупреждений: {}", nErrors, nWarnings));
}

void CASTTreeBase::PrintInfix()
{
    Debug(pRoot->GetInfix());
    Debug(fmt::format("Score : {} ", Score()));
    PrintErrorsWarnings();
}

size_t CASTTreeBase::Score()
{
    DFSPost(pRoot, [](CASTNodeBase* p)
        {
            p->UpdateScore();
            return true;
        }
    );
    return pRoot->Score();
}

void CASTTreeBase::Collect()
{
    do
    {
        Unchange();
        Flatten();
        DFSPost(pRoot, [](CASTNodeBase* p)
            {
                // обновляем скоринг узла
                p->UpdateScore();   
                // обновляем макимальную степень
                p->MaxSortPower();
                // если оператор плоский
                if (CASTNodeBase::IsFlatOperator(p))
                {
                    // сортируем его
                    static_cast<CASTFlatOperator*>(p)->Sort();
                    // и внутри делаем Collect
                    static_cast<CASTFlatOperator*>(p)->Collect();
                }
                // делаем оператор плоским
                p->Flatten();
                // если оператор не был удален
                // собираем константы
                if(!p->Deleted())
                    p->FoldConstants();

                return true;
            });
    } while (IsChanged());
}


void CASTTreeBase::Sort()
{
    ASTFragmentsMap fragments;
    DFSPost(pRoot, [&fragments](CASTNodeBase* p)
        {
            p->MaxSortPower();
            if(CASTNodeBase::IsFlatOperator(p))
                static_cast<CASTFlatOperator*>(p)->Sort();

            switch (p->GetType())
            {
                case ASTNodeType::Root:
                case ASTNodeType::Main:
                case ASTNodeType::Init:
                case ASTNodeType::EquationSystem:
                case ASTNodeType::Equation:
                    break;
                default:
                    fragments[p->GetInfix()].insert(p);
            }

            return true;
        });

    RemoveSingleFragments(fragments);
}


template<> CASTRoot* CASTTreeBase::CreateNode<CASTRoot>(CASTNodeBase* pParent)
{
    if (pRoot)
        EXCEPTIONMSG("Root already defined");

    CASTRoot *pNewRoot = CreateNodeImpl<CASTRoot>(pParent);
    pRoot = pNewRoot;
    return pNewRoot;
}

// создает правила преобразований AST-дерева
void CASTTreeBase::CreateRules()
{
    // все правила располагаются внутри дерева, каждый узел
    // которого сопоставляется с узлом обрабатываемого дерева

    // дерево правил начинается к корня
    CASTNodeBase* pRoot   = CreateNode<CASTRoot>(nullptr);
    // правила применяются внутри системы уравнений
    CASTNodeBase* pSystem = pRoot->CreateChild<CASTEquationSystem>();
   
    
    // правило общего множителя a*b + a*c = a*(b+c) для уравнения
    CASTNodeBase* pRule1 = pSystem->CreateChild<CASTEquation>();
    CASTNodeBase* pSum   = pRule1->CreateChild<CASTSum>();
    CASTNodeBase *pMult1 = pSum->CreateChild<CASTMul>();
    CASTNodeBase* pMult2 = pSum->CreateChild<CASTMul>();
    pMult1->CreateChild<CASTAny>("");
    pMult1->CreateChild<CASTAny>("");
    pMult2->CreateChild<CASTAny>("");
    pMult2->CreateChild<CASTAny>("");
    CASTNodeBase* pMult3 = pRule1->CreateChild<CASTMul>();
    pMult3->CreateChild<CASTAny>("");
    CASTNodeBase* pSum3 = pMult3->CreateChild<CASTSum>();
    pSum3->CreateChild<CASTAny>("");
    pSum3->CreateChild<CASTAny>("");

    // тест правила суммы квадратов - пока замаплен в ноль
    CASTNodeBase* pRule2 = pSystem->CreateChild<CASTEquation>();
    CASTNodeBase* pSumR2 = pRule2->CreateChild<CASTSum>();
    pRule2->CreateChild<CASTNumeric>(0.0);
    CASTNodeBase* pPow1 = pSumR2->CreateChild<CASTPow>();
    pPow1->CreateChild<CASTAny>("a");
    pPow1->CreateChild<CASTNumeric>(2.0);
    CASTNodeBase* pPow2 = pSumR2->CreateChild<CASTPow>();
    pPow2->CreateChild<CASTAny>("b");
    pPow2->CreateChild<CASTNumeric>(2.0);


    CASTNodeBase* pMinus = pSumR2->CreateChild<CASTUminus>();
    CASTNodeBase* pMul2ab = pMinus->CreateChild<CASTMul>();
    pMul2ab->CreateChild<CASTNumeric>(2.0);
    pMul2ab->CreateChild<CASTAny>("a");
    pMul2ab->CreateChild<CASTAny>("b");

    // заглушка чтобы было 2 требуемых дочерних узла у корня правил
    pRoot->CreateChild<CASTNumeric>(0.0); 
    //pRoot->PrintInfix();
}

void CASTTreeBase::MatchAtRuleEntry(const CASTNodeBase* pRuleEntry, const CASTNodeBase* pRule)
{
    CASTNodeBase* pPrintRule = pRule->ChildNodes().front();
    pRule = pPrintRule;
    using MatchStack = std::stack < CMatchInfo >;

    CMatchRuleNodeToTreeNode RuleToTree;

    // сет хэшей полученных комбинаций Any
    std::set<size_t> AnysCombinations;

    // выделяем узлы для матчинга в отдельный список
    // для подсчета хэша и исключения дублирования ответов
    // можно сделать для всех правил заранее
    // также выделяем все узлы с возможностью перестановок,
    // чтобы выполнять поиск с комбинациями
    ASTNodeList Anys;
    ASTNodeList PermutationsChain;
    DFS(pPrintRule, [&Anys, &PermutationsChain](CASTNodeBase* p) 
            { 
                if (p->CheckType(ASTNodeType::Any))
                    Anys.push_back(p);
                // для n-арных операторов создаем цепочку перестановок
                if (CASTNodeBase::IsFlatOperator(p))
                    PermutationsChain.push_back(p);
                return true;
            }
       );

    do
    {
        ASTNodeConstSet visited;
        MatchStack stack;

        // обходим правило и дерево синхронно, углубляясь внутрь структуры по стеку
        // начинаем с корневых узлов дерева и правил
        stack.push(RuleToTree.MatchInfo(pRuleEntry, pPrintRule));

        bool bContinueSearch = true;
        while (!stack.empty() && bContinueSearch)
        {
            // достаем из стека текущую пару узел дерева + узел правил
            CASTNodeBase* rule = stack.top().RuleNode();
            CTreeNodeInfo& nodeInfo = stack.top().NodeInfo();

            const auto& TreeChildNodes = nodeInfo.ChildNodes();

            if (!visited.Contains(rule))
            {
                // если узел правил еще не просматривали
                visited.insert(rule);
                ASTNodeConstSet found;
                // для всех дочерних узлов правила
                for (auto c : rule->ChildNodes())
                {
                    // ищем узел исходного дерева по типу в порядке узлов дерева правила
                    // мы ищем узлы дерева от начала к концу списка,
                    // поэтому порядок поиска нужно менять, чтобы найти нужную комбинацию узлов из правила
                    auto treecit = std::find_if(TreeChildNodes.begin(), TreeChildNodes.end(),
                        [&c, &found](const CASTNodeBase* p)
                        {
                            auto type = c->GetType();
                            // если этот узел дерева уже нашли, то пропускаем,
                            // он уже "занят"
                            if (found.Contains(p))
                                return false;

                            switch (type)
                            {
                            case ASTNodeType::Any:
                                return true;            // Any cоответсвует любому узлу исходного дерева
                            case ASTNodeType::Numeric:
                                // если узел - число, сравниваем значение со значением из правила
                                return CASTNodeBase::IsNumeric(p) && CASTNodeBase::NumericValue(c) == CASTNodeBase::NumericValue(p);
                            default:
                                return type == p->GetType();  // если типы узлов правила и дерева совпадают - тоже подходит.
                            }
                        });

                    // если текущий узел правила нашли
                    if (treecit != TreeChildNodes.end())
                    {
                        // вводим его в сет найденных, чтобы не найти еще раз
                        found.insert(*treecit);
                        // уходим в стек с найденным узлом
                        stack.push(RuleToTree.MatchInfo(*treecit, c));
                    }
                    else
                    {
                        // текущий узел правила не нашли - дальше искать нечего
                        // выходим и пробуем новую комбинацию узлов исходного дерева
                        bContinueSearch = false;
                        break;
                    }
                }
            }
            else
            {
                // если узел уже просмотрели 
                const CASTNodeBase* tree = nodeInfo.TreeNode();
                if (CASTNodeBase::IsLeaf(rule))
                {
                    // если у узла нет дочерних 
                    switch (rule->GetType())
                    {
                    case ASTNodeType::Any: // на Any задаем текущий узел дерева
                        static_cast<CASTAny*>(rule)->SetMatch(tree);
                        rule->SetText(tree->Infix());
                        break;
                    default:
                        // _ASSERTE(0);
                        break;
                    }
                }
                // возвращаемся по стеку вверх
                stack.pop();
            }
        }

        // если все узлы правила нашли по всей глубине
        if (bContinueSearch)
        {
            // для блокировки вывода повторов используем хэш
            // указателей на match из заполненных CASTAny
            size_t hash = 0;
            for (auto& c : Anys)
                hash_combine(hash, static_cast<const CASTAny*>(c)->GetMatch());
            if (AnysCombinations.insert(hash).second)
            {
                pPrintRule->PrintInfix();
            }
        }

        // это для отладки

        for (auto& c : Anys)
        {
            static_cast<CASTAny*>(c)->SetMatch(nullptr);        // сбрасываем match
            c->ToInfix();                                       // обновляем текст "Any"
        }
        pPrintRule->ToInfix();                                  // обновляем текст правил с "Any"

    } while (RuleToTree.NextPermutation(PermutationsChain));
}

void CASTTreeBase::Match(const CASTNodeBase* pRule)
{
    // в качестве правила используется уравнение
    if (!pRule->CheckType(ASTNodeType::Equation)) return;
    // выделяем левую часть уравнения и ищем в текущем дереве 
    // узел, соответствующий корню левой части
    for (auto& c : NodeList)
    {
        if(c->GetType() == pRule->ChildNodes().front()->GetType() && !c->Deleted())
            MatchAtRuleEntry(c, pRule);
    }
}

const CASTNodeBase* CASTTreeBase::GetRule()
{
    auto pEqs = pRoot->ChildNodes().begin();
    auto pEq1 = (*pEqs)->ChildNodes().begin();
    //return *std::next(pEq1);
    return *pEq1;
}
void CASTTreeBase::RemoveSingleFragments(ASTFragmentsMap& fragments) const
{
    // удаляем из фрагментов те, которые повторяются менее двух раз
    for (auto it = fragments.begin(); it != fragments.end(); )
    {
        auto& nodes{ it->second };
        // удаляем ссылки на удаленные узлы
        for (auto nodeit{ nodes.begin() }; nodeit != nodes.end();)
            if ((*nodeit)->Deleted())
                nodeit = nodes.erase(nodeit);
            else
                nodeit++;

        if (it->second.size() < 2)
            it = fragments.erase(it);
        else
            it++;
    }

    // проверяем, что найденные инфиксы действительно дают совпадение при 
    // проверке идентичности узлов в дереве
    for (auto it : fragments)
    {
        for (auto nit : it.second)
            for (auto nit2 : it.second)
                _ASSERTE(nit->Compare(nit2) == 0);
    }
}

CASTEquationSystem* CASTTreeBase::GetEquationSystem(ASTNodeType EquationSystemType)
{
    if (EquationSystemType != ASTNodeType::Main &&
        EquationSystemType != ASTNodeType::Init &&
        EquationSystemType != ASTNodeType::InitExtVars)
        EXCEPTIONMSG("Main or init types only applicable");

    auto itSystem = std::find_if(NodeList.begin(), NodeList.end(), [&EquationSystemType](const CASTNodeBase* p)
        {
            return !p->Deleted() && p->CheckType(ASTNodeType::EquationSystem) && p->GetParent()->CheckType(EquationSystemType);
        }
    );
    return itSystem == NodeList.end() ? nullptr : static_cast<CASTEquationSystem*>(*itSystem);
}

// строит карту соответствия между переменными и уравнениями, в которые они входят
void CASTTreeBase::ChainVariablesToEquations()
{
    if (ErrorCount() != 0) return;

    // очищаем текущие списки переменных в уравнениях
    for (auto& e : Equations)
        e->Vars.clear();

    // для всех переменных в карте
    for (auto& v : Vars)
    {
        // очищаем список уравнений
        v.second.Equations.clear();
        // если переменная константа - не учитываем ее
        if (v.second.Constant) continue;
        // проходим по списку экземпляров переменных и собираем уравнения, в которые они входят
        for (auto& vi : v.second.VarInstances)
        {
            CASTEquation* pVariableParentEquation(vi->GetParentEquation());
            // добавляем родительское уравнение в переменную
            v.second.Equations.insert(pVariableParentEquation);
            // ищем переменную по имени в карте переменных
            // так как в ссылках уравнения на переменные используются итераторы на карту
            auto itV = Vars.find(v.first);
            // если переменная найдена в карте 
            // добавляем ее к списку переменных уравнений
            if (itV != Vars.end())
                pVariableParentEquation->Vars.insert(itV);
            else
                EXCEPTIONMSG("Variable name not found in map");
        }
    }
}

void CASTTreeBase::CheckInitEquations()
{
    if (ErrorCount() > 0) return;

    for (auto& e : Equations)
    {
        if (!e->GetParentOfType(ASTNodeType::Init)) continue;
        const CASTNodeBase* pInitVar = e->ChildNodes().front();
        if (!pInitVar->CheckType(ASTNodeType::Variable))
        {
            Error(fmt::format("Уравнения инициализации должны иметь вид присваивания : Variable = Expression {}",
                    e->GetEquationDescription()));
        }
        else
        {
            auto pVarInfo = CheckVariableInfo(pInitVar->GetText());
            if (!pVarInfo)
                EXCEPTIONMSG("No variable info");
            // проверяем присвоение константной переменной
            // разрешаем это только для переменных в секции инициализации базовых значений внешний переменных
            if (pVarInfo->Constant && e->GetParentOfType(ASTNodeType::InitExtVars) == nullptr)
                Error(fmt::format("Попытка при инициализации присвоить значение переменной \"{}\", объявленной как const. \"{}\" {}", 
                    pInitVar->GetText(),
                    stringutils::ctrim(e->GetInfix()),
                    e->GetEquationSourceDescription()));
        }
    }
}

std::string CASTTreeBase::ProjectMessage(const std::string_view& message) const
{
    if (const auto projNameIt = properties.find(PropertyMap::szPropProjectName); projNameIt != properties.end())
        return fmt::format("{} : {}", projNameIt->second, message);
    else
        return std::string(message);
}

void CASTTreeBase::Error(std::string_view error) const
{
#ifdef _MSC_VER    
    SetConsoleOutputCP(CP_UTF8);
#endif    
    if (messageCallBacks.Error)
        messageCallBacks.Error(ProjectMessage(error));
    else
        std::cout << DFW2::CDFW2Messages::m_cszError << error << std::endl;
    nErrors++;
}

void CASTTreeBase::Warning(std::string_view warning) const
{
#ifdef _MSC_VER    
    SetConsoleOutputCP(CP_UTF8);
#endif    
    if (messageCallBacks.Warning)
        messageCallBacks.Warning(ProjectMessage(warning));
    else
        std::cout << DFW2::CDFW2Messages::m_cszWarning << warning << std::endl;
    nWarnings++;
}

void CASTTreeBase::Message(std::string_view message) const
{
#ifdef _MSC_VER    
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (messageCallBacks.Message)
        messageCallBacks.Message(ProjectMessage(message));
    else
        std::cout << DFW2::CDFW2Messages::m_cszInfo << message << std::endl;
}

void CASTTreeBase::Information(std::string_view error) const
{
#ifdef _MSC_VER    
    SetConsoleOutputCP(CP_UTF8);
#endif    
    if (messageCallBacks.Info)
        messageCallBacks.Info(ProjectMessage(error));
    else
        std::cout << DFW2::CDFW2Messages::m_cszError << error << std::endl;
}

void CASTTreeBase::Debug(std::string_view message) const
{
#ifdef _MSC_VER    
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (messageCallBacks.Debug)
        messageCallBacks.Debug(ProjectMessage(message));
    else
        std::cout << DFW2::CDFW2Messages::m_cszDebug << message << std::endl;
}

size_t CASTTreeBase::ErrorCount() const
{
    return nErrors;
}

VariableInfo& CASTTreeBase::GetVariableInfo(std::string_view VarName)
{
    return Vars[std::string(VarName)];
}

VariableInfo* CASTTreeBase::CheckVariableInfo(std::string_view VarName)
{
    VariableInfo* pVarInfo(nullptr);
    auto it = Vars.find(VarName);
    if (it != Vars.end())
        pVarInfo = &it->second;
    return pVarInfo;
}

ASTEquationsSet& CASTTreeBase::GetEquations()
{
    return Equations;
}

ASTHostBlocksSet& CASTTreeBase::GetHostBlocks()
{
    return HostBlocks;
}

const VarInfoMap& CASTTreeBase::GetVariables() const
{
    return Vars;
}

void CASTTreeBase::GenerateEquations()
{
    // преобразуем систему Main в дерево операторов
    CASTEquationSystem* pSystem = GetEquationSystem(ASTNodeType::Main);
    TransformToOperatorTree(pSystem);
    // избавляемся от тривиальных уравнений
    RemoveTrivialEquations(pSystem);
    // сортируем уравнения инициализации в порядок присваивания
    GenerateInitEquations();
    // строим связи между переменными и уравнениями
    ChainVariablesToEquations();
    SortEquationSection(ASTNodeType::Init);
    pRoot->PrintInfix();
    CheckInitEquations();
    SortEquationSection(ASTNodeType::Main);
    GenerateJacobian(pSystem);
}


CASTJacobiElement* CASTTreeBase::CreateDerivative(CASTJacobian* pJacobian, CASTEquation* pEquation, VarInfoMap::iterator itVariable)
{
     if (!pEquation->CheckType(ASTNodeType::Equation))
        EXCEPTIONMSG("Equation has wrong type");
    if (pEquation->ChildNodes().size() != 2)
        EXCEPTIONMSG("Equation must have 2 children");
    if (!CASTNodeBase::IsNumericZero(pEquation->ChildNodes().back()))
        EXCEPTIONMSG("Equation 2nd child must be zero");
    if (!CASTNodeBase::IsSum(pEquation->ChildNodes().front()) &&
        !CASTNodeBase::IsVariable(pEquation->ChildNodes().front()))
        EXCEPTIONMSG("Equation 1st child must be sum or variable");

    CASTJacobiElement* pJacobiElement = pJacobian->CreateChild<CASTJacobiElement>(pEquation, itVariable);

    struct DerivativeStack
    {
        CASTNodeBase* pSourceNode;
        CASTNodeBase* pDerivativeNode;
    };
    std::stack<DerivativeStack> stack;
    ASTNodeConstSet visited;

    stack.push({ pEquation->ChildNodes().front(), pJacobiElement });

    while (!stack.empty())
    {
        auto v = stack.top();

        if (v.pSourceNode->Deleted() || v.pDerivativeNode->Deleted())
            EXCEPTIONMSG("Processing deleted derivative node");

        stack.pop();
        if (!visited.Contains(v.pSourceNode))
        {
            visited.insert(v.pSourceNode);
            CASTNodeBase* pDerivative = v.pSourceNode->GetDerivative(itVariable, v.pDerivativeNode);
            if (pDerivative)
                for (auto c : v.pSourceNode->ChildNodes())
                    stack.push({ c, pDerivative });
        }
    }

    return pJacobiElement;
}


CASTJacobian* CASTTreeBase::GetJacobian()
{
    return pJacobian;
}

// генерирует матрицу Якоби для системы уравнений
void CASTTreeBase::GenerateJacobian(CASTEquationSystem* pSystem)
{
    // если есть ошибки в дереве - отказываемся работать
    if (ErrorCount() > 0) return;

    // создаем дочерний узел системы - Якобиан
    pJacobian = pSystem->CreateChild<CASTJacobian>();
    // проходим по уравнениям системы
    for (auto& eq : pSystem->ChildNodes())
    {
        // обрабатываем только уравнения
        if (!eq->CheckType(ASTNodeType::Equation)) continue;
        CASTEquation* pSourceEquation = static_cast<CASTEquation*>(eq);
        // если уравнение содержит хост-блок - пропускаем
        if (pSourceEquation->pHostBlock != nullptr) continue;
        // для остальных уравнений формируем выражения для расчета частных производных
        // от всех переменных уравенения
        for (auto& v : pSourceEquation->Vars)
        {
            // еще надо проверять переменные на то, что они являются константами
            CASTJacobiElement* pJacobiElement = CreateDerivative(pJacobian, pSourceEquation, v);
        }
    }
    // упрощаем выражения в дереве (в том числе и в якобиане, но можно стоит только якобиан, чтобы не терзать уравнения)
    Collect();
}

CASTVariable* CASTTreeBase::CreateVariableFromModelLink(std::string_view ModelLink, CASTNodeBase* Parent)
{
    std::string extVarName = FindModelLinkVariable(ModelLink);
    // если такой ссылки еще не ввели, генерируем для ссылки новое имя переменной
    // если есть - используем уже существующее имя и создаем просто экземпляр уже
    // существующей переменной
    if (extVarName.empty())
        extVarName = GenerateUniqueVariableName("ext");

    auto VarInfo = CheckVariableInfo(extVarName);

    auto var = Parent->CreateChild<CASTVariable>(extVarName);
    var->Info().FromSource = true;          // переменную из исходника в любом случае отмечаем как исходник
    var->Info().External = true;            // переменную отмечаем как внешнюю, так как это ссылка
    var->Info().ModelLink = ModelLink;      // сохраняем ссылку на модель в метаданных переменной


    // тут надо посмотреть, как вести себя с внешними переменными в Init
    if (!VarInfo)
    {
        // если переменная _впервые_ создается в Init-секции - отмечаем ее как Init
        // по идее она не должна входить в систему уравнений как переменная
        // так как ее уравнение должно быть в Main
        if (var->GetParentOfType(ASTNodeType::Init))
            var->Info().InitSection = true;
    }

    return var;
}

void CASTTreeBase::InitExternalBaseVariables()
{
    // создаем новую систему уравнений внутри Init для присваивания базовым значения переменных
    // значений внешний переменных в момент инициализации
    if (pExternalBaseInit == nullptr)
        pExternalBaseInit = GetEquationSystem(ASTNodeType::Init)->CreateChild<CASTInitExtVars>()->CreateChild<CASTEquationSystem>();

    // ищем узлы базового значения переменной
    for (auto base : NodeList)
    {
        if (!base->Deleted() && base->CheckType(ASTNodeType::ModLinkBase))
        {
            // получаем имя внешней переменной
            std::string extVarName(base->ChildNodes().front()->GetText());
            // генерируем имя переменной-константы, в которой будет значение внешней переменной при t=0
            std::string extVarBaseName = fmt::format("{}{}", extVarName, "base");
            // удаляем экземпляр внешней переменной, так как вместо нее будет константа
            base->DeleteChild(base->ChildNodes().begin());
            // создаем уравнение для инициализации внешней переменной
            CASTEquation* pExternalInitEquation = pExternalBaseInit->CreateChild<CASTEquation>();
            
            base->GetParent()->ReplaceChild<CASTVariable>(base,extVarBaseName);
            auto baseVar = pExternalInitEquation->CreateChild<CASTVariable>(extVarBaseName);
            pExternalInitEquation->CreateChild<CASTVariable>(extVarName);
            VariableInfo& baseInfo = baseVar->Info();
            if (baseInfo.pResovledByEquation == nullptr)
            {
                baseInfo.GeneratedByCompiler = baseInfo.Constant = 
                baseInfo.ExternalBase = baseInfo.FromSource = true;

                baseInfo.pResovledByEquation = pExternalInitEquation;
            }
        }
    }
}

CASTVariable* CASTTreeBase::CreateBaseVariableFromModelLink(std::string_view ModelLink, CASTNodeBase* Parent)
{
    
    // находим переменную с заданной ссылкой на модель
    std::string extVarName = FindModelLinkVariable(ModelLink);
    if (extVarName.empty())
    {
        // если внешней переменной со ссылкой еще нет, это означает что базовому значению пока не на что сослаться,
        // поэтому создаем dummy внешнюю переменную только для того, чтобы определить имя и задать свойства
        auto extVar = CreateVariableFromModelLink(ModelLink, Parent);
        extVarName = extVar->GetText();
        Parent->DeleteChild(extVar);
    }

    std::string extBaseName = fmt::format("{}{}", extVarName, "base");
    auto baseVar = Parent->CreateChild<CASTVariable>(extBaseName);
    VariableInfo& baseVarInfo = baseVar->Info();
    baseVarInfo.GeneratedByCompiler = baseVarInfo.Constant = baseVarInfo.ExternalBase = true;
    return baseVar;
}

void CASTTreeBase::ProcessProxyFunction()
{
    // так как манипуляции могут изменить NodeList, сначала отбираем элементы, которые
    // могут быть измененеы

    ASTNodeList range;
    std::copy_if(NodeList.begin(), NodeList.end(), std::back_inserter(range), [](CASTNodeBase* p) 
        { 
            return !p->Deleted() && p->CheckType(ASTNodeType::FnProxyVariable);
        });

    for (auto &r : range)
    {
        CASTFunctionBase* pFn = static_cast<CASTFunctionBase*>(r);
        // собираем в аргументе функции узлы переменных "V" и "BASE"
        ASTNodeList V, Base;
        DFS(pFn->ChildNodes().front(), [&V, &Base](CASTNodeBase* p)
            {
                if (CASTNodeBase::IsVariable(p))
                {
                    if (p->GetText() == "V")
                        V.push_back(p);
                    else if (p->GetText() == "BASE")
                        Base.push_back(p);
                }
                return true;
            });

        // проверяем второй дочерний узел
        CASTNodeBase* pExtVar = nullptr;
        if (pFn->ChildNodes().size() == 2)
        {
            // проверяем, что это переменная и у нее есть ModelLink
            if (CASTNodeBase::IsVariable(pFn->ChildNodes().back()) &&
                !static_cast<CASTVariable*>(pFn->ChildNodes().back())->Info().ModelLink.empty())
                pExtVar = pFn->ChildNodes().back();
            else
                pFn->Error(fmt::format("В функции {} во втором аргументе должна быть ссылка на модель.", pFn->GetText()), true);
        }

        // если переменные "V" и "BASE" найдены, заменяем их
        // на переменную, которая задана во втором аргументе функции Starter/Action

        bool bVandBaseReplacedOK = true;

        if (V.size() > 0 || Base.size() > 0)
        {
            bVandBaseReplacedOK = false;
            // во втором аргументе должна быть задана внешняя переменная
            if (pExtVar != nullptr)
            {
                // вместо переменных "V" и "BASE" создаем инстансы переменной из второго аргумента
                for (auto& v : V)
                    v->GetParent()->ReplaceChild<CASTVariable>(v, pExtVar->GetText());

                for (auto& v : Base)
                {
                    auto modlinkbase = v->GetParent()->ReplaceChild<CASTModLinkBase>(v);
                    modlinkbase->CreateChild<CASTVariable>(pExtVar->GetText());
                }

                /*
                // если есть ссылка на Base, то нужна 
                // отдельная переменная, в которой мы сохраним значение этой внешней переменной в Init
                if (Base.size() > 0)
                {
                    // формируем имя для переменной, в которой будет значение внешней переменной в начальный момент времени
                    std::string extBaseName = fmt::format("{}{}", pExtVar->GetText(), "base");
                    // заменяем все упоминания переменной "Base" на на переменную с новым именем
                    for (auto& v : Base)
                        v->GetParent()->ReplaceChild<CASTVariable>(v, extBaseName);
                    // ставим признаки новой переменной
                    VariableInfo& baseVarInfo = Vars.find(extBaseName)->second;
                    baseVarInfo.GeneratedByCompiler = baseVarInfo.Constant = baseVarInfo.ExternalBase = true;
                }
                */
                bVandBaseReplacedOK = true;
            }
            else
                pFn->Error(fmt::format("В функции {} используется ссылка на внешнюю переменную, но переменная не задана.", pFn->GetText()), true);
        }

        if (bVandBaseReplacedOK)
        {
            // если замена V и "BASE" прошла успешно, или ее вовсе не было,
            // заменяем функцию на первый аргумент
            CASTNodeBase* p1stArg = pFn->ExtractChild(pFn->ChildNodes().front());
            pFn->GetParent()->ReplaceChild(pFn, p1stArg);
        }
    }
}

void CASTTreeBase::ProcessConstantArgument()
{
    // постоянные аргументы выделяем из дочерних узлов функций и 
    // переносим в уравнения дополнительной системы в Init-секции
    CASTEquationSystem* pInit = GetEquationSystem(ASTNodeType::Init);
    CASTEquationSystem* pInitHostConstants = nullptr;
    // счетчик блоков
    size_t block(0);

    // так как манипуляции могут изменить NodeList, сначала отбираем элементы, которые
    // могут быть измененеы
    ASTNodeList range;
    std::copy_if(NodeList.begin(), NodeList.end(), std::back_inserter(range), [](CASTNodeBase* p) 
        { 
            return !p->Deleted() && CASTNodeBase::IsFunction(p);  
        });

    for (auto &r : range)
    {
        CASTFunctionBase* pFn = static_cast<CASTFunctionBase*>(r);
        CASTFunctionBase::ArgumentProps props;
        pFn->GetArgumentProps(props);
        size_t ArgNumber(0);

        for (auto& c : props)
        {
            if (c.argument.ConstantOnly && c.pChild != nullptr)
            {
                // если аргумент должен быть константой
                // формируем для него уравнение в дополнительной секции Init
                // если этой секции пока нет - создаем ее
                if (pInitHostConstants == nullptr)
                    pInitHostConstants = pInit->CreateChild< CASTInitConstants>()->CreateChild<CASTEquationSystem>();
                CASTEquation* pConstParameterEquation = pInitHostConstants->CreateChild<CASTEquation>();
                // формируем имя переменной, соответствующей константному аргументу
                CASTVariable* pConstArgVar = pConstParameterEquation->CreateChild<CASTVariable>(fmt::format("Func{}ConstArg{}",
                    block,
                    ArgNumber));
                VariableInfo& varInfo = pConstArgVar->Info();
                varInfo.GeneratedByCompiler = true;
                varInfo.InitSection = true;    // задаем свойство переменной Internal - чтобы исключить ее вывод в исходный текст

                // выражение для расчета аргумента выносим из дочерних узлов в правую часть нового уравнения
                CASTNodeBase* pConstParameterExpr = c.pChild;
                pFn->ChildNodes().erase(pFn->FindChild(c.pChild));
                pConstParameterEquation->CreateChild(pConstParameterExpr);
                // в блоке сохраняем ссылки на уравнения переменных инициализации
                pFn->ConstantArguments.push_back({ pConstParameterEquation, ArgNumber });
            }
            ArgNumber++;
        }
        block++;
    }
}

// проверяем дерево после парсинга, но до первого обхода
// чтобы найти ошибки в количестве аргументов функций и блоков,
// а также выделить аргументы функций и блоков, которые требуются в виде констант
void CASTTreeBase::PostParseCheck()
{
    // если при парсинге были ошибки - останавливаемся 
    if (ErrorCount() > 0) return;

    // если в исходном тексте была опущена секция Init, создаем ее
    // так как понадобятся уравнения для констант блоков и присваивания
    // базовых значений внешних переменных

    if (GetEquationSystem(ASTNodeType::Init) == nullptr)
        pRoot->CreateChild<CASTInit>()->CreateChild<CASTEquationSystem>();



    // просматриваем узлы, которые являются функциями (функции и хост-блоки)
    for (auto& p : NodeList)
    {
        if (CASTNodeBase::IsFunction(p))
        {
            CASTFunctionBase* pFn = static_cast<CASTFunctionBase*>(p);
            const CASTFunctionBase::FunctionInfo& fi = pFn->GetFunctionInfo();
            // если функция вариадик - проверяем количество аргументов, обозначенных как обязательные
            if (fi.Variadic)
            {
                // если количество аргументов меньше количества обязательных аргументов - ошибка
                if(p->ChildNodes().size() < fi.MandatoryArguments)
                    p->Error(fmt::format("Неправильное количество параметров функции {}. Ожидается {}...∞, задано {}.",
                        pFn->GetText(),
                        fi.MandatoryArguments,
                        p->ChildNodes().size()), true);
            }
            else
            {
                // для невариадик функций проверяем количество аргументов по допустимому количеству и количеству обязательных аргументов
                if (p->ChildNodes().size() > fi.Args.size() || p->ChildNodes().size() < fi.MandatoryArguments)
                    p->Error(fmt::format("Неправильное количество параметров функции {}. Ожидается {}, задано {}.",
                        pFn->GetText(),
                        fi.Args.size() != fi.MandatoryArguments ? fmt::format("{}...{}", fi.MandatoryArguments, fi.Args.size()) : fmt::format("{}", fi.Args.size()),
                        p->ChildNodes().size()), true);
            }
        }
    }

    // если после проверки количества аргументов появились ошибки - останавливаемся
    if (ErrorCount() > 0) return;

    ProcessProxyFunction();
    InitExternalBaseVariables();
    ProcessConstantArgument();
    Score(); // делаем первый DFS. Если бы не проверили соответствие количества переменных - были бы исключения при DFS

    for (auto& p : NodeList)
    {
        if (CASTNodeBase::IsFunction(p))
        {
            CASTFunctionBase* pFn = static_cast<CASTFunctionBase*>(p);
            // для всех функций и блоков проверяем
            // списки константных аргументов
            for (auto& cp : pFn->ConstantArguments)
            {
                CASTNodeBase* pExpression = cp.first->ChildNodes().back();
                // если выражение после DFS не является константным - выводим ошибку
                if (!pExpression->IsConst())
                {
                    p->Error(fmt::format("Функция {} ожидает в аргументе {} константу. Выражение \"{}\" не является константой",
                        p->GetText(),
                        cp.second,
                        pExpression->GetInfix()), true);
                }
            }
        }
    }
}

// генерирует уникальное имя переменной по заданному префиксу
std::string CASTTreeBase::GenerateUniqueVariableName(std::string_view Prefix)
{
    if (Prefix.length() == 0 || !std::isalpha(Prefix[0]))
        EXCEPTIONMSG("Wrong variable name prefix");

    size_t counter = 0;
    while (1)
    {
        std::string newName = fmt::format("{}{}", Prefix, counter++);
        if (Vars.find(newName) == Vars.end())
            return newName;
    }
}

// возвращает имя внешней переменной по заданной ссылке на модель
// если такой ссылки на модель еще не было - возвращает пустое имя
std::string CASTTreeBase::FindModelLinkVariable(std::string_view ModelLink)
{
    for (auto& v : Vars)
    {
        if (v.second.External)
            if (v.second.ModelLink == ModelLink)
                return v.first;
    }
    return "";
}


std::filesystem::path CASTTreeBase::GetPropertyPath(std::string_view PropNamePath)
{
    std::filesystem::path Path;
    if (auto itp = properties.find(PropNamePath); itp != properties.end())
    {
        Path = itp->second;
        if (Path.has_filename())
        {
            Warning(fmt::format("Путь не должен содержать имени файла \"{}\"", Path.string()));
            Path.remove_filename();
            itp->second = Path.string();
        }
    }
    return Path;
}


// Для части блоков которые могут быть Flat (|, &) и при разборе
// были представлены функциями нескольких аргументов нужна
// замена функций на хост-блоки, связанная с необходимостью определять
// разрывы

void CASTTreeBase::ApplyHostBlocks()
{
    // находим блоки, которые должны быть заменены на хост-блоки (Or/And)
    ASTNodeList ChangeBlocks;
    std::copy_if(NodeList.begin(), NodeList.end(), std::back_inserter(ChangeBlocks), [](const CASTNodeBase* pNode)
        {
            return pNode->CheckType(ASTNodeType::FnOr, ASTNodeType::FnAnd) && !pNode->Deleted();
        });

    // для всех найденных блоков
    for (auto& node : ChangeBlocks)
    {
        CASTHostBlockBase* host{ nullptr };
        switch (node->GetType())
        {
        case ASTNodeType::FnOr:
            host = CreateNode<CASTfnOrHost>(node->GetParent(), GetHostBlocks().size());
            break;
        case ASTNodeType::FnAnd:
            host = CreateNode<CASTfnAndHost>(node->GetParent(), GetHostBlocks().size());
            break;

        }
        if (host)
        {
            // переключаем дочерние узлы из исходного узла на хост-блок
            ASTNodeList OrChildren;
            node->ExtractChildren(OrChildren);
            host->AppendChildren(OrChildren);
            // заменяем исходный узел на новый хост-блок
            node->GetParent()->ReplaceChild(node, host);
        }
    }

};