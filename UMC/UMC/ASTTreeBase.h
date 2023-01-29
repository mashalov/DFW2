/*
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

#pragma once
#include <list>
#include <set>
#include <stack>
#include <set>
#include <map>
#include <string>
#include <string_view>
#include <sstream>
#include <array>
#include <functional>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include "fmt/core.h"
#include "fmt/format.h"

#include "utils.h"
#include "Variables.h"
#include "ASTNodeTypes.h"
#include "MessageCallbacks.h"


class CASTTreeBase;
class PropertyMap : public std::map<std::string, std::string, std::less<>>
{
protected:
    CASTTreeBase* pTree = nullptr;
    using std::map<std::string, std::string, std::less<>>::map;
public:
    static inline constexpr const char* szPropOutputPath = "OutputPath";
    static inline constexpr const char* szPropProjectName = "ProjectName";
    static inline constexpr const char* szPropExportSource = "ExportSource";
    static inline constexpr const char* szPropReferenceSources = "ReferenceSources";
    static inline constexpr const char* szPropDllLibraryPath = "DllLibraryPath";
    static inline constexpr const char* szPropConfiguration = "Configuration";
    static inline constexpr const char* szPropPlatform = "Platform";
    static inline constexpr const char* szPropRebuild = "Rebuild";
    static inline constexpr const char* szPropCachedModulesCount = "CachedModulesCount";
};

using ASTFragmentsMap  = std::map<std::string, ASTNodeSet>;
struct ASTFragmentInfo
{
    std::string infix;
    ASTNodeSet fragmentset;
    size_t score;
    ASTFragmentInfo(std::string_view Infix, ASTNodeSet& FragmentSet);
};
using ASTFragmentsList = std::list<ASTFragmentInfo>;
using ASTNodeConstStack = std::stack<const CASTNodeBase*>;
using DFSVisitorFunction = std::function<bool(CASTNodeBase*)>;

class CASTEquationSystem;
class CASTJacobiElement;
class CASTJacobian;

class CASTTreeBase
{
public:
    struct SimpleEquation;
protected:
    PropertyMap& properties;
    MessageCallBacks messageCallBacks;
	ASTNodeList NodeList;
    bool bChanged = true;
    void Unchange() { bChanged = false; }
    std::unique_ptr<CASTTreeBase> pRuleTree;
    CASTNodeBase* pRoot = nullptr;
    CASTJacobian* pJacobian = nullptr;
    CASTEquationSystem* pExternalBaseInit = nullptr; // дополнительная система уравнений для инициализации Base переменных
    void MatchAtRuleEntry(const CASTNodeBase* pRuleEntry, const CASTNodeBase* pRule);
    mutable size_t nErrors = 0, nWarnings = 0;      // функции репортинга делаем const, но счетчики хотим инкрементировать
    VarInfoMap Vars;                                // карта переменных дерева
    ASTEquationsSet Equations;                      // сет уравнений в дереве
    ASTHostBlocksSet HostBlocks;                    // сет хост-блоков

    ptrdiff_t DFSLevel_ = 0;                        // номер DFS-обхода (для вложенных обходов)

    using VarItList = std::list<VarInfoMap::iterator>;

    template<typename T, typename... Args>
    T* CreateNodeImpl(CASTNodeBase* pParent, Args... args)
    {
        T* pNode = new T(this, pParent, args...);
        NodeList.push_back(pNode);
        Change();
        return pNode;
    }

    ASTNodeConstSet TrivialEquationsReported; // сет для индикации однократного предупреждения о тривиальных уравнениях
    void ChainVariablesToEquations();
    ASTEquationsList::iterator TieBreak(ASTEquationsList& eqs);
    ASTEquationsList::iterator ResolveEquation(ASTEquationsList& eqs);
    void SortEquationSection(ASTNodeType SectionType);
    void CheckInitEquations();
    void TransformToOperatorTree(CASTEquationSystem *pSystem);
    void RemoveTrivialEquations(CASTEquationSystem* pSystem);
    bool IsHostBlockChildVariable(std::string_view VarName) const;
    bool IsEquationTrivial(CASTEquation* pEquation, SimpleEquation& substitution);
    bool FindTrivialEquation(CASTEquationSystem* pSystem, SimpleEquation& substitution);
    void GenerateJacobian(CASTEquationSystem* pSystem);
    CASTJacobiElement* CreateDerivative(CASTJacobian* pJacobian, CASTEquation* pEquation, VarInfoMap::iterator itVariable);
    void GenerateInitEquations();
    void ProcessProxyFunction();
    void ProcessConstantArgument();
    void InitExternalBaseVariables();
    std::string ProjectMessage(const std::string_view& message) const;

public:

    CASTVariable* CreateVariableFromModelLink(std::string_view ModelLink, CASTNodeBase* Parent);
    CASTVariable* CreateBaseVariableFromModelLink(std::string_view ModelLink, CASTNodeBase* Parent);

    void Change() 
    { 
        bChanged = true; 
    }
    bool IsChanged() const { return bChanged; }
    CASTTreeBase(PropertyMap& Properties);


    template<typename T, typename... Args>
    T* CreateNode(CASTNodeBase* pParent, Args... args)
    {
        return CreateNodeImpl<T>(pParent, args...);
    }

    template<typename T>
    T* CloneNode(CASTNodeBase* pParent, T* pCloneNode)
    {
        T* pNode = new T(this, pParent, pCloneNode);
        NodeList.push_back(pNode);
        Change();
        return pNode;
    }

    void DeleteNode(CASTNodeBase* pNode);
    void DFS(CASTNodeBase* Root, DFSVisitorFunction Visitor);
    void DFSPost(CASTNodeBase* Root, DFSVisitorFunction Visitor);
    void PostParseCheck();
    void Flatten();
    void Collect();
    void Sort();
    void CreateRules();
    void RemoveSingleFragments(ASTFragmentsMap& fragments) const;
    void Match(const CASTNodeBase* pRule);
    const CASTNodeBase* GetRule();
    size_t Score();
    void PrintInfix();
    void ApplyHostBlocks();
    void PrintErrorsWarnings() const;
    void Error(std::string_view error) const;
    void Warning(std::string_view warning) const;
    void Message(std::string_view message) const;
    void Information(std::string_view message) const;
    void Debug(std::string_view message) const;
    size_t ErrorCount() const;
    VariableInfo& GetVariableInfo(std::string_view VarName);
    VariableInfo* CheckVariableInfo(std::string_view VarName);
    ASTEquationsSet& GetEquations();
    ASTHostBlocksSet& GetHostBlocks();
    virtual ~CASTTreeBase();
    const VarInfoMap& GetVariables() const;
    void GenerateEquations();
    CASTJacobian* GetJacobian();
    CASTEquationSystem* GetEquationSystem(ASTNodeType EquationSystemType);
    std::string GenerateUniqueVariableName(std::string_view Prefix);
    std::string FindModelLinkVariable(std::string_view ModelLink);
    void SetMessageCallBacks(MessageCallBacks& MessageCallBackFunctions)
    {
        messageCallBacks = MessageCallBackFunctions;
    }
    std::filesystem::path GetPropertyPath(std::string_view PropNamePath);
    const PropertyMap& GetProperties() const { return properties; }
};


// special version for root creation in ASTTreeBase.cpp
template<> CASTRoot* CASTTreeBase::CreateNode<CASTRoot>(CASTNodeBase* pParent);

// реализация стека на резервированном векторе 
// без лишних аллокаций
using ASTNodeVec = std::vector<CASTNodeBase*>;
class CDFSStack : protected ASTNodeVec
{
protected:
    //size_t nMaxTop = 0; // наибольшая вершина стека
public:
    CDFSStack(size_t nCount)
    {
        ASTNodeVec::reserve(nCount);
    }
    void push(CASTNodeBase* pNode)
    {
        ASTNodeVec::push_back(pNode);
        //nMaxTop = (std::max)(nMaxTop, ASTNodeVec::size());
    }
    void pop()
    {
        ASTNodeVec::pop_back();
    }
    CASTNodeBase* top()
    {
        return ASTNodeVec::back();
    }
    bool empty() const
    {
        return ASTNodeVec::empty();
    }
};