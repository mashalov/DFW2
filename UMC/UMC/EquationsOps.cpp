#include "ASTNodes.h"
#include "MatchInfo.h"
#include "ASTSimpleEquation.h"

ASTEquationsList::iterator CASTTreeBase::TieBreak(ASTEquationsList& eqs)
{
    ASTEquationsList::iterator itEq = eqs.end();

    // мы знаем что в этой точке у нас есть n-уравнений для n-переменных
    // и мы должны решать это как систему
    // находим переменную, которая входит в максимальное количество уравнений
    auto Pred = [&eqs](const ASTEquationsSet::value_type& val)
    {
        return std::find(eqs.begin(), eqs.end(), val) != eqs.end() ; 
        // предикат для подсчета количества неразрешенных уравнений
        // тут надо неучитывать еще уравнения из которых уже был сделан tiebreak
        // можно попробовать заменить поиск в списке на анализ itResolvedBy - (разрешено) 
        // и itTieBrokenBy - (разорвано). Если ни одно из них - то уравнение не разрешено
    };

    // ищем переменную, которая входит в максимальное количество неразрешенных уравнений

    VarInfoMap::iterator itMax = Vars.end();
    for (auto it = Vars.begin(); it != Vars.end(); it++)
    {
        // и при этом сама не разрешена
        if (!it->second.IsUnresolvedStateVariable())
            continue;

        if (itMax == Vars.end() ||
            std::count_if(itMax->second.Equations.begin(), itMax->second.Equations.end(), Pred) <
            std::count_if(it->second.Equations.begin(), it->second.Equations.end(), Pred))
            itMax = it;
    }

    // если переменную нашли
    if (itMax != Vars.end())
    {
        auto Pred2 = [](const VarInfoSet::value_type& p)
        {
            return p->second.IsUnresolvedStateVariable();
        };

        auto& eqss = itMax->second.Equations;
        // ищем для нее уравнение с маскимальным количеством неразрешенных переменных
        auto itMaxEq = eqss.end();
        for (auto it = eqss.begin(); it != eqss.end(); it++)
        {
            CASTEquation* eq = *it;
            if (eq->itResolvedBy == Vars.end())
            {
                if (itMaxEq == eqss.end() ||
                    std::count_if((*itMaxEq)->Vars.begin(), (*itMaxEq)->Vars.end(), Pred2) <
                    std::count_if(eq->Vars.begin(), eq->Vars.end(), Pred2))
                    itMaxEq = it;
            }
        }

        if (itMaxEq != eqss.end())
        {
            // если нашли - то разрешаем эту переменную относительно этого уравнения
            itMax->second.pResovledByEquation = *itMaxEq;
            std::cout << "tiebreak on " << itMax->first << " " << itMax->second.pResovledByEquation->GetInfix() << std::endl;
            itEq = std::find(eqs.begin(), eqs.end(), *itMaxEq);
            // отмечаем, что уравнение был разорвано по переменной itMax
            (*itMaxEq)->itTieBrokenBy = itMax;
        }
    }

    return itEq;
}


// выбирает уравнение, которое можно разрешить
ASTEquationsList::iterator CASTTreeBase::ResolveEquation(ASTEquationsList& eqs)
{
    ASTEquationsList::iterator itResolveEq = eqs.end();

    for (auto itEq = eqs.begin(); itEq != eqs.end(); itEq++)
    {
        CASTEquation* eq = *itEq;

        _ASSERTE(eq->itResolvedBy == Vars.end());

        VarInfoSet::iterator itUnresolved = eq->Vars.end();
        // если нашли одну неразрешенную переменную - разрешаем ее и уравнение
        if (eq->GetParentOfType(ASTNodeType::Init))
        {
            itUnresolved = eq->Resolve();
        }
        else
        {
            if (eq->IsResolvable(itUnresolved) != CASTEquation::Resolvable::Yes)
                itUnresolved = eq->Vars.end();
        }

        if (itUnresolved != eq->Vars.end())
        {
            // отмечаем, что переменная разрешается данным уравнением
            CASTEquation*& pResolvedBy = (*itUnresolved)->second.pResovledByEquation;
            std::cout << "Resolved " << (*itUnresolved)->first << " from " << eq->GetInfix() << std::endl;
            _ASSERTE(!pResolvedBy);
            pResolvedBy = eq;
            itResolveEq = itEq;
            eq->itResolvedBy = *itUnresolved;
            break;
        }
    }
    return itResolveEq;
}

