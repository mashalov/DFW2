#pragma once
#include "vector"
#include "string"
#include "set"
#include "IResultWriterABI.h"


class CSlowVariableGraphItem
{
public:
	double Time_;
	mutable double Value_;
	mutable std::string Description_;
	CSlowVariableGraphItem(double Time, double Value, std::string_view ChangeDescription) : 
															Time_(Time), 
															Value_(Value),
															Description_(ChangeDescription)
	{ }
	bool operator < (const CSlowVariableGraphItem& sg) const
	{
		if (Time_ < sg.Time_) return true;
		return false;
	}
};

using SLOWGRAPHSET = std::set<CSlowVariableGraphItem> ;

class CSlowVariableItem
{
public:
	CSlowVariableItem(ptrdiff_t DeviceTypeId, const ResultIds& DeviceIds, std::string_view VarName);
	ptrdiff_t DeviceTypeId_;
	const ResultIds DeviceIds_;
	std::string VarName_;
	SLOWGRAPHSET Graph_;
	virtual ~CSlowVariableItem() = default;

	void AddGraphPoint(double Time, double Value, double PreviousValue, std::string_view ChangeDescription);
};

struct SlowVarItemCompare
{
	bool operator()(const CSlowVariableItem* lhs, const CSlowVariableItem *rhs) const
	{
		ptrdiff_t DevTypeDiff{ lhs->DeviceTypeId_- rhs->DeviceTypeId_ };
		if (DevTypeDiff > 0) 	return true;
		if (DevTypeDiff < 0) 	return false;
		const ResultIds& lk{ lhs->DeviceIds_ };
		const ResultIds& rk{ rhs->DeviceIds_ };
		ResultIds::const_iterator lit{ lk.begin() };
		ResultIds::const_iterator rit{ rk.begin() };

		while (lit != lk.end() && rit != rk.end())
		{
			ptrdiff_t lx{ 0 }, rx{ 0 };

			if (lit != lk.end())
			{
				lx = *lit;
				lit++;
			}

			if (rit != rhs->DeviceIds_.end())
			{
				rx = *rit;
				rit++;
			}

			lx -= rx;

			if (lx > 0) return true;
			if (lx < 0) return false;
		}

		return lhs->VarName_ > rhs->VarName_;
	}
};

class CSlowVariablesSet : public std::set <CSlowVariableItem*, SlowVarItemCompare>
{

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

