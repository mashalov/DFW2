#pragma once
#include "ASTNodeBase.h"
#include "ASTTreeBase.h"
#include "ASTNodes.h"

// helper для анализа паттерна простого уравнения вида [-]a+[-]b=0
struct CASTTreeBase::SimpleEquation
{
    struct Term
    {
        CASTNodeBase* pUminus = nullptr;    // наличие унарного минуса
        CASTNodeBase* pNode = nullptr;      // узел [a] или [b], см. выше

        // проверка части уравнения на соответствие паттерну
        bool Check(CASTNodeBase* pInput, ASTNodeType Type)
        {
            if (CASTNodeBase::IsUminus(pInput))
            {
                // если есть унарный минус, проверяем его на количество
                // дочерних узлов и запоминаем.
                // узел снимаем с унарного минуса
                pUminus = pInput;
                pUminus->CheckChildCount();
                pNode = pUminus->ChildNodes().front();
            }
            else
                pNode = pInput; // если унарного минуса нет - берем узел напрямую

            // возвращаем true, если выделили узел и его тип соответствует заданному
            return pNode != nullptr && pNode->CheckType(Type);
        }

        // helper для удобного приведения типа найденного узла
        template<typename T>
        T* Node() { return static_cast<T*>(pNode); }
    }
    left, right;

    // узлов два - левый и правый, каждый со своим унарным минусом

    CASTEquation* pEquation = nullptr;  // указатель на уравнение, в котором нашли паттерн

    // проверка уравнения на заданный паттерн
    bool Check(CASTEquation* eq, ASTNodeType LeftType, ASTNodeType RightType)
    {
        // очищаем
        *this = SimpleEquation();
        // проверяем структуру уравнения
        eq->CheckChildCount();
        // выделяем левую и правую части уравнения
        CASTNodeBase* pLeft = eq->ChildNodes().front();
        CASTNodeBase* pRight = eq->ChildNodes().back();
        // если уравнение слева имеет сумму с двумя дочерними узлами, а справа численный ноль
        // то это подходящий паттерн, снимаем левый и правый узлы с суммы и проверяем соответствие их типов заданным
        if (CASTNodeBase::IsSum(pLeft) && CASTNodeBase::IsNumericZero(pRight) && pLeft->ChildNodes().size() == 2)
            if (left.Check(pLeft->ChildNodes().front(), LeftType) && right.Check(pLeft->ChildNodes().back(), RightType))
                pEquation = eq;
        // если паттерн и типы совпадают - возвращаем true и запоминаем уравнение
        return pEquation != nullptr;
    }

    // проверка соответствия знаков
    bool Uminus()
    {
        // возвращаем true, если знаки слева и справа совпадают
        // для уравнения паттерна это означает, что если перенести правый узел в
        // правую часть равенства - в равенстве будет минус

        //  -a - b = 0  ->  -a =  b   -   true
        //  -a + b = 0  ->  -a = -b   -   false
        //   a - b = 0  ->   a =  b   -   false
        //   a + b = 0  ->   a = -b   -   true

        return (left.pUminus != nullptr) == (right.pUminus != nullptr);
    }
};
