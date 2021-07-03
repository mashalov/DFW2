#pragma once
#include "vector"
#include "string"
#include "set"
#include "IResultWriterABI.h"


class CSlowVariableGraphItem
{
public:
	double m_dTime;
	mutable double m_dValue;
	mutable std::string m_strDescription;
	CSlowVariableGraphItem(double dTime, double dValue, std::string_view ChangeDescription) : 
															m_dTime(dTime), 
															m_dValue(dValue),
															m_strDescription(ChangeDescription)
	{ }
	bool operator < (const CSlowVariableGraphItem& sg) const
	{
		if (m_dTime < sg.m_dTime) return true;
		return false;
	}
};

typedef std::set<CSlowVariableGraphItem> SLOWGRAPHSET;
typedef SLOWGRAPHSET::const_iterator SLOWGRAPHSETITRCONST;

class CSlowVariableItem
{
public:
	CSlowVariableItem(ptrdiff_t DeviceTypeId, const ResultIds& DeviceIds, std::string_view VarName);
	ptrdiff_t m_DeviceTypeId;
	const ResultIds m_DeviceIds;
	std::string m_strVarName;
	SLOWGRAPHSET m_Graph;
	~CSlowVariableItem();

	void AddGraphPoint(double Time, double Value, double PreviousValue, std::string_view ChangeDescription);
};

struct SlowVarItemCompare
{
	bool operator()(const CSlowVariableItem* lhs, const CSlowVariableItem *rhs) const
	{
		ptrdiff_t DevTypeDiff = lhs->m_DeviceTypeId - rhs->m_DeviceTypeId;
		if (DevTypeDiff > 0) 	return true;
		if (DevTypeDiff < 0) 	return false;
		const ResultIds& lk = lhs->m_DeviceIds;
		const ResultIds& rk = rhs->m_DeviceIds;
		ResultIds::const_iterator lit = lk.begin();
		ResultIds::const_iterator rit = rk.begin();

		while (lit != lk.end() && rit != rk.end())
		{
			ptrdiff_t lx = 0;
			ptrdiff_t rx = 0;

			if (lit != lk.end())
			{
				lx = *lit;
				lit++;
			}

			if (rit != rhs->m_DeviceIds.end())
			{
				rx = *rit;
				rit++;
			}

			lx -= rx;

			if (lx > 0) return true;
			if (lx < 0) return false;
		}

		return lhs->m_strVarName > rhs->m_strVarName;
	}
};

class CSlowVariablesSet : public std::set <CSlowVariableItem*, SlowVarItemCompare>
{
	using ITERATOR = CSlowVariablesSet::iterator;
public:

	void Add(ptrdiff_t DeviceTypeId, 
			 const ResultIds& DeviceIds, 
			 std::string_view VarName, 
			 double Time, double Value, 
			 double PreviousValue, 
			 std::string_view ChangeDescription);

	~CSlowVariablesSet()
	{
		for (auto &it : *this)
			delete it;
	}
};

