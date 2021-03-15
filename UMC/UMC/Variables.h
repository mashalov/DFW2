#pragma once
#include <string>
#include <map>

// форварды для указателей на типы, которые будут
// имплементированы позже
class CASTNodeBase;
class CASTEquation;
class CASTVariable;
class CASTRoot;
class CASTHostBlockBase;
using ASTNodeList = std::list<CASTNodeBase*>;


// сет узлов, с дополнительной функцией для быстрого
// определения вхождения узла
template<class T>
class ASTNodeSetT : public std::set<T*>
{
public:
    bool Contains(T* pNode) const
    {
        return ASTNodeSetT<T>::find(pNode) != ASTNodeSetT<T>::end();
    }
};

using ASTNodeConstSet = ASTNodeSetT<const CASTNodeBase>;
using ASTNodeSet = ASTNodeSetT<CASTNodeBase>;
using ASTEquationsSet = ASTNodeSetT<CASTEquation>;
using ASTVariablesSet = ASTNodeSetT<CASTVariable>;
using ASTHostBlocksSet = ASTNodeSetT<CASTHostBlockBase>;
using ASTEquationsList = std::list<CASTEquation*>;


// данные переменной
struct VariableInfo
{
    ASTEquationsSet Equations;                                      // сет уравнений, в которые входит переменная   
    ASTVariablesSet VarInstances;                                   // сет экземпляров переменной с данным именем
    CASTEquation* pResovledByEquation = nullptr;                    // уравнение, которое разрешает данную переменную
    CASTEquation* pInitByEquation = nullptr;                        // уравнение, которое инициализирует данную переменную
    CASTHostBlockBase* pHostBlockOutput = nullptr;                  // указатель на хост-блок, если переменная является его выходом
	bool GeneratedByCompiler = false;                               // флаг того, что переменная не задана пользователем
	bool FromSource = false;                                        // флаг того, что переменная взята из исходника
    bool Constant = false;                                          // флаг того, что переменная является константой
    bool NamedConstant = false;                                     // флаг того, что переменная является именованной константой исходных данных
    bool External = false;                                          // флаг того, что переменная является внешней
    bool ExternalBase = false;                                      // флаг того, что переменная введена компилятором для значения внешней переменной в начальных условиях
    bool InitSection = false;                                       // флаг того, что переменная введена в Init секции
    std::string ModelLink;                                          // ссылка на внешнюю переменную в формате table[key,...,key].value


    bool IsMainStateVariable() const
    {
        return IsStateVariable() && !InitSection;
    }

    bool IsStateVariable() const
    {
        return !IsExternal() && !Constant && VarInstances.size() > 0;
    }

    bool IsUnresolvedStateVariable() const
    {
        return IsStateVariable() && pResovledByEquation == nullptr;
    }

    bool IsNamedConstant() const
    {
        return !ExternalBase && Constant && NamedConstant && VarInstances.size() > 0;
    }

    bool IsConstant() const
    {
        return ( ExternalBase || Constant ) && VarInstances.size() > 0;
    }

    bool IsExternal() const
    {
        return External;
    }
};

// карта переменных, глобальная для дерева
using VarInfoMap = std::map<std::string, VariableInfo>;

// сравнитель переменных по итераторам карты
struct VarMapIteratorCompare
{
    bool operator() (const VarInfoMap::iterator& lhs, const VarInfoMap::iterator& rhs) const
    {
        return lhs->first < rhs->first;
    }
};

// сет итераторов на карту переменных дерева
using VarInfoSet = std::set<VarInfoMap::iterator, VarMapIteratorCompare>;
// список итераторов на карту переменных дерева для входов и выходов хост-блоков
using VarInfoList = std::list<VarInfoMap::iterator>;
