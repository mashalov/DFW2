#pragma once
#include "ASTNodeBase.h"

// отображение узла дерева
// со списком дочерних узлов, который можно 
// перемешивать, добиваясь всех возможных совпадений
class CTreeNodeInfo
{
    const CASTNodeBase* ptreeNode;
    ASTNodeList TreeChildNodes;

    // структура для связи списка дочерних узлов
    // и их индексов в списке

    // идея : строим список индексов дочерних узлов flat-узла
    // и ставим в соответствие индексу итератор списка дочерних узлов
    // для того чтобы перебрать все возможные комбинации последовательностей
    // дочерних узлов строим пермутации индексов c помощью std::next_permutation
    // а далее, соответствие итераторов индексов строим порядок дочерних узлов

    // данную операцию делаем, конечно, не на исходном списке узлов дерева, а на копии

    struct Sort
    {
        ASTNodeList::iterator it;   // итератор на дочерний узел
        ptrdiff_t Index;            // индекс дочернего узла

        // оператор сравнения для std::next_permutation 
        // и сортировки
        bool operator < (const Sort& lhs) const
        {
            return Index < lhs.Index;
        }
    };
    std::vector<Sort> order;

    // формирование вектора исходного порядка дочерних узлов
    void StartPermutations()
    {
        ptrdiff_t index(0);
        // проходим по списку дочерних узлов
        // и нумеруем их по порядку
        // запоминаем последовательности в order
        for (auto it = TreeChildNodes.begin(); it != TreeChildNodes.end(); it++, index++)
            order.push_back({ it, index });
        // сортируем (хотя это не особенно нужно, так как индексы и так упорядочены после цикла)
        std::sort(order.begin(), order.end());
        // приводим порядок итераторов в соответствие порядку индексов
        ToOrder();
    }
    // приведение списка дочерних узлов к текущему порядку сортировки
    void ToOrder()
    {
        // просто строим новый список дочерних узлов и заменяем им старый
        ASTNodeList TreeChildNodesNewOrder(TreeChildNodes.size());
        // transform строит новый список просто выбирая итератор из перемешанного вектора индексов
        std::transform(order.begin(), order.end(), TreeChildNodesNewOrder.begin(), [](const Sort& elm) { return *elm.it; });
        TreeChildNodes = TreeChildNodesNewOrder;
    }

public:
    // на вход принимаем исходный узел дерева и список его дочерних узлов
    CTreeNodeInfo(const CASTNodeBase* pTreeNode) : ptreeNode(pTreeNode)
    {
        TreeChildNodes = pTreeNode->ChildNodes();
    }

    // удаление из списка дочерних узлов таких, которые не входят в правило
    // например в сумму входит синус, которого нет в правилах. Перебор синуса не нужен.
    void RemoveUnusedByRuleNodes(const ASTNodeList& RuleNodes)
    {
        TreeChildNodes.remove_if([&RuleNodes](const CASTNodeBase* p)
            {
                // не удаляем из дочерних узлов узлы, соответствующие Any
                return std::find_if(RuleNodes.begin(), RuleNodes.end(), [p](const CASTNodeBase* q)
                    {
                        //return q->CheckType(ASTNodeType::Any) || q->CheckType(p->GetType());
                        return q->CheckType(ASTNodeType::Any, p->GetType());
                    }) == RuleNodes.end();
            });

        StartPermutations();
    }

    // вычисление следующей комбинации дочерних узлов
    bool NextPermutation()
    {
        // если оператор n-арный (сумма или умножение), то делаем 
        if (!CASTNodeBase::IsFlatOperator(ptreeNode))
            return false;

        // комбинация строится в рамках цепочки permutation chain
        bool bNextPerm = std::next_permutation(order.begin(), order.end());
        // приводим порядок дочерних узлов к новому порядку индексов
        ToOrder();
        return bNextPerm;
    }

    const ASTNodeList& ChildNodes()
    {
        return TreeChildNodes;
    }
    
    const CASTNodeBase* TreeNode()
    {
        return ptreeNode;
    }
};

// отображение узлов дерева на узел с возможностью перебора дочерних узлов
using TreeNodeMap = std::map<const CASTNodeBase*, CTreeNodeInfo>;

// соответствие узла правила узлу дерева
class CMatchInfo
{
    TreeNodeMap::iterator itTreeNodeInfo;   
    CASTNodeBase* pruleNode;
public:
    CMatchInfo(CASTNodeBase* pRuleNode, TreeNodeMap::iterator TreeNodeInfo) :
        pruleNode(pRuleNode), itTreeNodeInfo(TreeNodeInfo)
    {
     
    }
    CTreeNodeInfo& NodeInfo() { return itTreeNodeInfo->second; }
    CASTNodeBase* RuleNode() { return pruleNode; }
};

// список соответствий узлов правил узлам дерева
class CMatchRuleNodeToTreeNode
{
    TreeNodeMap MatchMap;
    std::map<CASTNodeBase*, CTreeNodeInfo*> RuleNodeToTreeNode;
public:

    // на вход принимаем узел исходного дерева и узел дерева правил
    CMatchInfo MatchInfo(const CASTNodeBase* pTreeNode, CASTNodeBase* pRuleNode)
    {
        auto mit = MatchMap.find(pTreeNode); // находим узел дерева в отображении на правила
        if (mit == MatchMap.end())
        {
            // если не нашли - создаем новое отображение с новым перебором дочерних узлов
            mit = MatchMap.insert(std::make_pair(pTreeNode, CTreeNodeInfo(pTreeNode))).first;
            // очищаем перебор от лишних узлов и инициализируем перебор
            mit->second.RemoveUnusedByRuleNodes(pRuleNode->ChildNodes());
        }
        // обратное отображение узла правил на узел исходного дерева
        RuleNodeToTreeNode[pRuleNode] = &mit->second;
        return CMatchInfo(pRuleNode, mit);
    }

    // вычисление следующей комбинации всех узлов дерева для правил, поддерживающих 
    // комбинации. Вычисление идет снизу вверх. Когда для верхнего узла все комбинации
    // будут исчерпаны, возвращает false
    bool NextPermutation(ASTNodeList& PermutationsChain)
    {
        bool bNextPermutation = false;
        for (auto it = PermutationsChain.rbegin(); it != PermutationsChain.rend(); it++)
        {
            auto rit = RuleNodeToTreeNode.find(*it);
            if (rit != RuleNodeToTreeNode.end())
            {
                bNextPermutation = rit->second->NextPermutation();
                if (!bNextPermutation)
                    continue;
                else
                    break;
            }
        }
        return bNextPermutation;
    }
};