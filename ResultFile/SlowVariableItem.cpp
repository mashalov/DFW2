#include "stdafx.h"
#include "SlowVariableItem.h"

CSlowVariableItem::CSlowVariableItem(ptrdiff_t DeviceTypeId, const ResultIds& DeviceIds, std::string_view VarName) :
								DeviceTypeId_(DeviceTypeId),
								DeviceIds_(DeviceIds),
								VarName_(VarName)
{
}




void CSlowVariableItem::AddGraphPoint(double Time, double Value, double PreviousValue, std::string_view ChangeDescription)
{
	if (Graph_.empty())
	{
		CSlowVariableGraphItem test(-0.1, PreviousValue, ChangeDescription);
		Graph_.insert(test);
	}

	CSlowVariableGraphItem test(Time, Value, ChangeDescription);
	if (test.Time_ >= 0)
	{
		auto its{ Graph_.insert(test) };
		if (!its.second)
			its.first->Value_ = Value;
	}
}



void CSlowVariablesSet::Add(ptrdiff_t DeviceTypeId, 
							const ResultIds& DeviceIds, 
							std::string_view VarName, 
							double Time, 
							double Value, 
							double PreviousValue, 
							std::string_view ChangeDescription)
{
	CSlowVariableItem *pItem = new CSlowVariableItem(DeviceTypeId, DeviceIds, VarName);
	auto its = insert(pItem);
	if (!its.second)
	{
		delete pItem;
		pItem = *its.first;
	}
	pItem->AddGraphPoint(Time, Value, PreviousValue, ChangeDescription);
}
