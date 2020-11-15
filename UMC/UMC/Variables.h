#pragma once
#include <string>
#include <map>

// �������� ��� ���������� �� ����, ������� �����
// ���������������� �����
class CASTNodeBase;
class CASTEquation;
class CASTVariable;
class CASTRoot;
class CASTHostBlockBase;
using ASTNodeList = std::list<CASTNodeBase*>;


// ��� �����, � �������������� �������� ��� ��������
// ����������� ��������� ����
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


// ������ ����������
struct VariableInfo
{
    ASTEquationsSet Equations;                                      // ��� ���������, � ������� ������ ����������   
    ASTVariablesSet VarInstances;                                   // ��� ����������� ���������� � ������ ������
    CASTEquation* pResovledByEquation = nullptr;                    // ���������, ������� ��������� ������ ����������
    CASTEquation* pInitByEquation = nullptr;                        // ���������, ������� �������������� ������ ����������
    CASTHostBlockBase* pHostBlockOutput = nullptr;                  // ��������� �� ����-����, ���� ���������� �������� ��� �������
	bool GeneratedByCompiler = false;                               // ���� ����, ��� ���������� �� ������ �������������
	bool FromSource = false;                                        // ���� ����, ��� ���������� ����� �� ���������
    bool Constant = false;                                          // ���� ����, ��� ���������� �������� ����������
    bool NamedConstant = false;                                     // ���� ����, ��� ���������� �������� ����������� ���������� �������� ������
    bool External = false;                                          // ���� ����, ��� ���������� �������� �������
    bool ExternalBase = false;                                      // ���� ����, ��� ���������� ������� ������������ ��� �������� ������� ���������� � ��������� ��������
    bool InitSection = false;                                       // ���� ����, ��� ���������� ������� � Init ������
    std::string ModelLink;                                          // ������ �� ������� ���������� � ������� table[key,...,key].value


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

// ����� ����������, ���������� ��� ������
using VarInfoMap = std::map<std::string, VariableInfo>;

// ���������� ���������� �� ���������� �����
struct VarMapIteratorCompare
{
    bool operator() (const VarInfoMap::iterator& lhs, const VarInfoMap::iterator& rhs) const
    {
        return lhs->first < rhs->first;
    }
};

// ��� ���������� �� ����� ���������� ������
using VarInfoSet = std::set<VarInfoMap::iterator, VarMapIteratorCompare>;
// ������ ���������� �� ����� ���������� ������ ��� ������ � ������� ����-������
using VarInfoList = std::list<VarInfoMap::iterator>;