/*
void CASTTreeBase::GetUnresolvedVariables(CASTEquation* eq, VarItList& Unresolved)
{
    _ASSERTE(!eq->bResolved);

    Unresolved.clear();
    // проверяем, нет ли переменных, которые разрешены из этого уравнения
    // (последствия tiebreak

    // просматриваем переменные уравнения

    if (Unresolved.empty())
    {
        // в уравнении все переменные известны, но уравнение не разрешено
        std::list<std::string> vareqs;
        bool bRedundantEquation = true;
        for (auto& v : eq->Vars)
        {
            if (eq == v->second.pResovledByEquation)
            {
                bRedundantEquation = false;
                break;
            }

            vareqs.push_back(fmt::format("{} -> {} {}",
                v->first,
                trim(v->second.pResovledByEquation->GetInfix()),
                v->second.pResovledByEquation->GetEquationSourceDescription()));
        }
        if (bRedundantEquation)
        {
            Error(fmt::format("Избыточное уравнение {}.\nВсе переменные уже разрешены:\n{}",
                eq->GetEquationDescription(),
                fmt::join(vareqs, "\n")));
        }
    }
    else
    // если список состоит из одной переменной, ее можно разрешить из уравнения
}
*/

void CASTTreeBase::SortEquationSection(ASTNodeType SectionType)
{
    if (ErrorCount() !=0) return;

    // находим секцию системы уравнений 
    auto pEquationSection = GetEquationSystem(SectionType);

    // отбираем уравнения, который входят в заданную секцию
    ASTEquationsList eqs;

    for (auto& v : Vars)
        v.second.pResovledByEquation = nullptr;
   
    for (auto& e : pEquationSection->ChildNodes())
    {
        if (e->CheckType(ASTNodeType::Equation))
        {
            CASTEquation* pEquation(static_cast<CASTEquation*>(e));
            eqs.push_back(pEquation);
        }
    }

    // если уравнений в секции нет - заканчиваем
    if (eqs.empty()) return;

    // проверяем, сколько переменных входит в систему уравнений
    VarInfoSet eqVars;
    for (auto& eq : eqs)
    {
        for (auto& eqVar : eq->Vars)
            if (eqVar->second.IsUnresolvedStateVariable())
                eqVars.insert(eqVar);
    }

    if (eqVars.size() != eqs.size())
    {
        Error(fmt::format("Количество уравнений {} не равно количеству переменных {} в секции \"{}\"",
            eqs.size(),
            eqVars.size(),
            pEquationSection->GetParent()->GetText()));
        return;
    }

    // можно проверять, нет ли уравнений с переменными, которые входят только в это уравнение. Тогда это уравнение
    // можно удалить

    // список итераторов на переменные, которые не могут быть разрешены
    VarItList Unresolved;
    // новый порядок уравнений в порядке разрешения переменных (execution order)
    ASTNodeList EquationsNewOrder;

    // работаем, пока все уравнения не будут разрешены
    while (eqs.size() > 0)
    {
        // ищем уравнение, которое можно разрешить
        auto itResolve = ResolveEquation(eqs);
        if (itResolve != eqs.end())
        {
            // если нашли, добавляем его в новый порядок уравнений
            // отмечаем как разрешенное (переменная, по которой разрешили отмечается внутри ResolveEquation)
            EquationsNewOrder.push_back(*itResolve);
            (*itResolve)->PrintInfix();
            eqs.erase(itResolve);
        }
        else
        {
            // если разрешимых уравенений нет - имеем алгебраический цикл
            // в секции Init он не допустим
            if (SectionType == ASTNodeType::Init)
                break;

            // если работаем в секции Main - пытаемся сделать tiebreak
            auto itTieBroken = TieBreak(eqs);
            if (itTieBroken != eqs.end())
            {
                // если Tiebreak выдал уравнение - разрешаем его, но не 
                // ставим флаг bResolved - так как уравнение лишь предполагается к разрешению
                EquationsNewOrder.push_back(*itTieBroken);
                (*itTieBroken)->PrintInfix();
                eqs.erase(itTieBroken);
            }
            else
                break;
        }
    }

    // проверяем количество отсортированных уравнений
    if (eqVars.size() != EquationsNewOrder.size())
    {
        // если количество уравнений инициализации не равно количеству уравнений в новом порядка
        // выводим список уравнений алгебраического цикла
        // это уравнения которые в инициализации, но не в новом порядка
        std::list<std::string>  AlgebraicLoop;
        for (auto& c : pEquationSection->ChildNodes())
        {
            if(!c->CheckType(ASTNodeType::Equation)) continue;
            // если уравнения из инициализации нет в новом порядке
            // добавляем его в список уравнений алгебраического цикла
            if (std::find(EquationsNewOrder.begin(), EquationsNewOrder.end(), c) == EquationsNewOrder.end())
            {
                AlgebraicLoop.push_back(c->GetInfix());
            }
        }
        // выводим сообщение об ошибке со списком уравнений алгебраического цикла
        Error(fmt::format("Алгебраический цикл в следующих уравнениях секции \"{}\": \n {}",
            pEquationSection->GetParent()->GetText(),
            fmt::join(AlgebraicLoop, "\n")));
    }
    else
    {
        // если все уравнения отсортированы в новый порядок
        // меняем порядок уравнений в инициализации на новый
        auto nit = EquationsNewOrder.begin();
        for (auto& c : pEquationSection->ChildNodes())
        {
            if (!c->CheckType(ASTNodeType::Equation)) continue;
            // проверяем, есть ли неразрешенные уравнения
            // (они могут быть из-за tiebreaking)
            CASTEquation* pEq = static_cast<CASTEquation*>(*nit);
            VarInfoSet::iterator itVar = pEq->Vars.end();
            // и если есть - проверяем их и пытаемся проверить все ли переменные разрешены
            // если да - то разрешаем уравнение. Разрешаем только разорванные по алгебраическому циклу уравнения
            if (pEq->itResolvedBy == Vars.end() && 
                pEq->itTieBrokenBy != Vars.end() && 
                pEq->IsResolvable(itVar) == CASTEquation::Resolvable::Already)
                pEq->itResolvedBy = pEq->itTieBrokenBy;
            _ASSERTE(pEq->itResolvedBy != Vars.end());
            c = *nit;
            nit++;
        }
    }

    // проверяем все ли переменные разрешены
    for (auto& var : eqVars)
        _ASSERTE(var->second.pResovledByEquation);
}

