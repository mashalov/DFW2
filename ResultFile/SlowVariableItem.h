﻿#pragma once
#include "vector"
#include "string"
#include "set"

typedef std::vector<long> LONGVECTOR;

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
	CSlowVariableItem(long DeviceTypeId, const LONGVECTOR& DeviceIds, std::string_view VarName);
	long m_DeviceTypeId;
	LONGVECTOR m_DeviceIds;
	std::string m_strVarName;
	SLOWGRAPHSET m_Graph;
	~CSlowVariableItem();

	bool AddGraphPoint(double Time, double Value, double PreviousValue, std::string_view ChangeDescription);
};

struct SlowVarItemCompare
{
	bool operator()(const CSlowVariableItem* lhs, const CSlowVariableItem *rhs) const
	{
		long DevTypeDiff = lhs->m_DeviceTypeId - rhs->m_DeviceTypeId;
		if (DevTypeDiff > 0) 	return true;
		if (DevTypeDiff < 0) 	return false;
		const LONGVECTOR& lk = lhs->m_DeviceIds;
		const LONGVECTOR& rk = rhs->m_DeviceIds;
		LONGVECTOR::const_iterator lit = lk.begin();
		LONGVECTOR::const_iterator rit = rk.begin();

		while (lit != lk.end() && rit != rk.end())
		{
			long lx = 0;
			long rx = 0;

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

	bool Add(long DeviceTypeId, 
			 const LONGVECTOR& DeviceIds, 
			 std::string_view VarName, 
			 double Time, double Value, 
			 double PreviousValue, 
			 std::string_view ChangeDescription);

	~CSlowVariablesSet()
	{
		for (auto &it : *this)
			delete it;
	}
#ifdef _MSC_VER	
	bool VariantToIds(VARIANT* pVar, LONGVECTOR& vec) const;
#endif	
};

