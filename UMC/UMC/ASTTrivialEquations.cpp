#include "ASTSimpleEquation.h"
#include "ASTNodes.h"

// проверяет, является ли именованная перменная аргументом хост-блока
bool CASTTreeBase::IsHostBlockChildVariable(std::string_view VarName) const
{
    bool bRes(false);
    for (auto& h : HostBlocks)
        if (h->IsChildVariable(VarName))
        {
            bRes = true;
            break;
        }
    return bRes;
}

bool CASTTreeBase::IsEquationTrivial(CASTEquation* pEquation, SimpleEquation& substitution)
{
    // проверяем уравнение вида переменная + число = 0
    pEquation->GetInfix();
    if (substitution.Check(pEquation, ASTNodeType::Variable, ASTNodeType::Numeric))
    {
        // если переменная входит в исходный текст или является аргументом хост блока - не трогаем ее
        if (!substitution.left.Node<CASTVariable>()->Info().FromSource &&
            !IsHostBlockChildVariable(substitution.left.pNode->GetText()))
            return true;
    } // проверяем уравнение вида переменная + функция-константа
    else if (substitution.Check(pEquation, ASTNodeType::Variable, ASTNodeType::FnTime))
    {
        if (!substitution.left.Node<CASTVariable>()->Info().FromSource &&
            !IsHostBlockChildVariable(substitution.left.pNode->GetText()))
            return true;
    } // проверяем уравнение переменная + переменная = 0
    else if (substitution.Check(pEquation, ASTNodeType::Variable, ASTNodeType::Variable))
    {
        // проверяем, не является ли переменная, которую мы хотим заменить
        // переменной из исходного текста
        // если да, то проверяем, не является ли переменная на которую мы хотим
        // ее заменить искусственной
        // если да, меняем переменные местами

        if (substitution.left.Node<CASTVariable>()->Info().FromSource && 
            !substitution.right.Node<CASTVariable>()->Info().FromSource)
            std::swap(substitution.left, substitution.right);

        // проверяем не входит ли заменяемая переменая в какой-нибудь хост-блок в качестве входа
        // если да - проверяем ввод минуса и замену на константу. Если что-то из этого требуется - 
        // блокируем замену, так как в хост-блок нужно передавать значение переменной
        // без преобразования, а константные входы ранее были заменены на переменные

        if ((substitution.Uminus() || substitution.right.pNode->IsConst()) && 
            IsHostBlockChildVariable(substitution.left.pNode->GetText()))
            return false;

        for (auto& h : HostBlocks)
        {
            // дальше проверяем выходную переменную
            // нужно чтобы уравнение имело вид Var = HostBlock, или Var - HostBlock = 0
            // если переменная Var заменяется на некоторую Var1 с минусом, то блокируем замену
            SimpleEquation hostCheck;
            // проверяем уравнение блока на паттерн Var - HostBlock
            if (hostCheck.Check(h->GetParentEquation(), ASTNodeType::Variable, h->GetType()))
            {
                // если имя переменной совпадает с заменяемой
                if (hostCheck.left.pNode->GetText() == substitution.left.pNode->GetText())
                {
                    _ASSERTE(!hostCheck.left.pUminus);
                    // и при замене переменной мы должны будем ввести в уравнение блока минус - блокируем замену
                    if (substitution.Uminus()) // то блокируем замену
                        return false;
                }
            }
        }

        if (substitution.left.Node<CASTVariable>()->Info().FromSource && substitution.right.Node<CASTVariable>()->Info().FromSource)
        {
            // если обе переменных из исходника, это скорее всего ошибка, но
            // мы не удаляем это уравнение, так как одна из переменных, заданных пользователем,
            // будет потеряна

            // ????????? тут еще может быть проверка на то, что одна из переменных является внешней ???????????
            // !!!!!!!!! еще нужно сохранять уравнения для выходных переменных примитивов !!!!!!!!!

            if (!TrivialEquationsReported.Contains(pEquation))
            {
                // выдаем предупреждение по тривиальному уравнению с пользовательскими переменными 1 раз
                Warning(fmt::format(u8"Тривиальное уравнение с исходными переменными {}",
                    static_cast<CASTEquation*>(pEquation)->GetEquationDescription()));
                TrivialEquationsReported.insert(pEquation);
            }

            // блокируем замену переменных для переменных из исходника
            return false;
        }

        return true;
    }

    return false;
}

bool CASTTreeBase::FindTrivialEquation(CASTEquationSystem* pSystem, SimpleEquation& substitution)
{
    if (!pSystem->CheckType(ASTNodeType::EquationSystem))
        EXCEPTIONMSG("pSystem is not Equation System");

    for (auto eq : pSystem->ChildNodes())
    {
        if (IsEquationTrivial(static_cast<CASTEquation*>(eq), substitution))
            return true;
    }
    return false;
}

void CASTTreeBase::RemoveTrivialEquations(CASTEquationSystem* pSystem)
{
    if (ErrorCount() != 0) return;

    SimpleEquation substitution;
    Collect();
    while (FindTrivialEquation(pSystem, substitution))
    {
        // находим тривиальное уравнение и в системе заменяем
        // переменные
        DFSPost(pSystem, [&substitution](CASTNodeBase* p)
            {
                if (CASTNodeBase::IsVariable(p))
                {
                    CASTVariable* pVar = static_cast<CASTVariable*>(p);
                    CASTNodeBase* pVarParent = pVar->GetParent();
                    if (substitution.left.pNode->GetText() == pVar->GetText())
                    {
                        // находим итератор заменяемого
                        auto itdel = pVarParent->FindChild(p);
                        // делаем клон того, на что заменяем на родителя. У него теперь лишний дочерний узел
                        auto itnew = pVarParent->FindChild(substitution.right.pNode->Clone(pVarParent));
                        // меняем местами заменяемый и клон
                        std::swap(*itdel, *itnew);
                        // удаляем заменяемый
                        pVarParent->DeleteChild(itnew);
                        //pVar->GetParent()->ReplaceChild<CASTVariable>(p, substitution.right.pNode->GetText());
                        // если в подстановке переменных нужен минус, создаем промежуточный CASTUminus
                        if (substitution.Uminus())
                            pVarParent->InsertItermdiateChild<CASTUminus>(*itdel);
                    }
                }

                return true;
            }
        );

        pSystem->DeleteChild(substitution.pEquation);
        Collect();
    }

    // связываем блоки и выходные переменные блоков
    for (auto& h: HostBlocks)
    {
        auto ParentEq = h->GetParentEquation();
        ParentEq->pHostBlock = h;
        if (substitution.Check(ParentEq, ASTNodeType::Variable, h->GetType()))
        {
            substitution.left.Node<CASTVariable>()->Info().pHostBlockOutput = h;
            h->Outputs.push_back(Vars.find(std::string(substitution.left.pNode->GetText())));
        }
    }
}