void CASTTreeBase::TransformToOperatorTree(CASTEquationSystem* pSystem)
{
    if (ErrorCount() != 0) return;

    // преобразуем обычное AST в развернутое AST, узлами которого являются отдельные операторы
    // параметры для лямбды DFS

    if (!pSystem->CheckType(ASTNodeType::EquationSystem))
        EXCEPTIONMSG("pSystem is not Equation System");

    struct EqSystems
    {
        CASTNodeBase* pSystem = nullptr;                         // указатели на текущую систему и на преобразованную систему
        CASTNodeBase* pNewSystem = nullptr;
        CASTTreeBase* pTree = nullptr;                              // указатель на дерево
    }
    eqs{ pSystem, CreateNode<CASTEquationSystem>(nullptr), this };

    // создаем новую пустую систему уравнений
    DFSPost(pSystem, [&eqs](CASTNodeBase* p)
        {
            CASTNodeBase* pParent(p->GetParent());
            if (CASTNodeBase::TrasformNodeToOperator(p))
            {
                // создаем новое имя переменной
                std::string strEqId = eqs.pTree->GenerateUniqueVariableName("eq");
                // в новой системе уравнений создаем новое уравнение
                auto pTreeEquation = eqs.pNewSystem->CreateChild<CASTEquation>();
                // создаем переменную нового уравнения (левая часть уравнения)
                auto pNewVariable = pTreeEquation->CreateChild<CASTVariable>(strEqId);
                pNewVariable->Info().GeneratedByCompiler = true;
                // находим данный узел в родителе
                auto thisNodeIt = pParent->FindChild(p);
                // проверяем есть ли он (заменить на throw fatal)
                if (thisNodeIt == pParent->ChildNodes().end())
                    EXCEPTIONMSG("This node is not found in parent");
                // создаем правую часть уравнения
                pTreeEquation->CreateChild(*thisNodeIt);
                // заменяем "себя" в родителе на новую переменную
                // задаем текст новой переменной, заменяющей этот оператор в родителе 
                // на идентичный переменной нового уравнения
                *thisNodeIt = eqs.pTree->CreateNode<CASTVariable>(pParent, strEqId);
            }
            else if (p->CheckType(ASTNodeType::Equation))
            {
                // если обходим уравнение - переносим его в новую систему
                auto eq = p->GetParent()->ExtractChild(p);
                eqs.pNewSystem->CreateChild(eq);
            }

            return true;
        });

    // заменяем старые системы уравнений на новые
    ASTNodeList children;
    eqs.pNewSystem->ExtractChildren(children);
    eqs.pNewSystem->Delete();
    eqs.pSystem->AppendChildren(children);
}

void CASTTreeBase::GenerateInitEquations()
{
    if (ErrorCount() != 0) return;

    auto pInit = GetEquationSystem(ASTNodeType::Init);
    auto pMain = GetEquationSystem(ASTNodeType::Main);
    for (auto& me : pMain->ChildNodes())
    {
        if (!me->CheckType(ASTNodeType::Equation)) continue;
        auto* pMainEq = static_cast<CASTEquation*>(me);
        pMainEq->CloneTree(pInit);
    }
}