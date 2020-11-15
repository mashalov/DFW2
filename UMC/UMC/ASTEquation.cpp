#include "ASTNodes.h"
#include "ASTSimpleEquation.h"

CASTEquation::Resolvable CASTEquation::IsResolvable(VarInfoSet::iterator& itResolvingVariable)
{
	// нам нужно найти одну неразрешенную переменную, относительно которой можно разрешить уравнение
	itResolvingVariable = Vars.end();
	bool bOneUnresolved = true;
	for (auto itV = Vars.begin(); itV != Vars.end(); itV++)
	{
		if (!(*itV)->second.IsUnresolvedStateVariable()) continue;
		// если переменная не была пока разрешена
		// запоминаем ее
		if (itResolvingVariable != Vars.end())
		{
			// если уже нашли неразрешенную переменную и есть еще одна - 
			// уравнение не может быть разрешено
			bOneUnresolved = false;
			break;
		}
		itResolvingVariable = itV;
	}

	if (itResolvingVariable == Vars.end())
	{
		// если неразрешенных переменных не нашли - уравнение уже разрешено
		return CASTEquation::Resolvable::Already;
	}
	else
	{
		if (bOneUnresolved)
			return CASTEquation::Resolvable::Yes;       // если неразрешенная переменная только одна - разрешить можно
		else
		{
			itResolvingVariable = Vars.end();
			return CASTEquation::Resolvable::No;        // если более одной неразрешенной переменной - разрешить нельзя
		}
	}
}

VarInfoSet::iterator CASTEquation::Resolve()
{
	// проверяем количество дочерних узлов уравнения
	CheckChildCount();
	// пока не определили, что уравнение можно разрешить,
	// итератор переменной по которой возможно разрешение
	// ставим в end
	VarInfoSet::iterator itUnresolved = Vars.end();
	// если уравнение не разрешаемо (не одна неизвестная переменная или известны все) -
	// выходим
	if (IsResolvable(itUnresolved) != CASTEquation::Resolvable::Yes)
		return itUnresolved;

	// получаем левую и правую части уравнения
	CASTNodeBase* pLeft = Children.front();
	CASTNodeBase* pRight = Children.back();
	// если в левой части уравнения переменная, относительно которой можно разрешить - то 
	// уравнение уже готово
	VarInfoMap::iterator itVar = *itUnresolved;
	if (IsVariable(pLeft) && pLeft->GetText() == itVar->first)
		return itUnresolved;

	// если уравнение не сумма, равная нулю - ошибка
	if(!(IsSum(pLeft) && IsNumericZero(pRight))) 
		EXCEPTIONMSG("Equation has wrong pattern");

	// ищем элемент уравнения, связанный с единственной неизвестной
	CASTNodeBase* pVar = nullptr;
	CASTNodeBase* pUminus = nullptr;
	// просматриваем инстансы разрешенной переменной, один из которых
	// может входит в данное уравнение


	for (auto& v : itVar->second.VarInstances)
		if (v->GetParentEquation() == this)
		{
			// если переменная принадлежит
			// к даному уравнению - переменную нашли
			pVar = v;
			// проверяем, нет ли у переменной унарного минуса
			// в качестве родителя. Если есть - элемент будет
			// унарный минус с данной переменной. Если нет - просто переменная

			// по хорошему можно просто убрать далее Term::Check();
			if (pVar->GetParentOfType(ASTNodeType::Uminus))
				pUminus = pVar->GetParent();
			break;
		}

	if (!pVar)
		EXCEPTIONMSG("Variable not found");

	// если паттерн подходит, меняем местами
	// левую и правую части уравнения
	std::swap(Children.front(), Children.back());
	// если переменная с унарным минусом, слагаемое минус переменная, иначе - просто переменная
	CASTNodeBase* pTerm = pUminus ? pUminus : pVar;
	// извлекаем слагаемое из суммы
	pLeft->ExtractChild(pTerm);
	// заменяем левую часть уравнения на слагаемое
	ReplaceChild(Children.front(), pTerm);
	// далее нам нужно выставить знаки
	//  a + sum = 0    ->  a = -sum
	// -a + sum = 0	   -> -a = -sum == a = sum
	// если слагаемое с минусом
	// то просто убираем минус у слагаемого. Оставляем переменную
	// если слагаемое с плюсом - ставим унарный минус у суммы справа
	if (pUminus)
		ReplaceChild(pUminus, pUminus->ExtractChild(pVar)); 
	else
		InsertItermdiateChild<CASTUminus>(pLeft); 
	// отмечаем, что переменная разрешается данным уравнением
	(*itUnresolved)->second.pInitByEquation = this;
	// отмечаем, что уравнение инициализирует найденную переменную
	itInitVar = *itUnresolved;
	// если есть хост-блок, входящий в это уравнение - 
	// отмечаем уравнение как уравнение хост-блока
	for (auto& h : pTree->GetHostBlocks())
		if (h->GetParentEquation() == this)
		{
			pHostBlock = h;
			break;
		}
	return itUnresolved;
}